/*
 * TI OMAP general purpose memory controller emulation.
 *
 * Copyright (C) 2007-2009 Nokia Corporation
 * Original code written by Andrzej Zaborowski <andrew@openedhand.com>
 * Enhancements for OMAP3 and NAND support written by Juha Riihimäki
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 or
 * (at your option) any later version of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, see <http://www.gnu.org/licenses/>.
 */
#include "hw.h"
#include "flash.h"
#include "omap.h"

/* General-Purpose Memory Controller */
struct omap_gpmc_s {
    qemu_irq irq;
    int accept_256;

    uint8_t revision;
    uint8_t sysconfig;
    uint16_t irqst;
    uint16_t irqen;
    uint16_t timeout;
    uint16_t config;
    uint32_t prefconfig[2];
    int prefcontrol;
    int preffifo;
    int prefcount;
    struct omap_gpmc_cs_file_s {
        uint32_t config[7];
        target_phys_addr_t base;
        size_t size;
        int iomemtype;
        void (*base_update)(void *opaque, target_phys_addr_t new);
        void (*unmap)(void *opaque);
        void *opaque;
    } cs_file[8];
    int ecc_cs;
    int ecc_ptr;
    uint32_t ecc_cfg;
    ECCState ecc[9];
};

static void omap_gpmc_int_update(struct omap_gpmc_s *s)
{
    qemu_set_irq(s->irq, s->irqen & s->irqst);
}

static void omap_gpmc_cs_map(struct omap_gpmc_cs_file_s *f, int base, int mask,
                             int accept_256)
{
    if (accept_256 && !mask) {
        mask = 0x10;
    }
    /* TODO: check for overlapping regions and report access errors */
    if ((mask != 0x8 && mask != 0xc && mask != 0xe && mask != 0xf
         && (accept_256 && mask != 0x10)) ||
        (base < 0 || base >= 0x40) ||
        (base & (accept_256 ? 0x1f : 0x0f) & ~mask)) {
        fprintf(stderr, "%s: wrong cs address mapping/decoding!\n",
                __FUNCTION__);
        return;
    }

    if (!f->opaque) {
        return;
    }

    f->base = base << 24;
    f->size = (0x0fffffff & ~(mask << 24)) + 1;
    /* TODO: rather than setting the size of the mapping (which should be
     * constant), the mask should cause wrapping of the address space, so
     * that the same memory becomes accessible at every <i>size</i> bytes
     * starting from <i>base</i>.  */
    if (f->iomemtype) {
        cpu_register_physical_memory(f->base, f->size, f->iomemtype);
    }

    if (f->base_update) {
        f->base_update(f->opaque, f->base);
    }
}

static void omap_gpmc_cs_unmap(struct omap_gpmc_cs_file_s *f)
{
    if (f->size) {
        if (f->unmap) {
            f->unmap(f->opaque);
        }
        if (f->iomemtype) {
            cpu_register_physical_memory(f->base, f->size, IO_MEM_UNASSIGNED);
        }
        f->base = 0;
        f->size = 0;
    }
}

void omap_gpmc_reset(struct omap_gpmc_s *s)
{
    int i;

    s->sysconfig = 0;
    s->irqst = 0;
    s->irqen = 0;
    omap_gpmc_int_update(s);
    s->timeout = 0;
    s->config = 0xa00;
    s->prefconfig[0] = 0x00004000;
    s->prefconfig[1] = 0x00000000;
    s->prefcontrol = 0;
    s->preffifo = 0;
    s->prefcount = 0;
    for (i = 0; i < 8; i ++) {
        if (s->cs_file[i].config[6] & (1 << 6))	{ /* CSVALID */
            omap_gpmc_cs_unmap(s->cs_file + i);
        }
        s->cs_file[i].config[0] = i ? 1 << 12 : 0;
        s->cs_file[i].config[1] = 0x101001;
        s->cs_file[i].config[2] = 0x020201;
        s->cs_file[i].config[3] = 0x10031003;
        s->cs_file[i].config[4] = 0x10f1111;
        s->cs_file[i].config[5] = 0;
        s->cs_file[i].config[6] = 0xf00 | (i ? 0 : 1 << 6);
        if (s->cs_file[i].config[6] & (1 << 6))	{ /* CSVALID */
            omap_gpmc_cs_map(&s->cs_file[i],
                             s->cs_file[i].config[6] & 0x3f,       /* BASE */
                             (s->cs_file[i].config[6] >> 8) & 0xf, /* MASK */
                             s->accept_256);
        }
    }
    omap_gpmc_cs_map(s->cs_file, 0, 0xf, s->accept_256);
    s->ecc_cs = 0;
    s->ecc_ptr = 0;
    s->ecc_cfg = 0x3fcff000;
    for (i = 0; i < 9; i ++) {
        ecc_reset(&s->ecc[i]);
    }
}

static void omap_gpmc_save_state(QEMUFile *f, void *opaque)
{
    struct omap_gpmc_s *s = (struct omap_gpmc_s *)opaque;
    int i, j;

    qemu_put_byte(f, s->sysconfig);
    qemu_put_be16(f, s->irqst);
    qemu_put_be16(f, s->irqen);
    qemu_put_be16(f, s->timeout);
    qemu_put_be16(f, s->config);
    qemu_put_be32(f, s->prefconfig[0]);
    qemu_put_be32(f, s->prefconfig[1]);
    qemu_put_sbe32(f, s->prefcontrol);
    qemu_put_sbe32(f, s->preffifo);
    qemu_put_sbe32(f, s->prefcount);
    for (i = 0; i < 8; i++) {
        for (j = 0; j < 7; j++) {
            qemu_put_be32(f, s->cs_file[i].config[j]);
        }
    }
    qemu_put_sbe32(f, s->ecc_cs);
    qemu_put_sbe32(f, s->ecc_ptr);
    qemu_put_be32(f, s->ecc_cfg);
    for (i = 0; i < 9; i++) {
        qemu_put_byte(f, s->ecc[i].cp);
        qemu_put_be16(f, s->ecc[i].lp[0]);
        qemu_put_be16(f, s->ecc[i].lp[1]);
        qemu_put_be16(f, s->ecc[i].count);
    }
}

static int omap_gpmc_load_state(QEMUFile *f, void *opaque, int version_id)
{
    struct omap_gpmc_s *s = (struct omap_gpmc_s *)opaque;
    int i, j;

    if (version_id) {
        return -EINVAL;
    }

    s->sysconfig = qemu_get_byte(f);
    s->irqst = qemu_get_be16(f);
    s->irqen = qemu_get_be16(f);
    s->timeout = qemu_get_be16(f);
    s->config = qemu_get_be16(f);
    s->prefconfig[0] = qemu_get_be32(f);
    s->prefconfig[1] = qemu_get_be32(f);
    s->prefcontrol = qemu_get_sbe32(f);
    s->preffifo = qemu_get_sbe32(f);
    s->prefcount = qemu_get_sbe32(f);
    for (i = 0; i < 8; i++) {
        for (j = 0; j < 7; j++) {
            s->cs_file[i].config[j] = qemu_get_be32(f);
        }
        if (s->cs_file[i].config[6] & (1 << 6)) { /* CSVALID */
            omap_gpmc_cs_map(&s->cs_file[i],
                             s->cs_file[i].config[6] & 0x3f,       /* BASE */
                             (s->cs_file[i].config[6] >> 8 & 0xf), /* MASK */
                             s->accept_256);
        }
    }
    s->ecc_cs = qemu_get_sbe32(f);
    s->ecc_ptr = qemu_get_sbe32(f);
    s->ecc_cfg = qemu_get_be32(f);
    for (i = 0; i < 9; i++) {
        s->ecc[i].cp = qemu_get_byte(f);
        s->ecc[i].lp[0] = qemu_get_be16(f);
        s->ecc[i].lp[1] = qemu_get_be16(f);
        s->ecc[i].count = qemu_get_be16(f);
    }

    omap_gpmc_int_update(s);

    return 0;
}

static uint32_t omap_gpmc_read32(void *opaque, target_phys_addr_t addr)
{
    struct omap_gpmc_s *s = (struct omap_gpmc_s *) opaque;
    int cs;
    struct omap_gpmc_cs_file_s *f;
    uint32_t x1, x2, x3, x4;

    switch (addr) {
    case 0x000:	/* GPMC_REVISION */
        return s->revision;

    case 0x010:	/* GPMC_SYSCONFIG */
        return s->sysconfig;

    case 0x014:	/* GPMC_SYSSTATUS */
        return 1;						/* RESETDONE */

    case 0x018:	/* GPMC_IRQSTATUS */
        return s->irqst;

    case 0x01c:	/* GPMC_IRQENABLE */
        return s->irqen;

    case 0x040:	/* GPMC_TIMEOUT_CONTROL */
        return s->timeout;

    case 0x044:	/* GPMC_ERR_ADDRESS */
    case 0x048:	/* GPMC_ERR_TYPE */
        return 0;

    case 0x050:	/* GPMC_CONFIG */
        return s->config;

    case 0x054:	/* GPMC_STATUS */
        return 0x001;

    case 0x060 ... 0x1d4:
        cs = (addr - 0x060) / 0x30;
        addr -= cs * 0x30;
        f = s->cs_file + cs;
        switch (addr) {
        case 0x60:	/* GPMC_CONFIG1 */
            return f->config[0];
        case 0x64:	/* GPMC_CONFIG2 */
            return f->config[1];
        case 0x68:	/* GPMC_CONFIG3 */
            return f->config[2];
        case 0x6c:	/* GPMC_CONFIG4 */
            return f->config[3];
        case 0x70:	/* GPMC_CONFIG5 */
            return f->config[4];
        case 0x74:	/* GPMC_CONFIG6 */
            return f->config[5];
        case 0x78:	/* GPMC_CONFIG7 */
            return f->config[6];
        case 0x84:	/* GPMC_NAND_DATA */
            if (((f->config[0] >> 10) & 3) == 2 /* NAND device type ? */
                && f->opaque) {
                nand_setpins(f->opaque, 0, 0, 0, 1, 0);
                switch (((f->config[0] >> 12) & 3)) {
                case 0: /* 8bit */
                    x1 = nand_getio(f->opaque);
                    x2 = nand_getio(f->opaque);
                    x3 = nand_getio(f->opaque);
                    x4 = nand_getio(f->opaque);
                    return (x4 << 24) | (x3 << 16) | (x2 << 8) | x1;
                case 1: /* 16bit */
                    x1 = nand_getio(f->opaque);
                    x2 = nand_getio(f->opaque);
                    return (x2 << 16) | x1;
                default:
                    return 0;
                }
            }
            return 0;
        default:
            break;
        }
        break;

    case 0x1e0:	/* GPMC_PREFETCH_CONFIG1 */
        return s->prefconfig[0];
    case 0x1e4:	/* GPMC_PREFETCH_CONFIG2 */
        return s->prefconfig[1];
    case 0x1ec:	/* GPMC_PREFETCH_CONTROL */
        return s->prefcontrol;
    case 0x1f0:	/* GPMC_PREFETCH_STATUS */
        return (s->preffifo << 24) |
                ((s->preffifo >
                  ((s->prefconfig[0] >> 8) & 0x7f) ? 1 : 0) << 16) |
                s->prefcount;

    case 0x1f4:	/* GPMC_ECC_CONFIG */
        return s->ecc_cs;
    case 0x1f8:	/* GPMC_ECC_CONTROL */
        return s->ecc_ptr;
    case 0x1fc:	/* GPMC_ECC_SIZE_CONFIG */
        return s->ecc_cfg;
    case 0x200 ... 0x220:	/* GPMC_ECC_RESULT */
        cs = (addr & 0x1f) >> 2;
        /* TODO: check correctness */
        return ((s->ecc[cs].cp    &  0x07) <<  0) |
                ((s->ecc[cs].cp    &  0x38) << 13) |
                ((s->ecc[cs].lp[0] & 0x1ff) <<  3) |
                ((s->ecc[cs].lp[1] & 0x1ff) << 19);

    case 0x230:	/* GPMC_TESTMODE_CTRL */
        return 0;
    case 0x234:	/* GPMC_PSA_LSB */
    case 0x238:	/* GPMC_PSA_MSB */
        return 0x00000000;
    }

    OMAP_BAD_REG(addr);
    return 0;
}

static uint32_t omap_gpmc_read8(void *opaque, target_phys_addr_t addr)
{
    struct omap_gpmc_s *s = (struct omap_gpmc_s *) opaque;
    int cs;
    struct omap_gpmc_cs_file_s *f;

    switch (addr) {
    case 0x060 ... 0x1d4:
        cs = (addr - 0x060) / 0x30;
        addr -= cs * 0x30;
        f = s->cs_file + cs;
        switch (addr) {
        case 0x84 ... 0x87:	/* GPMC_NAND_DATA */
            if (((f->config[0] >> 10) & 3) == 2 /* NAND device type ? */
                && f->opaque) {
                nand_setpins(f->opaque, 0, 0, 0, 1, 0);
                switch (((f->config[0] >> 12) & 3)) {
                case 0: /* 8bit */
                    return nand_getio(f->opaque);
                case 1: /* 16bit */
                    /* reading 8bits from a 16bit device?! */
                    return nand_getio(f->opaque);
                default:
                    return 0;
                }
            }
            return 0;
        default:
            break;
        }
        break;
    default:
        break;
    }
    OMAP_BAD_REG(addr);
    return 0;
}

static uint32_t omap_gpmc_read16(void *opaque, target_phys_addr_t addr)
{
    struct omap_gpmc_s *s = (struct omap_gpmc_s *) opaque;
    int cs;
    struct omap_gpmc_cs_file_s *f;
    uint32_t x1, x2;

    switch (addr) {
    case 0x060 ... 0x1d4:
        cs = (addr - 0x060) / 0x30;
        addr -= cs * 0x30;
        f = s->cs_file + cs;
        switch (addr) {
        case 0x84:	/* GPMC_NAND_DATA */
        case 0x86:
            if (((f->config[0] >> 10) & 3) == 2 /* NAND device type ? */
                && f->opaque) {
                nand_setpins(f->opaque, 0, 0, 0, 1, 0);
                switch (((f->config[0] >> 12) & 3)) {
                case 0: /* 8bit */
                    x1 = nand_getio(f->opaque);
                    x2 = nand_getio(f->opaque);
                    return (x2 << 8) | x1;
                case 1: /* 16bit */
                    return nand_getio(f->opaque);
                default:
                    return 0;
                }
            }
            return 0;
        default:
            break;
        }
        break;
    default:
        break;
    }
    OMAP_BAD_REG(addr);
    return 0;
}

static void omap_gpmc_write32(void *opaque, target_phys_addr_t addr,
                              uint32_t value)
{
    struct omap_gpmc_s *s = (struct omap_gpmc_s *) opaque;
    int cs;
    struct omap_gpmc_cs_file_s *f;

    switch (addr) {
    case 0x000:	/* GPMC_REVISION */
    case 0x014:	/* GPMC_SYSSTATUS */
    case 0x054:	/* GPMC_STATUS */
    case 0x1f0:	/* GPMC_PREFETCH_STATUS */
    case 0x200 ... 0x220:	/* GPMC_ECC_RESULT */
    case 0x234:	/* GPMC_PSA_LSB */
    case 0x238:	/* GPMC_PSA_MSB */
        OMAP_RO_REGV(addr, value);
        break;

    case 0x010:	/* GPMC_SYSCONFIG */
        if ((value >> 3) == 0x3)
            fprintf(stderr, "%s: bad SDRAM idle mode %i\n",
                    __FUNCTION__, value >> 3);
        if (value & 2)
            omap_gpmc_reset(s);
        s->sysconfig = value & 0x19;
        break;

    case 0x018:	/* GPMC_IRQSTATUS */
        s->irqen = ~value;
        omap_gpmc_int_update(s);
        break;

    case 0x01c:	/* GPMC_IRQENABLE */
        s->irqen = value & 0xf03;
        omap_gpmc_int_update(s);
        break;

    case 0x040:	/* GPMC_TIMEOUT_CONTROL */
        s->timeout = value & 0x1ff1;
        break;

    case 0x044:	/* GPMC_ERR_ADDRESS */
    case 0x048:	/* GPMC_ERR_TYPE */
        break;

    case 0x050:	/* GPMC_CONFIG */
        s->config = value & 0xf13;
        break;

    case 0x060 ... 0x1d4:
        cs = (addr - 0x060) / 0x30;
        addr -= cs * 0x30;
        f = s->cs_file + cs;
        switch (addr) {
        case 0x60:	/* GPMC_CONFIG1 */
            f->config[0] = value & 0xffef3e13;
            break;
        case 0x64:	/* GPMC_CONFIG2 */
            f->config[1] = value & 0x001f1f8f;
            break;
        case 0x68:	/* GPMC_CONFIG3 */
            f->config[2] = value & 0x001f1f8f;
            break;
        case 0x6c:	/* GPMC_CONFIG4 */
            f->config[3] = value & 0x1f8f1f8f;
            break;
        case 0x70:	/* GPMC_CONFIG5 */
            f->config[4] = value & 0x0f1f1f1f;
            break;
        case 0x74:	/* GPMC_CONFIG6 */
            f->config[5] = value & 0x00000fcf;
            break;
        case 0x78:	/* GPMC_CONFIG7 */
            if ((f->config[6] ^ value) & 0xf7f) {
                if (f->config[6] & (1 << 6))		/* CSVALID */
                    omap_gpmc_cs_unmap(f);
                if (value & (1 << 6))			/* CSVALID */
                    omap_gpmc_cs_map(f, value & 0x3f,	/* MASKADDR */
                                     (value >> 8 & 0xf));	/* BASEADDR */
            }
            f->config[6] = value & 0x00000f7f;
            break;
        case 0x7c:	/* GPMC_NAND_COMMAND */
        case 0x80:	/* GPMC_NAND_ADDRESS */
        case 0x84:	/* GPMC_NAND_DATA */
            if (((f->config[0] >> 10) & 3) == 2 /* NAND device type ? */
                && f->opaque) {
                switch (addr) {
                case 0x7c: nand_setpins(f->opaque, 1, 0, 0, 1, 0); break; /* CLE */
                case 0x80: nand_setpins(f->opaque, 0, 1, 0, 1, 0); break; /* ALE */
                case 0x84: nand_setpins(f->opaque, 0, 0, 0, 1, 0); break;
                default: break;
                }
                switch (((f->config[0] >> 12) & 3)) {
                case 0: /* 8bit */
                    nand_setio(f->opaque, value & 0xff);
                    nand_setio(f->opaque, (value >> 8) & 0xff);
                    nand_setio(f->opaque, (value >> 16) & 0xff);
                    nand_setio(f->opaque, (value >> 24) & 0xff);
                    break;
                case 1: /* 16bit */
                    nand_setio(f->opaque, value & 0xffff);
                    nand_setio(f->opaque, (value >> 16) & 0xffff);
                    break;
                default:
                    break;
                }
            }
            break;
        default:
            goto bad_reg;
        }
        break;

    case 0x1e0:	/* GPMC_PREFETCH_CONFIG1 */
        s->prefconfig[0] = value & 0x7f8f7fbf;
        /* TODO: update interrupts, fifos, dmas */
        break;

    case 0x1e4:	/* GPMC_PREFETCH_CONFIG2 */
        s->prefconfig[1] = value & 0x3fff;
        break;

    case 0x1ec:	/* GPMC_PREFETCH_CONTROL */
        s->prefcontrol = value & 1;
        if (s->prefcontrol) {
            if (s->prefconfig[0] & 1)
                s->preffifo = 0x40;
            else
                s->preffifo = 0x00;
        }
        /* TODO: start */
        break;

    case 0x1f4:	/* GPMC_ECC_CONFIG */
        s->ecc_cs = 0x8f;
        break;
    case 0x1f8:	/* GPMC_ECC_CONTROL */
        if (value & (1 << 8))
            for (cs = 0; cs < 9; cs ++)
                ecc_reset(&s->ecc[cs]);
        s->ecc_ptr = value & 0xf;
        if (s->ecc_ptr == 0 || s->ecc_ptr > 9) {
            s->ecc_ptr = 0;
            s->ecc_cs &= ~1;
        }
        break;
    case 0x1fc:	/* GPMC_ECC_SIZE_CONFIG */
        s->ecc_cfg = value & 0x3fcff1ff;
        break;
    case 0x230:	/* GPMC_TESTMODE_CTRL */
        if (value & 7)
            fprintf(stderr, "%s: test mode enable attempt\n", __FUNCTION__);
        break;

    default:
    bad_reg:
        OMAP_BAD_REGV(addr, value);
        return;
    }
}

static void omap_gpmc_write8(void *opaque, target_phys_addr_t addr,
                             uint32_t value)
{
    struct omap_gpmc_s *s = (struct omap_gpmc_s *) opaque;
    int cs;
    struct omap_gpmc_cs_file_s *f;

    switch (addr) {
    case 0x060 ... 0x1d4:
        cs = (addr - 0x060) / 0x30;
        addr -= cs * 0x30;
        f = s->cs_file + cs;
        switch (addr) {
        case 0x7c ... 0x7f:	/* GPMC_NAND_COMMAND */
        case 0x80 ... 0x83:	/* GPMC_NAND_ADDRESS */
        case 0x84 ... 0x87:	/* GPMC_NAND_DATA */
            if (((f->config[0] >> 10) & 3) == 2 /* NAND device type ? */
                && f->opaque) {
                switch (addr) {
                case 0x7c ... 0x7f:
                    nand_setpins(f->opaque, 1, 0, 0, 1, 0); /* CLE */
                    break;
                case 0x80 ... 0x83:
                    nand_setpins(f->opaque, 0, 1, 0, 1, 0); /* ALE */
                    break;
                case 0x84 ... 0x87:
                    nand_setpins(f->opaque, 0, 0, 0, 1, 0);
                    break;
                default:
                    break;
                }
                switch (((f->config[0] >> 12) & 3)) {
                case 0: /* 8bit */
                    nand_setio(f->opaque, value & 0xff);
                    break;
                case 1: /* 16bit */
                    /* writing to a 16bit device with 8bit access!? */
                    nand_setio(f->opaque, value & 0xffff);
                    break;
                default:
                    break;
                }
            }
            break;
        default:
            goto bad_reg;
        }
        break;
    default:
    bad_reg:
        OMAP_BAD_REGV(addr, value);
        return;
    }
}

static void omap_gpmc_write16(void *opaque, target_phys_addr_t addr,
                              uint32_t value)
{
    struct omap_gpmc_s *s = (struct omap_gpmc_s *) opaque;
    int cs;
    struct omap_gpmc_cs_file_s *f;

    switch (addr) {
    case 0x060 ... 0x1d4:
        cs = (addr - 0x060) / 0x30;
        addr -= cs * 0x30;
        f = s->cs_file + cs;
        switch (addr) {
        case 0x7c:	/* GPMC_NAND_COMMAND */
        case 0x7e:
        case 0x80:	/* GPMC_NAND_ADDRESS */
        case 0x82:
        case 0x84:	/* GPMC_NAND_DATA */
        case 0x86:
            if (((f->config[0] >> 10) & 3) == 2 /* NAND device type ? */
                && f->opaque) {
                switch (addr) {
                case 0x7c:
                case 0x7e:
                    nand_setpins(f->opaque, 1, 0, 0, 1, 0); /* CLE */
                    break;
                case 0x80:
                case 0x82:
                    nand_setpins(f->opaque, 0, 1, 0, 1, 0); /* ALE */
                    break;
                case 0x84:
                case 0x86:
                    nand_setpins(f->opaque, 0, 0, 0, 1, 0);
                    break;
                default:
                    break;
                }
                switch (((f->config[0] >> 12) & 3)) {
                case 0: /* 8bit */
                    nand_setio(f->opaque, value & 0xff);
                    nand_setio(f->opaque, (value >> 8) & 0xff);
                    break;
                case 1: /* 16bit */
                    nand_setio(f->opaque, value & 0xffff);
                    break;
                default:
                    break;
                }
            }
            break;
        default:
            goto bad_reg;
        }
        break;
    default:
    bad_reg:
        OMAP_BAD_REGV(addr, value);
        return;
    }
}

static CPUReadMemoryFunc * const omap_gpmc_readfn[] = {
    omap_gpmc_read8,
    omap_gpmc_read16,
    omap_gpmc_read32,
};

static CPUWriteMemoryFunc * const omap_gpmc_writefn[] = {
    omap_gpmc_write8,
    omap_gpmc_write16,
    omap_gpmc_write32,
};

struct omap_gpmc_s *omap_gpmc_init(struct omap_mpu_state_s *mpu,
                                   target_phys_addr_t base, qemu_irq irq)
{
    int iomemtype;
    struct omap_gpmc_s *s = qemu_mallocz(sizeof(*s));

    s->accept_256 = cpu_is_omap3630(mpu);
    s->revision = cpu_class_omap3(mpu) ? 0x50 : 0x20;
    omap_gpmc_reset(s);

    iomemtype = cpu_register_io_memory(omap_gpmc_readfn,
                    omap_gpmc_writefn, s, DEVICE_NATIVE_ENDIAN);
    cpu_register_physical_memory(base, 0x1000, iomemtype);

    register_savevm("omap_gpmc", -1, 0,
                    omap_gpmc_save_state, omap_gpmc_load_state, s);
    return s;
}

void omap_gpmc_attach(struct omap_gpmc_s *s, int cs, int iomemtype,
                      void (*base_upd)(void *opaque, target_phys_addr_t new),
                      void (*unmap)(void *opaque), void *opaque,
                      int devicetype)
{
    struct omap_gpmc_cs_file_s *f;

    if (cs < 0 || cs >= 8) {
        fprintf(stderr, "%s: bad chip-select %i\n", __FUNCTION__, cs);
        exit(-1);
    }
    f = &s->cs_file[cs];

    f->iomemtype = iomemtype;
    f->base_update = base_upd;
    f->unmap = unmap;
    f->opaque = opaque;

    f->config[0] &= ~(0x3 << 10);
    f->config[0] |= (devicetype & 3) << 10;
    if (devicetype == 2) { /* NAND */
        f->config[0] &= ~(0x3 << 12);
        if (nand_getbuswidth(f->opaque) == 16)
            f->config[0] |= 1 << 12;
    }

    if (f->config[6] & (1 << 6))				/* CSVALID */
        omap_gpmc_cs_map(f, f->config[6] & 0x3f,    /* BASEADDR */
                         (f->config[6] >> 8) & 0xf, /* MASKADDR */
                         s->accept_256);
}

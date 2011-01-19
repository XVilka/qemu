/*
 * TI OMAP SDRAM controller emulation.
 *
 * Copyright (C) 2007-2008 Nokia Corporation
 * Written by Andrzej Zaborowski <andrew@openedhand.com>
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
#include "omap.h"

/* SDRAM Controller Subsystem */
struct omap_sdrc_s {
    uint8_t config;
    uint32_t cscfg;
    uint32_t sharing;
    uint32_t dlla_ctrl;
    uint32_t power_reg;
    struct {
        uint32_t mcfg;
        uint32_t mr;
        uint32_t emr2;
        uint32_t actim_ctrla;
        uint32_t actim_ctrlb;
        uint32_t rfr_ctrl;
        uint32_t manual;
    } cs[2];
};

void omap_sdrc_reset(struct omap_sdrc_s *s)
{
    s->config    = 0x10;
    s->cscfg     = 0x4;
    s->sharing   = 0; // TODO: copy from system control module
    s->dlla_ctrl = 0;
    s->power_reg = 0x85;
    s->cs[0].mcfg        = s->cs[1].mcfg        = 0; // TODO: copy from system control module!
    s->cs[0].mr          = s->cs[1].mr          = 0x0024;
    s->cs[0].emr2        = s->cs[1].emr2        = 0;
    s->cs[0].actim_ctrla = s->cs[1].actim_ctrla = 0;
    s->cs[0].actim_ctrlb = s->cs[1].actim_ctrlb = 0;
    s->cs[0].rfr_ctrl    = s->cs[1].rfr_ctrl    = 0;
    s->cs[0].manual      = s->cs[1].manual      = 0;
}

static uint32_t omap_sdrc_read(void *opaque, target_phys_addr_t addr)
{
    struct omap_sdrc_s *s = (struct omap_sdrc_s *) opaque;
    int cs = 0;

    switch (addr) {
    case 0x00:  /* SDRC_REVISION */
        return 0x20;

    case 0x10:  /* SDRC_SYSCONFIG */
        return s->config;

    case 0x14:  /* SDRC_SYSSTATUS */
        return 1; /* RESETDONE */

    case 0x40:  /* SDRC_CS_CFG */
        return s->cscfg;

    case 0x44:  /* SDRC_SHARING */
        return s->sharing;

    case 0x48:  /* SDRC_ERR_ADDR */
        return 0;

    case 0x4c:  /* SDRC_ERR_TYPE */
        return 0x8;

    case 0x60:  /* SDRC_DLLA_SCTRL */
        return s->dlla_ctrl;

    case 0x64:  /* SDRC_DLLA_STATUS */
        return ~(s->dlla_ctrl & 0x4);

    case 0x68:  /* SDRC_DLLB_CTRL */
    case 0x6c: /* SDRC_DLLB_STATUS */
        return 0x00;

    case 0x70:  /* SDRC_POWER */
        return s->power_reg;

    case 0xb0 ... 0xd8:
        cs = 1;
        addr -= 0x30;
        /* fall through */
    case 0x80 ... 0xa8:
        switch (addr & 0x3f) {
        case 0x00: /* SDRC_MCFG_x */
            return s->cs[cs].mcfg;
        case 0x04: /* SDRC_MR_x */
            return s->cs[cs].mr;
        case 0x08: /* SDRC_EMR1_x */
            return 0x00;
        case 0x0c: /* SDRC_EMR2_x */
            return s->cs[cs].emr2;
        case 0x10: /* SDRC_EMR3_x */
            return 0x00;
        case 0x14:
            if (cs)
                return s->cs[1].actim_ctrla; /* SDRC_ACTIM_CTRLA_1 */
            return 0x00;                     /* SDRC_DCDL1_CTRL */
        case 0x18:
            if (cs)
                return s->cs[1].actim_ctrlb; /* SDRC_ACTIM_CTRLB_1 */
            return 0x00;                     /* SDRC_DCDL2_CTRL */
        case 0x1c:
            if (!cs)
                return s->cs[0].actim_ctrla; /* SDRC_ACTIM_CTRLA_0 */
            break;
        case 0x20:
            if (!cs)
                return s->cs[0].actim_ctrlb; /* SDRC_ACTIM_CTRLB_0 */
            break;
        case 0x24: /* SDRC_RFR_CTRL_x */
            return s->cs[cs].rfr_ctrl;
        case 0x28: /* SDRC_MANUAL_x */
            return s->cs[cs].manual;
        default:
            break;
        }
        addr += cs * 0x30; // restore address to get correct error messages
        break;

    default:
        break;
    }

    OMAP_BAD_REG(addr);
    return 0;
}

static void omap_sdrc_write(void *opaque, target_phys_addr_t addr,
                            uint32_t value)
{
    struct omap_sdrc_s *s = (struct omap_sdrc_s *) opaque;
    int cs = 0;

    switch (addr) {
    case 0x00:  /* SDRC_REVISION */
    case 0x14:  /* SDRC_SYSSTATUS */
    case 0x48:  /* SDRC_ERR_ADDR */
    case 0x64:  /* SDRC_DLLA_STATUS */
    case 0x6c:  /* SDRC_DLLB_STATUS */
        OMAP_RO_REGV(addr, value);
        break;

    case 0x10:  /* SDRC_SYSCONFIG */
        /* ignore invalid idle mode settings */
        /*if ((value >> 3) != 0x2)
         fprintf(stderr, "%s: bad SDRAM idle mode %i for SDRC_SYSCONFIG (full value 0x%08x)\n",
         __FUNCTION__, value >> 3, value);*/
        if (value & 2)
            omap_sdrc_reset(s);
        s->config = value & 0x18;
        break;

    case 0x40:  /* SDRC_CS_CFG */
        s->cscfg = value & 0x30f;
        break;

    case 0x44:  /* SDRC_SHARING */
        if (!(s->sharing & 0x40000000)) /* LOCK */
            s->sharing = value & 0x40007f00;
        break;

    case 0x4c:  /* SDRC_ERR_TYPE */
        OMAP_BAD_REGV(addr, value);
        break;

    case 0x60:  /* SDRC_DLLA_CTRL */
        s->dlla_ctrl = value & 0xffff00fe;
        break;

    case 0x68:  /* SDRC_DLLB_CTRL */
        /* silently ignore */
        /*OMAP_BAD_REGV(addr, value);*/
        break;

    case 0x70:  /* SDRC_POWER_REG */
        s->power_reg = value & 0x04fffffd;
        break;

    case 0xb0 ... 0xd8:
        cs = 1;
        addr -= 0x30;
        /* fall through */
    case 0x80 ... 0xa8:
        switch (addr & 0x3f) {
        case 0x00: /* SDRC_MCFG_x */
            if (!(s->cs[cs].mcfg & 0x40000000)) { /* LOCKSTATUS */
                if (value & 0x00080000) /* ADDRMUXLEGACY */
                    s->cs[cs].mcfg = value & 0x477bffdf;
                else
                    s->cs[cs].mcfg = value & 0x41fbffdf; // ????
            }
            break;
        case 0x04: /* SDRC_MR_x */
            s->cs[cs].mr = value & 0xfff;
            break;
        case 0x08: /* SDRC_EMR1_x */
            break;
        case 0x0c: /* SDRC_EMR2_x */
            s->cs[cs].emr2 = value & 0xfff;
            break;
        case 0x10: /* SDRC_EMR3_x */
            break;
        case 0x14:
            if (cs)
                s->cs[1].actim_ctrla = value & 0xffffffdf; /* SDRC_ACTIM_CTRLA_1 */
            break;                                         /* SDRC_DCDL1_CTRL */
        case 0x18:
            if (cs)
                s->cs[1].actim_ctrlb = value & 0x000377ff; /* SDRC_ACTIM_CTRLB_1 */
            break;                                         /* SDRC_DCDL2_CTRL */
        case 0x1c:
            if (!cs)
                s->cs[0].actim_ctrla = value & 0xffffffdf; /* SDRC_ACTIM_CTRLA_0 */
            else
                OMAP_BAD_REGV(addr + 0x30, value);
            break;
        case 0x20:
            if (!cs)
                s->cs[0].actim_ctrlb = value & 0x000377ff; /* SDRC_ACTIM_CTRLB_0 */
            else
                OMAP_BAD_REGV(addr + 0x30, value);
            break;
        case 0x24: /* SDRC_RFR_CTRL_x */
            s->cs[cs].rfr_ctrl = value & 0x00ffff03;
            break;
        case 0x28: /* SDRC_MANUAL_x */
            s->cs[cs].manual = value & 0xffff000f;
            break;
        default:
            OMAP_BAD_REGV(addr + cs * 0x30, value);
            break;
        }
        break;

    default:
        OMAP_BAD_REGV(addr, value);
        break;
    }
}

static CPUReadMemoryFunc * const omap_sdrc_readfn[] = {
    omap_badwidth_read32,
    omap_badwidth_read32,
    omap_sdrc_read,
};

static CPUWriteMemoryFunc * const omap_sdrc_writefn[] = {
    omap_badwidth_write32,
    omap_badwidth_write32,
    omap_sdrc_write,
};

struct omap_sdrc_s *omap_sdrc_init(target_phys_addr_t base)
{
    int iomemtype;
    struct omap_sdrc_s *s = qemu_mallocz(sizeof(*s));

    omap_sdrc_reset(s);

    iomemtype = cpu_register_io_memory(omap_sdrc_readfn,
                    omap_sdrc_writefn, s, DEVICE_NATIVE_ENDIAN);
    cpu_register_physical_memory(base, 0x1000, iomemtype);

    return s;
}

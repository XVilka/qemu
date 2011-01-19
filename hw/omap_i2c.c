/*
 * TI OMAP on-chip I2C controller.  Only "new I2C" mode supported.
 *
 * Copyright (C) 2007 Andrzej Zaborowski  <balrog@zabor.org>
 * Copyright (C) 2009 Nokia Corporation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
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
#include "i2c.h"
#include "omap.h"
#include "sysbus.h"

#define I2C_MAX_FIFO_SIZE (1 << 6)
#define I2C_FIFO_SIZE_MASK ((I2C_MAX_FIFO_SIZE) - 1)

typedef struct omap_i2c_bus_s {
    i2c_bus *bus;
    qemu_irq irq;
    qemu_irq drq[2];

    uint8_t revision;
    int fifosize;
    char *id;

    uint16_t mask;
    uint16_t stat;
    uint16_t we;
    uint16_t dma;
    uint16_t count;
    int count_cur;
    uint16_t sysc;
    uint16_t control;
    uint16_t own_addr[4];
    uint16_t slave_addr;
    uint8_t sblock;
    uint8_t divider;
    uint16_t times[2];
    uint16_t test;
    int fifostart;
    int fifolen;
    uint8_t fifo[I2C_MAX_FIFO_SIZE];
} OMAPI2CBusState;

typedef struct omap_i2c_s {
    SysBusDevice busdev;
    int32_t mpu_model;
    int buscount;
    OMAPI2CBusState *bus;
} OMAPI2CState;

/* I2C controller revision register values */
#define OMAP1_INTR_REV    0x11
#define OMAP2_INTR_REV	  0x34
#define OMAP3_INTR_REV    0x3c
#define OMAP3630_INTR_REV 0x40

//#define I2C_DEBUG
#ifdef I2C_DEBUG
#define TRACE(fmt, ...) \
    fprintf(stderr, "%s " fmt "\n", __FUNCTION__, ##__VA_ARGS__)
#else
#define TRACE(...)
#endif

static void omap_i2c_interrupts_update(OMAPI2CBusState *s)
{
    TRACE("IRQ=%04x,RDRQ=%d,XDRQ=%d",
          s->stat & s->mask,
          ((s->dma >> 15 ) & 1) & ((s->stat >> 3) & 1),
          ((s->dma >> 7 ) & 1 )& ((s->stat >> 4 ) & 1));
    qemu_set_irq(s->irq, s->stat & s->mask);
    if ((s->dma >> 15) & 1)                          /* RDMA_EN */
        qemu_set_irq(s->drq[0], (s->stat >> 3) & 1); /* RRDY */
    if ((s->dma >> 7) & 1)                           /* XDMA_EN */
        qemu_set_irq(s->drq[1], (s->stat >> 4) & 1); /* XRDY */
}

static void omap_i2c_fifo_run(OMAPI2CBusState *s)
{
    int ack = 1, i;

    if (!i2c_bus_busy(s->bus))
        return;

    if ((s->control >> 2) & 1) {				/* RM */
        if ((s->control >> 1) & 1) {				/* STP */
            i2c_end_transfer(s->bus);
            s->control &= ~(1 << 1);				/* STP */
            s->count_cur = s->count;
            s->fifolen = 0;
        } else if ((s->control >> 9) & 1) {			/* TRX */
            while (ack && s->fifolen) {
                ack = (i2c_send(s->bus, s->fifo[s->fifostart++]) >= 0);
                s->fifostart &= I2C_FIFO_SIZE_MASK;
                s->fifolen--;
            }
            s->fifolen = 0;
            s->stat |= 1 << 4;					/* XRDY */
        } else {
            for (i = 0; i < 4; i++)
                s->fifo[(s->fifostart + i) & I2C_FIFO_SIZE_MASK] =
                    i2c_recv(s->bus);
            s->fifolen = 4;
            s->stat |= 1 << 3;					/* RRDY */
        }
    } else {
        if ((s->control >> 9) & 1) {                /* TRX */
            TRACE("master transmit, count_cur=%d, fifolen=%d",
                  s->count_cur, s->fifolen);
            for (; ack && s->count_cur && s->fifolen; s->count_cur--) {
                ack = (i2c_send(s->bus, s->fifo[s->fifostart++]) >= 0);
                s->fifostart &= I2C_FIFO_SIZE_MASK;
                s->fifolen--;
            }
            s->stat &= ~0x4410;                     /* XDR | XUDF | XRDY */
            if (ack && s->count_cur) {              /* send more? */
                /* we know that FIFO is empty */
                if (s->revision < OMAP3_INTR_REV)
                    s->stat |= 1 << 4;              /* XRDY */
                else {
                    if (s->count_cur > (s->dma & 0x3f)) /* XTRSH */
                        s->stat |= 1 << 4;          /* XRDY */
                    else
                        s->stat |= 1 << 14;         /* XDR */
                }
            }
            if (!s->count_cur)                      /* everything sent? */
                s->stat |= 1 << 2;                  /* ARDY */
        } else {                                    /* !TRX */
            TRACE("master receive");
            for (; s->count_cur && s->fifolen < s->fifosize; s->count_cur--) {
                i = i2c_recv(s->bus);
                if (i < 0) break; /* stop receiving if nothing to receive */
                s->fifo[(s->fifostart + s->fifolen++) & I2C_FIFO_SIZE_MASK] =
                    (uint8_t)(i & 0xff);
                TRACE("received fifo[%02x] = %02x", s->fifolen - 1,
                      s->fifo[(s->fifostart + s->fifolen - 1)
                              & I2C_FIFO_SIZE_MASK]);
            }
            s->stat &= ~((1 << 3) | (1 << 13));            /* RRDY | RDR */
            if (s->fifolen) {
                if (s->revision < OMAP3_INTR_REV)
                    s->stat |= 1 << 3;                     /* RRDY */
                else {
                    if (s->fifolen > ((s->dma >> 8) & 0x3f)) /* RTRSH */
                        s->stat |= 1 << 3;                 /* RRDY */
                    else
                        s->stat |= 1 << 13;                /* RDR */
                }
            } else if (!s->count_cur && (s->control & 2))  /* STP */
                s->stat |= 1 << 2;                         /* ARDY */
        }
        if (!s->count_cur) {
            TRACE("no more data to transmit/receive");
            i2c_end_transfer(s->bus);
            if ((s->control >> 1) & 1) {   /* STP */
                s->control &= ~0x0602;     /* MST | TRX | STP */
                s->count_cur = s->count;
            }
        }
    }

    s->stat |= (!ack) << 1;	              /* NACK */
    if (!ack)
        s->control &= ~(1 << 1);          /* STP */
    TRACE("finished, STAT = %04x, CNT = %d", s->stat, s->count_cur);
}

static void omap_i2c_bus_reset(OMAPI2CBusState *s)
{
    s->mask = 0;
    s->stat = 0;
    s->dma = 0;
    s->count = 0;
    s->count_cur = 0;
    s->we = 0;
    s->sysc = 0;
    s->fifolen = 0;
    s->fifostart = 0;
    s->control = 0;
    s->own_addr[0] = 0;
    s->own_addr[1] = 0;
    s->own_addr[2] = 0;
    s->own_addr[3] = 0;
    s->slave_addr = 0;
    s->sblock = 0;
    s->divider = 0;
    s->times[0] = 0;
    s->times[1] = 0;
    s->test = 0;

    i2c_end_transfer(s->bus);
}

static uint32_t omap_i2c_read(void *opaque, target_phys_addr_t addr)
{
    OMAPI2CBusState *s = opaque;
    int offset = addr & OMAP_MPUI_REG_MASK;
    uint16_t ret;

    switch (offset) {
        case 0x00: /* I2C_REV */
            TRACE("REV returns %04x", s->revision);
            return s->revision;
        case 0x04: /* I2C_IE */
            TRACE("IE returns %04x", s->mask);
            return s->mask;
        case 0x08: /* I2C_STAT */
            ret = s->stat | (i2c_bus_busy(s->bus) << 12 );
            if (s->revision >= OMAP3_INTR_REV
                && (s->stat & 0x4010)) { /* XRDY or XDR  */
                s->stat |= 1 << 10; /* XUDF as required by errata 1.153 */
            }
            TRACE("STAT returns %04x", ret);
            return ret;
        case 0x0c: /* I2C_IV / I2C_WE */
            if (s->revision >= OMAP3_INTR_REV)
                return s->we;
            if (s->revision >= OMAP2_INTR_REV)
                break;
            ret = ffs(s->stat & s->mask);
            if (ret)
                s->stat ^= 1 << (ret - 1);
            omap_i2c_interrupts_update(s);
            return ret;
        case 0x10: /* I2C_SYSS */
            return (s->control >> 15) & 1; /* reset completed == I2C_EN */
        case 0x14: /* I2C_BUF */
            TRACE("BUF returns %04x", s->dma);
            return s->dma;
        case 0x18: /* I2C_CNT */
            TRACE("CNT returns %04x", s->count_cur);
            return s->count_cur; /* DCOUNT */
        case 0x1c: /* I2C_DATA */
            ret = 0;
            if (s->fifolen) {
                if (s->revision < OMAP3_INTR_REV) {
                    if (s->control & (1 << 14)) /* BE */
                        ret = (((uint16_t)s->fifo[s->fifostart]) << 8)
                            | s->fifo[(s->fifostart + 1) & I2C_FIFO_SIZE_MASK];
                    else
                        ret = (((uint16_t)s->fifo[(s->fifostart + 1)
                                                  & I2C_FIFO_SIZE_MASK]) << 8)
                            | s->fifo[s->fifostart];
                    s->fifostart = (s->fifostart + 2) & I2C_FIFO_SIZE_MASK;
                    if (s->fifolen == 1) {
                        s->stat |= 1 << 15; /* SBD */
                        s->fifolen = 0;
                    } else
                        s->fifolen -= 2;
                    if (!s->fifolen) {
                        s->stat &= ~(1 << 3); /* RRDY */
                        s->stat |= 1 << 2;    /* ARDY */
                    }
                } else {
                    s->stat &= ~(1 << 7); /* AERR */
                    ret = s->fifo[s->fifostart++];
                    s->fifostart &= I2C_FIFO_SIZE_MASK;
                    if (--s->fifolen) {
                        if (s->fifolen <= ((s->dma >> 8) & 0x3f)) {
                            s->stat &= ~(1 << 3); /* RRDY */
                            s->stat |= 1 << 13;   /* RDR */
                        }
                    } else {
                        s->stat &= ~((1 << 3) | (1 << 13)); /* RRDY | RDR */
                        s->stat |= 1 << 2;                  /* ARDY */
                    }
                }
                s->stat &= ~(1 << 11); /* ROVR */
            } else if (s->revision >= OMAP3_INTR_REV)
                s->stat |= (1 << 7); /* AERR */
            TRACE("DATA returns %04x", ret);
            omap_i2c_fifo_run(s);
            omap_i2c_interrupts_update(s);
            return ret;
        case 0x20: /* I2C_SYSC */
            TRACE("SYSC returns %04x", s->sysc);
            return s->sysc;
        case 0x24: /* I2C_CON */
            TRACE("CON returns %04x", s->control);
            return s->control;
        case 0x28: /* I2C_OA / I2C_OA0 */
            return s->own_addr[0];
        case 0x2c: /* I2C_SA */
            return s->slave_addr;
        case 0x30: /* I2C_PSC */
            return s->divider;
        case 0x34: /* I2C_SCLL */
            return s->times[0];
        case 0x38: /* I2C_SCLH */
            return s->times[1];
        case 0x3c: /* I2C_SYSTEST */
            if (s->test & (1 << 15)) { /* ST_EN */
                s->test ^= 0xa;
                return s->test;
            }
            return s->test & ~0x300f;
        case 0x40: /* I2C_BUFSTAT */
            if (s->revision >= OMAP3_INTR_REV) {
                switch (s->fifosize) {
                    case 8:  ret = 0x0000; break;
                    case 16: ret = 0x4000; break;
                    case 32: ret = 0x8000; break;
                    case 64: ret = 0xc000; break;
                    default: ret = 0x0000; break;
                }
                ret |= ((s->fifolen) & 0x3f) << 8;  /* RXSTAT */
                ret |= (s->count_cur) & 0x3f;       /* TXSTAT */
                TRACE("BUFSTAT returns %04x", ret);
                return ret;
            }
            break;
        case 0x44: /* I2C_OA1 */
        case 0x48: /* I2C_OA2 */
        case 0x4c: /* I2C_OA3 */
            if (s->revision >= OMAP3_INTR_REV)
                return s->own_addr[(addr >> 2) & 3];
            break;
        case 0x50: /* I2C_ACTOA */
            if (s->revision >= OMAP3_INTR_REV)
                return 0; /* TODO: determine accessed slave own address */
            break;
        case 0x54: /* I2C_SBLOCK */
            if (s->revision >= OMAP3_INTR_REV)
                return s->sblock;
            break;
        default:
            break;
    }

    OMAP_BAD_REG(addr);
    return 0;
}

static uint32_t omap_i2c_readb(void *opaque, target_phys_addr_t addr)
{
    OMAPI2CBusState *s = opaque;
    int offset = addr & OMAP_MPUI_REG_MASK;
    uint8_t ret;

    switch (offset) {
        case 0x1c: /* I2C_DATA */
            ret = 0;
            if (s->fifolen) {
                if (s->revision < OMAP3_INTR_REV) {
                    if (s->control & (1 << 14)) /* BE */
                        ret = (((uint8_t)s->fifo[s->fifostart]) << 8)
                            | s->fifo[(s->fifostart + 1) & I2C_FIFO_SIZE_MASK];
                    else
                        ret = (((uint8_t)s->fifo[(s->fifostart + 1)
                                                 & I2C_FIFO_SIZE_MASK]) << 8)
                            | s->fifo[s->fifostart];
                    s->fifostart = (s->fifostart + 2) & I2C_FIFO_SIZE_MASK;
                    if (s->fifolen == 1) {
                        s->stat |= 1 << 15; /* SBD */
                        s->fifolen = 0;
                    } else
                        s->fifolen -= 2;
                    if (!s->fifolen) {
                        s->stat &= ~(1 << 3); /* RRDY */
                        s->stat |= 1 << 2;    /* ARDY */
                    }
                } else {
                    s->stat &= ~(1 << 7); /* AERR */
                    ret = (uint8_t)s->fifo[s->fifostart++];
                    s->fifostart &= I2C_FIFO_SIZE_MASK;
                    if (--s->fifolen) {
                        if (s->fifolen <= ((s->dma >> 8) & 0x3f)) {
                            s->stat &= ~(1 << 3); /* RRDY */
                            s->stat |= 1 << 13;   /* RDR */
                        }
                    } else {
                        s->stat &= ~((1 << 3) | (1 << 13)); /* RRDY | RDR */
                        s->stat |= 1 << 2;                  /* ARDY */
                    }
                }
                s->stat &= ~(1 << 11); /* ROVR */
            } else if (s->revision >= OMAP3_INTR_REV)
                s->stat |= (1 << 7); /* AERR */
            TRACE("DATA returns %04x", ret);
            omap_i2c_fifo_run(s);
            omap_i2c_interrupts_update(s);
            return ret;
         default:
            break;
    }

    OMAP_BAD_REG(addr);
    return 0;
}

static void omap_i2c_write(void *opaque, target_phys_addr_t addr,
                           uint32_t value)
{
    OMAPI2CBusState *s = opaque;
    int offset = addr & OMAP_MPUI_REG_MASK;
    int nack;

    switch (offset) {
        case 0x00: /* I2C_REV */
        case 0x10: /* I2C_SYSS */
        case 0x40: /* I2C_BUFSTAT */
        case 0x50: /* I2C_ACTOA */
            OMAP_RO_REG(addr);
            break;
        case 0x04: /* I2C_IE */
            TRACE("IE = %04x", value);
            if (s->revision < OMAP2_INTR_REV) {
                s->mask = value & 0x1f;
            } else if (s->revision < OMAP3_INTR_REV) {
                s->mask = value & 0x3f;
            } else if (s->revision == OMAP3_INTR_REV) {
                s->mask = value & 0x63ff;
            } else { /* omap3630 */
                s->mask = value & 0x6fff;
            }
            omap_i2c_interrupts_update(s);
            break;
        case 0x08: /* I2C_STAT */
            if (s->revision < OMAP2_INTR_REV) {
                OMAP_RO_REG(addr);
            } else {
                TRACE("STAT = %04x", value);
                /* RRDY and XRDY are reset by hardware. (in all versions???) */
                if (s->revision < OMAP3_INTR_REV) {
                    value &= 0x27;
                } else if (s->revision == OMAP3_INTR_REV) {
                    value &= 0x63e7;
                } else { /* omap3630 */
                    value &= 0x6ee7;
                }
                s->stat &= ~value;
                omap_i2c_interrupts_update(s);
            }
            break;
        case 0x0c: /* I2C_IV / I2C_WE */
            if (s->revision < OMAP3_INTR_REV) {
                OMAP_RO_REG(addr);
            } else if (s->revision == OMAP3_INTR_REV) {
                s->we = value & 0x636f;
            } else { /* omap3630 */
                s->we = value & 0x6f6f;
            }
            break;
        case 0x14: /* I2C_BUF */
            TRACE("BUF = %04x", value);
            if (s->revision < OMAP3_INTR_REV)
                s->dma = value & 0x8080;
            else {
                s->dma = value & 0xbfbf;
                if ((value & (1 << 14))    /* RXFIFO_CLR */
                    || (value & (1 << 6))) /* TXFIFO_CLR */
                    s->fifolen = 0;
            }
            if (value & (1 << 15))     /* RDMA_EN */
                s->mask &= ~(1 << 3);  /* RRDY_IE */
            if (value & (1 << 7))      /* XDMA_EN */
                s->mask &= ~(1 << 4);  /* XRDY_IE */
            break;
        case 0x18: /* I2C_CNT */
            TRACE("CNT = %04x", value);
            s->count = value; /* DCOUNT */
            break;
        case 0x1c: /* I2C_DATA */
            TRACE("DATA = %04x", value);
            if (s->revision < OMAP3_INTR_REV) {
                if (s->fifolen > 2) {
                    /* XXX: remote access (qualifier) error - what's that? */
                    break;
                }
                if (s->control & (1 << 14)) { /* BE */
                    s->fifo[(s->fifostart + s->fifolen++)
                            & I2C_FIFO_SIZE_MASK] =
                        (uint8_t)((value >> 8) & 0xff);
                    s->fifo[(s->fifostart + s->fifolen++)
                            & I2C_FIFO_SIZE_MASK] =
                        (uint8_t)(value & 0xff);
                } else {
                    s->fifo[(s->fifostart + s->fifolen++)
                            & I2C_FIFO_SIZE_MASK] =
                        (uint8_t)(value & 0xff);
                    s->fifo[(s->fifostart + s->fifolen++)
                            & I2C_FIFO_SIZE_MASK] =
                        (uint8_t)((value >> 8) & 0xff);
                }
            } else {
                if (s->fifolen < s->fifosize) {
                    s->stat &= ~(1 << 7); /* AERR */
                    s->fifo[(s->fifostart + s->fifolen++)
                            & I2C_FIFO_SIZE_MASK] =
                        (uint8_t)(value & 0xff);
                } else
                    s->stat |= (1 << 7); /* AERR */
            }
            s->stat &= ~(1 << 10); /* XUDF */
            omap_i2c_fifo_run(s);
            omap_i2c_interrupts_update(s);
            break;
        case 0x20: /* I2C_SYSC */
            if (s->revision < OMAP2_INTR_REV) {
                OMAP_BAD_REG(addr);
                break;
            }
            TRACE("SYSC = %04x", value);
            if (value & 2)
                omap_i2c_bus_reset(s);
            else if (s->revision >= OMAP3_INTR_REV)
                s->sysc = value & 0x031d;
            break;
        case 0x24: /* I2C_CON */
            TRACE("CON = %04x", value);
            s->control = value & (s->revision < OMAP3_INTR_REV
                                  ? 0xcf87 : 0xbff3);
            if (~value & (1 << 15)) { /* I2C_EN */
                if (s->revision < OMAP2_INTR_REV)
                    omap_i2c_bus_reset(s);
                break;
            }
            if (s->revision >= OMAP3_INTR_REV &&
                ((value >> 12) & 3) > 1) { /* OPMODE */
                fprintf(stderr,
                        "%s: only FS and HS modes are supported\n",
                        __FUNCTION__);
                break;
            }
            if ((value & (1 << 10))) { /* MST */
                if (value & 1) { /* STT */
                    nack = !!i2c_start_transfer(s->bus, s->slave_addr, /*SA*/
                                                (~value >> 9) & 1);    /*TRX*/
                    s->stat |= nack << 1;        /* NACK */
                    s->control &= ~(1 << 0);     /* STT */
                    s->fifolen = 0;
                    if (nack)
                        s->control &= ~(1 << 1); /* STP */
                    else {
                        s->count_cur = s->count;
                        omap_i2c_fifo_run(s);
                    }
                    omap_i2c_interrupts_update(s);
                } else if (value & 2) { /* STP, but not STT */
                    i2c_end_transfer(s->bus);
                    s->control &= ~0x0602;     /* MST | TRX | STP */
                    s->count_cur = s->count;
                }
            }
            break;
        case 0x28: /* I2C_OA / I2C_OA0 */
            TRACE("OA0 = %04x", value);
            s->own_addr[0] = value & (s->revision < OMAP3_INTR_REV
                                      ? 0x3ff : 0xe3ff);
            /*i2c_set_slave_address(&s->slave[0],
                                  value & (s->revision >= OMAP3_INTR_REV
                                           && (s->control & 0x80)
                                           ? 0x3ff: 0x7f));*/
            break;
        case 0x2c: /* I2C_SA */
            TRACE("SA = %04x", value);
            s->slave_addr = value & 0x3ff;
            break;
        case 0x30: /* I2C_PSC */
            s->divider = value;
            break;
        case 0x34: /* I2C_SCLL */
            s->times[0] = value & (s->revision < OMAP3_INTR_REV
                                   ? 0xff : 0xffff);
            break;
        case 0x38: /* I2C_SCLH */
            s->times[1] = value & (s->revision < OMAP3_INTR_REV
                                   ? 0xff : 0xffff);
            break;
        case 0x3c: /* I2C_SYSTEST */
            if (s->revision < OMAP3_INTR_REV) {
                value &= 0xf805;
            } else if (s->revision == OMAP3_INTR_REV) {
                value &= 0xf815;
            } else { /* omap3630 */
                value = (value & 0xf835) | 0x1c00;
            }
            if ((value & (1 << 15))) { /* ST_EN */
                fprintf(stderr, "%s: System Test not supported\n",
                        __FUNCTION__);
                s->test = (s->test & 0x0a) | value;
            } else {
                value &= ~0xff;
                s->test = (s->test & 0x1f) | value;
            }
            if (value & (1 << 11)) { /* SBB */
                if (s->revision >= OMAP2_INTR_REV) {
                    s->stat |= 0x3f;
                    if (s->revision >= OMAP3_INTR_REV) {
                        s->stat |= 0x6300;
                        if (s->revision > OMAP3_INTR_REV) {
                            s->stat |= 0x0c00;
                        }
                    }
                    omap_i2c_interrupts_update(s);
                }
            }
            break;
        case 0x44: /* I2C_OA1 */
        case 0x48: /* I2C_OA2 */
        case 0x4c: /* I2C_OA3 */
            if (s->revision < OMAP3_INTR_REV) {
                OMAP_BAD_REG(addr);
            } else {
                addr = (addr >> 2) & 3;
                TRACE("OA%d = %04x", (int)addr, value);
                s->own_addr[addr] = value & 0x3ff;
                /*i2c_set_slave_address(&s->slave[addr],
                                      value & ((s->control & (0x80 >> addr))
                                               ? 0x3ff: 0x7f));*/
            }
            break;
        case 0x54: /* I2C_SBLOCK */
            if (s->revision < OMAP3_INTR_REV)
                OMAP_BAD_REG(addr);
            else {
                s->sblock = value & 0x0f;
            }
            break;
        default:
            OMAP_BAD_REG(addr);
            break;
    }
}

static void omap_i2c_writeb(void *opaque, target_phys_addr_t addr,
                            uint32_t value)
{
    OMAPI2CBusState *s = opaque;
    int offset = addr & OMAP_MPUI_REG_MASK;

    switch (offset) {
        case 0x1c: /* I2C_DATA */
            TRACE("DATA = %02x", value);
            if (s->revision < OMAP3_INTR_REV && s->fifolen > 2) {
                /* XXX: remote access (qualifier) error - what's that? */
                break;
            }
            if (s->fifolen < s->fifosize) {
                s->fifo[(s->fifostart + s->fifolen++) & I2C_FIFO_SIZE_MASK] =
                    (uint8_t)(value & 0xff);
                if (s->revision >= OMAP3_INTR_REV)
                    s->stat &= ~(1 << 7); /* AERR */
                s->stat &= ~(1 << 10);    /* XUDF */
                omap_i2c_fifo_run(s);
            } else if (s->revision >= OMAP3_INTR_REV)
                s->stat |= (1 << 7);      /* AERR */
            omap_i2c_interrupts_update(s);
            break;
        default:
            OMAP_BAD_REG(addr);
            break;
    }
}

static CPUReadMemoryFunc * const omap_i2c_readfn[] = {
    omap_i2c_readb,
    omap_i2c_read,
    omap_badwidth_read16,
};

static CPUWriteMemoryFunc * const omap_i2c_writefn[] = {
    omap_i2c_writeb,	/* Only the last fifo write can be 8 bit.  */
    omap_i2c_write,
    omap_badwidth_write16,
};

static int omap_i2c_bus_post_load(void *opaque, int version_id)
{
    OMAPI2CBusState *s = opaque;
    omap_i2c_interrupts_update(s);
    return 0;
}

static const VMStateDescription vmstate_omap_i2c_bus = {
    .name = "omap_i2c",
    .version_id = 1,
    .minimum_version_id = 1,
    .minimum_version_id_old = 1,
    .post_load = omap_i2c_bus_post_load,
    .fields = (VMStateField[]) {
        VMSTATE_UINT16(mask, OMAPI2CBusState),
        VMSTATE_UINT16(stat, OMAPI2CBusState),
        VMSTATE_UINT16(we, OMAPI2CBusState),
        VMSTATE_UINT16(dma, OMAPI2CBusState),
        VMSTATE_UINT16(count, OMAPI2CBusState),
        VMSTATE_INT32(count_cur, OMAPI2CBusState),
        VMSTATE_UINT16(sysc, OMAPI2CBusState),
        VMSTATE_UINT16(control, OMAPI2CBusState),
        VMSTATE_UINT16_ARRAY(own_addr, OMAPI2CBusState, 4),
        VMSTATE_UINT16(slave_addr, OMAPI2CBusState),
        VMSTATE_UINT8(sblock, OMAPI2CBusState),
        VMSTATE_UINT8(divider, OMAPI2CBusState),
        VMSTATE_UINT16_ARRAY(times, OMAPI2CBusState, 2),
        VMSTATE_UINT16(test, OMAPI2CBusState),
        VMSTATE_INT32(fifostart, OMAPI2CBusState),
        VMSTATE_INT32(fifolen, OMAPI2CBusState),
        VMSTATE_INT32(fifosize, OMAPI2CBusState),
        VMSTATE_UINT8_ARRAY(fifo, OMAPI2CBusState, I2C_MAX_FIFO_SIZE),
        VMSTATE_END_OF_LIST()
    }
};

static int omap_i2c_init(SysBusDevice *dev)
{
    int i;
    uint8_t rev;
    OMAPI2CState *s = FROM_SYSBUS(OMAPI2CState, dev);
    if (s->mpu_model < omap2410) {
        rev = OMAP1_INTR_REV;
        s->buscount = 1;
    }  else if (s->mpu_model < omap3430) {
        rev = OMAP2_INTR_REV;
        s->buscount = 2;
    } else {
        s->buscount = 3;
        if (s->mpu_model < omap3630) {
            rev = OMAP3_INTR_REV;
        } else {
            rev = OMAP3630_INTR_REV;
        }
    }
    s->bus = qemu_mallocz(s->buscount * sizeof(*s->bus));
    for (i = 0; i < s->buscount; i++) {
        s->bus[i].revision = rev;
        if (rev < OMAP3_INTR_REV) {
            s->bus[i].fifosize = 4;
        } else {
            s->bus[i].fifosize = (i < 2) ? 8 : 64;
        }
        sysbus_init_irq(dev, &s->bus[i].irq);
        sysbus_init_irq(dev, &s->bus[i].drq[0]);
        sysbus_init_irq(dev, &s->bus[i].drq[1]);
        sysbus_init_mmio(dev, (rev < OMAP2_INTR_REV) ? 0x800 : 0x1000,
                         cpu_register_io_memory(omap_i2c_readfn,
                                                omap_i2c_writefn, &s->bus[i],
                                                DEVICE_NATIVE_ENDIAN));
        s->bus[i].bus = i2c_init_bus(&dev->qdev, NULL);
        vmstate_register(&dev->qdev, i, &vmstate_omap_i2c_bus, &s->bus[i]);
    }
    return 0;
}

static void omap_i2c_reset(DeviceState *dev)
{
    OMAPI2CState *s = FROM_SYSBUS(OMAPI2CState, sysbus_from_qdev(dev));
    int i;
    for (i = 0; i < s->buscount; i++) {
        omap_i2c_bus_reset(&s->bus[i]);
    }
}

static SysBusDeviceInfo omap_i2c_info = {
    .init = omap_i2c_init,
    .qdev.name = "omap_i2c",
    .qdev.size = sizeof(OMAPI2CState),
    .qdev.reset = omap_i2c_reset,
    .qdev.props = (Property[]) {
        DEFINE_PROP_INT32("mpu_model", OMAPI2CState, mpu_model, 0),
        DEFINE_PROP_END_OF_LIST()
    }
};

static void omap_i2c_register_devices(void)
{
    sysbus_register_withprop(&omap_i2c_info);
}

i2c_bus *omap_i2c_bus(DeviceState *omap_i2c, int n)
{
    OMAPI2CState *s = FROM_SYSBUS(OMAPI2CState, sysbus_from_qdev(omap_i2c));
    if (n >= s->buscount) {
        hw_error("%s: requested bus %d (maximum allowed bus number is %d)\n",
                 __FUNCTION__, n, s->buscount - 1);
    }
    return s->bus[n].bus;
}

device_init(omap_i2c_register_devices)

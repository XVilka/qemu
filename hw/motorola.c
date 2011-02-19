/*
 * Motorola Droid, Milestone phones.
 *
 * Copyright (C) Anton Kochkov <anton.kochkov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 or
 * (at your option) version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include "qemu-common.h"
#include "sysemu.h"
#include "omap.h"
#include "arm-misc.h"
#include "irq.h"
#include "console.h"
#include "boards.h"
#include "i2c.h"
#include "spi.h"
#include "devices.h"
#include "flash.h"
#include "hw.h"
#include "bt.h"
#include "net.h"
#include "loader.h"
#include "sysbus.h"
#include "blockdev.h"

//#define MOTLCD_DEBUG

#ifdef MOTLCD_DEBUG
#define TRACE_MOTLCD(fmt, ...) \
    fprintf(stderr, "%s@%d: " fmt "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define TRACE_MOTLCD(...)
#endif

#define OMAP_TAG_NOKIA_BT	0x4e01
#define OMAP_TAG_WLAN_CX3110X	0x4e02
#define OMAP_TAG_CBUS		0x4e03
#define OMAP_TAG_EM_ASIC_BB5	0x4e04

#ifdef CONFIG_GLES2
#include "gles2.h"
#endif

#define MILESTONE_SDRAM_SIZE (256 * 1024 * 1024)
#define MILESTONE_NAND_CS 0
#define MILESTONE_NAND_BUFSIZE (0xc000 << 1)
#define MILESTONE_SMC_CS 1

#define MILESTONE_ONENAND_GPIO       65
#define MILESTONE_CAMFOCUS_GPIO      68
#define MILESTONE_CAMLAUNCH_GPIO     69
#define MILESTONE_SLIDE_GPIO         71
#define MILESTONE_PROXIMITY_GPIO     89
#define MILESTONE_HEADPHONE_EN_GPIO  98
#define MILESTONE_TSC2005_IRQ_GPIO   100
#define MILESTONE_TSC2005_RESET_GPIO 104
#define MILESTONE_CAMCOVER_GPIO      110
#define MILESTONE_KBLOCK_GPIO        113
#define MILESTONE_HEADPHONE_GPIO     177
#define MILESTONE_LIS331DLH_INT2_GPIO 180
#define MILESTONE_LIS331DLH_INT1_GPIO 181

//#define DEBUG_BQ2415X
//#define DEBUG_TPA6130

#define MILESTONE_TRACE(fmt, ...) \
    fprintf(stderr, "%s@%d: " fmt "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)

#ifdef DEBUG_BQ2415X
#define TRACE_BQ2415X(fmt, ...) MILESTONE_TRACE(fmt, ##__VA_ARGS__)
#else
#define TRACE_BQ2415X(...)
#endif
#ifdef DEBUG_TPA6130
#define TRACE_TPA6130(fmt, ...) MILESTONE_TRACE(fmt, ##__VA_ARGS__)
#else
#define TRACE_TPA6130(...)
#endif
#ifdef DEBUG_LIS331DLH
#define TRACE_LIS331DLH(fmt, ...) MILESTONE_TRACE(fmt, ##__VA_ARGS__)
#else
#define TRACE_LIS331DLH(...)
#endif


/* LCD MIPI DBI-C controller (URAL) */
struct motlcd_s {
    SPIDevice spi;
    int resp[4];
    int param[4];
    int p;
    int pm;
    int cmd;

    int sleep;
    int booster;
    int te;
    int selfcheck;
    int partial;
    int normal;
    int vscr;
    int invert;
    int onoff;
    int gamma;
    uint32_t id;

    uint8_t milestone;
    int cabc;
    int brightness;
    int ctrl;
};

static void motlcd_reset(DeviceState *qdev)
{
    struct motlcd_s *s = FROM_SPI_DEVICE(struct motlcd_s,
                                        SPI_DEVICE_FROM_QDEV(qdev));

    //if (!s->sleep)
    //    fprintf(stderr, "%s: Display off\n", __FUNCTION__);

    s->pm = 0;
    s->cmd = 0;

    s->sleep = 1;
    s->booster = 0;
    s->selfcheck =
            (1 << 7) |	/* Register loading OK.  */
            (1 << 5) |	/* The chip is attached.  */
            (1 << 4);	/* Display glass still in one piece.  */
    s->te = 0;
    s->partial = 0;
    s->normal = 1;
    s->vscr = 0;
    s->invert = 0;
    s->onoff = 1;
    s->gamma = 0;
}

static uint32_t motlcd_txrx(SPIDevice *spidev, uint32_t cmd, int len)
{
    struct motlcd_s *s = FROM_SPI_DEVICE(struct motlcd_s, spidev);
    uint8_t ret;

    if (s->milestone && len == 10) {
        cmd >>= 1;
        len--;
    }

    if (len > 9)
        hw_error("%s: FIXME: bad SPI word width %i\n", __FUNCTION__, len);

    if (s->p >= ARRAY_SIZE(s->resp))
        ret = 0;
    else
        ret = s->resp[s->p ++];
    if (s->pm --> 0)
        s->param[s->pm] = cmd;
    else
        s->cmd = cmd;

    switch (s->cmd) {
    case 0x00:	/* NOP */
        TRACE_MOTLCD("NOP");
        break;

    case 0x01:	/* SWRESET */
        TRACE_MOTLCD("SWRESET");
        motlcd_reset(&s->spi.qdev);
        break;

    case 0x02:	/* BSTROFF */
        TRACE_MOTLCD("BSTROFF");
        s->booster = 0;
        break;
    case 0x03:	/* BSTRON */
        TRACE_MOTLCD("BSTRON");
        s->booster = 1;
        break;

    case 0x04:	/* RDDID */
        s->p = 0;
        s->resp[0] = (s->id >> 16) & 0xff;
        s->resp[1] = (s->id >>  8) & 0xff;
        s->resp[2] = (s->id >>  0) & 0xff;
        TRACE_MOTLCD("RDDID 0x%02x 0x%02x 0x%02x",
                    s->resp[0], s->resp[1], s->resp[2]);
        break;

    case 0x06:	/* RD_RED */
    case 0x07:	/* RD_GREEN */
        /* XXX the bootloader sometimes issues RD_BLUE meaning RDDID so
         * for the bootloader one needs to change this.  */
    case 0x08:	/* RD_BLUE */
        TRACE_MOTLCD("RD_RED/GREEN_BLUE 0x01");
        s->p = 0;
        /* TODO: return first pixel components */
        s->resp[0] = 0x01;
        break;

    case 0x09:	/* RDDST */
        s->p = 0;
        s->resp[0] = s->booster << 7;
        s->resp[1] = (5 << 4) | (s->partial << 2) |
                (s->sleep << 1) | s->normal;
        s->resp[2] = (s->vscr << 7) | (s->invert << 5) |
                (s->onoff << 2) | (s->te << 1) | (s->gamma >> 2);
        s->resp[3] = s->gamma << 6;
        TRACE_MOTLCD("RDDST 0x%02x 0x%02x 0x%02x 0x%02x",
                    s->resp[0], s->resp[1], s->resp[2], s->resp[3]);
        break;

    case 0x0a:	/* RDDPM */
        s->p = 0;
        s->resp[0] = (s->onoff << 2) | (s->normal << 3) | (s->sleep << 4) |
                (s->partial << 5) | (s->sleep << 6) | (s->booster << 7);
        TRACE_MOTLCD("RDDPM 0x%02x", s->resp[0]);
        break;
    case 0x0b:	/* RDDMADCTR */
        s->p = 0;
        s->resp[0] = 0;
        TRACE_MOTLCD("RDDMACTR 0x%02x", s->resp[0]);
        break;
    case 0x0c:	/* RDDCOLMOD */
        s->p = 0;
        s->resp[0] = 5;	/* 65K colours */
        TRACE_MOTLCD("RDDCOLMOD 0x%02x", s->resp[0]);
        break;
    case 0x0d:	/* RDDIM */
        s->p = 0;
        s->resp[0] = (s->invert << 5) | (s->vscr << 7) | s->gamma;
        TRACE_MOTLCD("RDDIM 0x%02x", s->resp[0]);
        break;
    case 0x0e:	/* RDDSM */
        s->p = 0;
        s->resp[0] = s->te << 7;
        TRACE_MOTLCD("RDDSM 0x%02x", s->resp[0]);
        break;
    case 0x0f:	/* RDDSDR */
        s->p = 0;
        s->resp[0] = s->selfcheck;
        TRACE_MOTLCD("RDDSDR 0x%02x", s->resp[0]);
        break;

    case 0x10:	/* SLPIN */
        TRACE_MOTLCD("SLPIN");
        s->sleep = 1;
        break;
    case 0x11:	/* SLPOUT */
        TRACE_MOTLCD("SLPOUT");
        s->sleep = 0;
        s->selfcheck ^= 1 << 6;	/* POFF self-diagnosis Ok */
        break;

    case 0x12:	/* PTLON */
        TRACE_MOTLCD("PTLON");
        s->partial = 1;
        s->normal = 0;
        s->vscr = 0;
        break;
    case 0x13:	/* NORON */
        TRACE_MOTLCD("NORON");
        s->partial = 0;
        s->normal = 1;
        s->vscr = 0;
        break;

    case 0x20:	/* INVOFF */
        TRACE_MOTLCD("INVOFF");
        s->invert = 0;
        break;
    case 0x21:	/* INVON */
        TRACE_MOTLCD("INVON");
        s->invert = 1;
        break;

    case 0x22:	/* APOFF */
    case 0x23:	/* APON */
        TRACE_MOTLCD("APON/OFF");
        goto bad_cmd;

    case 0x25:	/* WRCNTR */
        TRACE_MOTLCD("WRCNTR");
        if (s->pm < 0)
            s->pm = 1;
        goto bad_cmd;

    case 0x26:	/* GAMSET */
        if (!s->pm) {
            s->gamma = ffs(s->param[0] & 0xf) - 1;
            TRACE_MOTLCD("GAMSET 0x%02x", s->gamma);
        } else if (s->pm < 0) {
            s->pm = 1;
        }
        break;

    case 0x28:	/* DISPOFF */
        TRACE_MOTLCD("DISPOFF");
        s->onoff = 0;
        break;
    case 0x29:	/* DISPON */
        TRACE_MOTLCD("DISPON");
        s->onoff = 1;
        break;

    case 0x2a:	/* CASET */
    case 0x2b:	/* RASET */
    case 0x2c:	/* RAMWR */
    case 0x2d:	/* RGBSET */
    case 0x2e:	/* RAMRD */
    case 0x30:	/* PTLAR */
    case 0x33:	/* SCRLAR */
        goto bad_cmd;

    case 0x34:	/* TEOFF */
        TRACE_MOTLCD("TEOFF");
        s->te = 0;
        break;
    case 0x35:	/* TEON */
        if (!s->pm) {
            s->te = 1;
            TRACE_MOTLCD("TEON 0x%02x", s->param[0] & 0xff);
        } else if (s->pm < 0) {
            s->pm = 1;
        }
        break;

    case 0x36:	/* MADCTR */
        TRACE_MOTLCD("MADCTR");
        goto bad_cmd;

    case 0x37:	/* VSCSAD */
        TRACE_MOTLCD("VSCSAD");
        s->partial = 0;
        s->normal = 0;
        s->vscr = 1;
        break;

    case 0x38:	/* IDMOFF */
    case 0x39:	/* IDMON */
        TRACE_MOTLCD("IDMON/OFF");
        goto bad_cmd;
    case 0x3a:	/* COLMOD */
        if (!s->pm) {
            TRACE_MOTLCD("COLMOD 0x%02x", s->param[0] & 0xff);
        } else if (s->pm < 0) {
            s->pm = 1;
        }
        break;

    case 0x51: /* WRITE_BRIGHTNESS */
        if (s->milestone) {
            if (!s->pm) {
                s->brightness = s->param[0] & 0xff;
                TRACE_MOTLCD("WRITE_BRIGHTNESS 0x%02x", s->brightness);
            } else if (s->pm < 0) {
                s->pm = 1;
            }
        } else {
            goto bad_cmd;
        }
        break;
    case 0x52: /* READ_BRIGHTNESS */
        if (s->milestone) {
            s->p = 0;
            s->resp[0] = s->brightness;
            TRACE_MOTLCD("READ_BRIGHTNESS 0x%02x", s->resp[0]);
        } else {
            goto bad_cmd;
        }
        break;
    case 0x53: /* WRITE_CTRL */
        if (s->milestone) {
            if (!s->pm) {
                s->ctrl = s->param[0] & 0xff;
                TRACE_MOTLCD("WRITE_CTRL 0x%02x", s->ctrl);
            } else if (s->pm < 0) {
                s->pm = 1;
            }
        } else {
            goto bad_cmd;
        }
        break;
    case 0x54: /* READ_CTRL */
        if (s->milestone) {
            s->p = 0;
            s->resp[0] = s->ctrl;
            TRACE_MOTLCD("READ_CTRL 0x%02x", s->resp[0]);
        } else {
            goto bad_cmd;
        }
        break;
    case 0x55: /* WRITE_CABC */
        if (s->milestone) {
            if (!s->pm) {
                s->cabc = s->param[0] & 0xff;
                TRACE_MOTLCD("WRITE_CABC 0x%02x", s->cabc);
            } else if (s->pm < 0) {
                s->pm = 1;
            }
        } else {
            goto bad_cmd;
        }
        break;
    case 0x56: /* READ_CABC */
        if (s->milestone) {
            s->p = 0;
            s->resp[0] = s->cabc;
            TRACE_MOTLCD("READ_CABC 0x%02x", s->resp[0]);
        } else {
            goto bad_cmd;
        }
        break;

    case 0xb0:	/* CLKINT / DISCTL */
    case 0xb1:	/* CLKEXT */
        if (!s->pm) {
            TRACE_MOTLCD("CLKINT/EXT");
        } else if (s->pm < 0) {
            s->pm = 2;
        }
        break;

    case 0xb4:	/* FRMSEL */
        TRACE_MOTLCD("FRMSEL");
        break;

    case 0xb5:	/* FRM8SEL */
    case 0xb6:	/* TMPRNG / INIESC */
    case 0xb7:	/* TMPHIS / NOP2 */
    case 0xb8:	/* TMPREAD / MADCTL */
    case 0xba:	/* DISTCTR */
    case 0xbb:	/* EPVOL */
        goto bad_cmd;

    case 0xbd:	/* Unknown */
        s->p = 0;
        s->resp[0] = 0;
        s->resp[1] = 1;
        TRACE_MOTLCD("??? 0x%02x 0x%02x", s->resp[0], s->resp[1]);
        break;

    case 0xc2:	/* IFMOD */
        if (!s->pm) {
            TRACE_MOTLCD("IFMOD");
        } else if (s->pm < 0) {
            s->pm = (s->milestone) ? 3 : 2;
        }
        break;

    case 0xc6:	/* PWRCTL */
    case 0xc7:	/* PPWRCTL */
    case 0xd0:	/* EPWROUT */
    case 0xd1:	/* EPWRIN */
    case 0xd4:	/* RDEV */
    case 0xd5:	/* RDRR */
        goto bad_cmd;

    case 0xda:	/* RDID1 */
        s->p = 0;
        s->resp[0] = (s->id >> 16) & 0xff;
        TRACE_MOTLCD("RDID1 0x%02x", s->resp[0]);
        break;
    case 0xdb:	/* RDID2 */
        s->p = 0;
        s->resp[0] = (s->id >>  8) & 0xff;
        TRACE_MOTLCD("RDID2 0x%02x", s->resp[0]);
        break;
    case 0xdc:	/* RDID3 */
        s->p = 0;
        s->resp[0] = (s->id >>  0) & 0xff;
        TRACE_MOTLCD("RDID3 0x%02x", s->resp[0]);
        break;

    default:
    bad_cmd:
        fprintf(stderr, "%s: unknown command 0x%02x\n", __FUNCTION__, s->cmd);
        break;
    }

    return ret;
}

static int motlcd_init(SPIDevice *spidev)
{
    return 0;
}

static SPIDeviceInfo motlcd_info = {
    .init = motlcd_init,
    .txrx = motlcd_txrx,
    .qdev.name = "lcd_milestone",
    .qdev.size = sizeof(struct motlcd_s),
    .qdev.reset = motlcd_reset,
    .qdev.props = (Property[]) {
        DEFINE_PROP_UINT32("id", struct motlcd_s, id, 0),
        DEFINE_PROP_UINT8("milestone", struct motlcd_s, milestone, 0),
        DEFINE_PROP_END_OF_LIST()
    }
};


static uint32_t ssi_read(void *opaque, target_phys_addr_t addr)
{
    switch (addr) {
        case 0x00: /* REVISION */
            return 0x10;
        case 0x14: /* SYSSTATUS */
            return 1; /* RESETDONE */
        default:
            break;
    }
    //printf("%s: addr= " OMAP_FMT_plx "\n", __FUNCTION__, addr);
    return 0;
}

static void ssi_write(void *opaque, target_phys_addr_t addr, uint32_t value)
{
    //printf("%s: addr=" OMAP_FMT_plx ", value=0x%08x\n", __FUNCTION__, addr, value);
}

static CPUReadMemoryFunc *ssi_read_func[] = {
    ssi_read,
    ssi_read,
    ssi_read,
};

static CPUWriteMemoryFunc *ssi_write_func[] = {
    ssi_write,
    ssi_write,
    ssi_write,
};

typedef struct BQ2415XState_s {
    i2c_slave i2c;
    int firstbyte;
    uint8 reg;

    uint8_t id;
    uint8_t st_ctrl;
    uint8_t ctrl;
    uint8_t bat_v;
    uint8_t tcc;
} BQ2415XState;

static void bq2415x_reset(DeviceState *ds)
{
    BQ2415XState *s = FROM_I2C_SLAVE(BQ2415XState, I2C_SLAVE_FROM_QDEV(ds));

    s->firstbyte = 0;
    s->reg = 0;

    s->st_ctrl = 0x50 | 0x80; // 40
    s->ctrl = 0x30;
    s->bat_v = 0x0a;
    s->tcc = 0xa1; // 89
}

static void bq2415x_event(i2c_slave *i2c, enum i2c_event event)
{
    BQ2415XState *s = FROM_I2C_SLAVE(BQ2415XState, i2c);
    if (event == I2C_START_SEND)
        s->firstbyte = 1;
}

static int bq2415x_rx(i2c_slave *i2c)
{
    BQ2415XState *s = FROM_I2C_SLAVE(BQ2415XState, i2c);
    int value = -1;
    switch (s->reg) {
        case 0x00:
            value = s->st_ctrl;
            TRACE_BQ2415X("st_ctrl = 0x%02x", value);
            break;
        case 0x01:
            value = s->ctrl;
            TRACE_BQ2415X("ctrl = 0x%02x", value);
            break;
        case 0x02:
            value = s->bat_v;
            TRACE_BQ2415X("bat_v = 0x%02x", value);
            break;
        case 0x03:
        case 0x3b:
            value = s->id;
            TRACE_BQ2415X("id = 0x%02x", value);
            break;
        case 0x04:
            value = s->tcc;
            TRACE_BQ2415X("tcc = 0x%02x", value);
            break;
        default:
            TRACE_BQ2415X("unknown register 0x%02x", s->reg);
            value = 0;
            break;
    }
    s->reg++;
    return value;
}

static int bq2415x_tx(i2c_slave *i2c, uint8_t data)
{
    BQ2415XState *s = FROM_I2C_SLAVE(BQ2415XState, i2c);
    if (s->firstbyte) {
        s->reg = data;
        s->firstbyte = 0;
    } else {
        switch (s->reg) {
            case 0x00:
                TRACE_BQ2415X("st_ctrl = 0x%02x", data);
                s->st_ctrl = (s->st_ctrl & 0x3f) | (data & 0x40) | 0x80;
                break;
            case 0x01:
                TRACE_BQ2415X("ctrl = 0x%02x", data);
                s->ctrl = data;
                break;
            case 0x02:
                TRACE_BQ2415X("bat_v = 0x%02x", data);
                s->bat_v = data;
                break;
            case 0x04:
                TRACE_BQ2415X("tcc = 0x%02x", data);
                s->tcc = data | 0x80;
                break;
            default:
                TRACE_BQ2415X("unknown register 0x%02x (value 0x%02x)",
                              s->reg, data);
                break;
        }
        s->reg++;
    }
    return 1;
}

static int bq2415x_init(i2c_slave *i2c)
{
    return 0;
}

static I2CSlaveInfo bq2415x_info = {
    .qdev.name = "bq2415x",
    .qdev.size = sizeof(BQ2415XState),
    .qdev.reset = bq2415x_reset,
    .qdev.props = (Property[]) {
        DEFINE_PROP_UINT8("id", BQ2415XState, id, 0x49),
        DEFINE_PROP_END_OF_LIST(),
    },
    .init = bq2415x_init,
    .event = bq2415x_event,
    .recv = bq2415x_rx,
    .send = bq2415x_tx
};

typedef struct tpa6130_s {
    i2c_slave i2c;
    int firstbyte;
    int reg;
    uint8_t data[3];
} TPA6130State;

static void tpa6130_reset(DeviceState *ds)
{
    TPA6130State *s = FROM_I2C_SLAVE(TPA6130State, I2C_SLAVE_FROM_QDEV(ds));
    s->firstbyte = 0;
    s->reg = 0;
    memset(s->data, 0, sizeof(s->data));
}

static void tpa6130_event(i2c_slave *i2c, enum i2c_event event)
{
    TPA6130State *s = FROM_I2C_SLAVE(TPA6130State, i2c);
    if (event == I2C_START_SEND)
        s->firstbyte = 1;
}

static int tpa6130_rx(i2c_slave *i2c)
{
    TPA6130State *s = FROM_I2C_SLAVE(TPA6130State, i2c);
    int value = 0;
    switch (s->reg) {
        case 1 ... 3:
            value = s->data[s->reg - 1];
            TRACE_TPA6130("reg %d = 0x%02x", s->reg, value);
            break;
        case 4: /* VERSION */
            value = 0x01;
            TRACE_TPA6130("version = 0x%02x", value);
            break;
        default:
            TRACE_TPA6130("unknown register 0x%02x", s->reg);
            break;
    }
    s->reg++;
    return value;
}

static int tpa6130_tx(i2c_slave *i2c, uint8_t data)
{
    TPA6130State *s = FROM_I2C_SLAVE(TPA6130State, i2c);
    if (s->firstbyte) {
        s->reg = data;
        s->firstbyte = 0;
    } else {
        switch (s->reg) {
            case 1 ... 3:
                TRACE_TPA6130("reg %d = 0x%02x", s->reg, data);
                s->data[s->reg - 1] = data;
                break;
            default:
                TRACE_TPA6130("unknown register 0x%02x", s->reg);
                break;
        }
        s->reg++;
    }
    return 1;
}

static void tpa6130_irq(void *opaque, int n, int level)
{
    if (n) {
        hw_error("%s: unknown interrupt source %d\n", __FUNCTION__, n);
    } else {
        /* headphone enable */
        TRACE_TPA6130("enable = %d", level);
    }
}

static int tpa6130_init(i2c_slave *i2c)
{
    qdev_init_gpio_in(&i2c->qdev, tpa6130_irq, 1);
    return 0;
}

static I2CSlaveInfo tpa6130_info = {
    .qdev.name = "tpa6130",
    .qdev.size = sizeof(TPA6130State),
    .qdev.reset = tpa6130_reset,
    .init = tpa6130_init,
    .event = tpa6130_event,
    .recv = tpa6130_rx,
    .send = tpa6130_tx
};

typedef struct LIS331DLHState_s {
    i2c_slave i2c;
    int firstbyte;
    uint8_t reg;

    qemu_irq irq[2];
    int8_t axis_max, axis_step;
    int noise, dr_test_ack;

    uint8_t ctrl1, ctrl2, ctrl3;
    uint8_t status;
    struct {
        uint8_t cfg, src, ths, dur;
    } ff_wu[2];
    struct {
        uint8_t cfg, src, thsy_x, thsz;
        uint8_t timelimit, latency, window;
    } click;

    int32_t x, y, z;
} LIS331DLHState;

static void lis331dlh_interrupt_update(LIS331DLHState *s)
{
#ifdef DEBUG_LIS331DLH
    static const char *rules[8] = {
        "GND", "FF_WU_1", "FF_WU_2", "FF_WU_1|2", "DR",
        "???", "???", "CLICK"
    };
#endif
    int active = (s->ctrl3 & 0x80) ? 0 : 1;
    int cond, latch;
    int i;
    for (i = 0; i < 2; i++) {
        switch ((s->ctrl3 >> (i * 3)) & 0x07) {
            case 0:
                cond = 0;
                break;
            case 1:
                cond = s->ff_wu[0].src & 0x40;
                latch = s->ff_wu[0].cfg & 0x40;
                break;
            case 2:
                cond = s->ff_wu[1].src & 0x40;
                latch = s->ff_wu[1].cfg & 0x40;
                break;
            case 3:
                cond = ((s->ff_wu[0].src | s->ff_wu[1].src) & 0x40);
                latch = ((s->ff_wu[0].cfg | s->ff_wu[1].cfg) & 0x40);
                break;
            case 4:
                cond = (((s->ff_wu[0].src | s->ff_wu[1].src) & 0x3f) &
                        (((s->ctrl1 & 0x01) ? 0x03 : 0x00) |
                         ((s->ctrl1 & 0x02) ? 0x0c : 0x00) |
                         ((s->ctrl1 & 0x04) ? 0x30 : 0x00)));
                latch = 0;
                break;
            case 7:
                cond = s->click.src & 0x40;
                latch = s->click.cfg & 0x40;
                break;
            default:
                TRACE_LIS331DLH("unsupported irq config (%d)",
                               (s->ctrl3 >> (i * 3)) & 0x07);
                cond = 0;
                latch = 0;
                break;
        }
        TRACE_LIS331DLH("%s: %s irq%d", rules[(s->ctrl3 >> (i * 3)) & 0x07],
                       cond ? (latch ? "activate" : "pulse") : "deactivate",
                       i);
        qemu_set_irq(s->irq[i], cond ? active : !active);
        if (cond && !latch) {
            qemu_set_irq(s->irq[i], !active);
        }
    }
}

static void lis331dlh_trigger(LIS331DLHState *s, int axis, int value)
{
    if (value > s->axis_max) value = s->axis_max;
    if (value < -s->axis_max) value = -s->axis_max;
    switch (axis) {
        case 0: s->x = value; break;
        case 1: s->y = value; break;
        case 2: s->z = value; break;
        default: break;
    }
    if (s->status & (0x01 << axis)) {
        s->status |= 0x10 << axis;
    } else {
        s->status |= 0x01 << axis;
    }
    if ((s->status & 0x07) == 0x07) {
        s->status |= 0x08;
    }
    if ((s->status & 0x70) == 0x70) {
        s->status |= 0x80;
    }
    uint8_t bit = 0x02 << (axis << 1); /* over threshold */
    s->ff_wu[0].src |= bit;
    s->ff_wu[1].src |= bit;

    int i = 0;
    for (; i < 2; i++) {
        if (s->ff_wu[i].src & 0x3f) {
            if (s->ff_wu[i].cfg & 0x80) {
                if ((s->ff_wu[i].cfg & 0x3f) == (s->ff_wu[i].src & 0x3f)) {
                    s->ff_wu[i].src |= 0x40;
                }
            } else {
                if (s->ff_wu[i].src & s->ff_wu[i].cfg & 0x3f) {
                    s->ff_wu[i].src |= 0x40;
                }
            }
        }
        TRACE_LIS331DLH("FF_WU_%d: CFG=0x%02x, SRC=0x%02x",
                       i, s->ff_wu[i].cfg, s->ff_wu[i].src);
    }

    lis331dlh_interrupt_update(s);
}

static void lis331dlh_step(void *opaque, int axis, int high, int activate)
{
    TRACE_LIS331DLH("axis=%d, high=%d, activate=%d", axis, high, activate);
    LIS331DLHState *s = opaque;
    if (activate) {
        int v = 0;
        switch (axis) {
            case 0: v = s->x + (high ? s->axis_step : -s->axis_step); break;
            case 1: v = s->y + (high ? s->axis_step : -s->axis_step); break;
            case 2: v = s->z + (high ? s->axis_step : -s->axis_step); break;
            default: break;
        }
        if (v > s->axis_max) v = -(s->axis_max - s->axis_step);
        if (v < -s->axis_max) v = s->axis_max - s->axis_step;
        lis331dlh_trigger(s, axis, v);
    }
}

static int lis331dlh_change(DeviceState *dev, const char *target,
                           const char *arg)
{
    LIS331DLHState *s = (LIS331DLHState *)dev;
    int axis;
    if (!strcmp(target, "x")) {
        axis = 0;
    } else if (!strcmp(target, "y")) {
        axis = 1;
    } else if (!strcmp(target, "z")) {
        axis = 2;
    } else {
        return -1;
    }
    int value = 0;
    if (sscanf(arg, "%d", &value) != 1) {
        return -1;
    }
    lis331dlh_trigger(s, axis, value);
    return 0;
}

static void lis331dlh_reset(DeviceState *ds)
{
    LIS331DLHState *s = FROM_I2C_SLAVE(LIS331DLHState, I2C_SLAVE_FROM_QDEV(ds));

    s->firstbyte = 0;
    s->reg = 0;

    s->noise = 4;
    s->dr_test_ack = 0;

    s->ctrl1 = 0x03;
    s->ctrl2 = 0x00;
    s->ctrl3 = 0x00;
    s->status = 0x00;

    memset(s->ff_wu, 0x00, sizeof(s->ff_wu));
    memset(&s->click, 0x00, sizeof(s->click));

    s->x = 0;
    s->y = -s->axis_max;
    s->z = 0;

    lis331dlh_interrupt_update(s);
}

static void lis331dlh_event(i2c_slave *i2c, enum i2c_event event)
{
    LIS331DLHState *s = FROM_I2C_SLAVE(LIS331DLHState, i2c);
    if (event == I2C_START_SEND)
        s->firstbyte = 1;
}

static uint8_t lis331dlh_readcoord(LIS331DLHState *s, int coord)
{
    int v;

    switch (coord) {
        case 0:
            v = s->x;
            break;
        case 1:
            v = s->y;
            break;
        case 2:
            v = s->z;
            break;
        default:
            hw_error("%s: unknown axis %d", __FUNCTION__, coord);
            break;
    }
    s->status &= ~(0x88 | (0x11 << coord));
    if (s->ctrl1 & 0x10) {
        switch (coord) {
            case 0:
                v -= s->noise;
                break;
            case 1:
            case 2:
                v += s->noise;
                break;
            default:
                break;
        }
        if (++s->noise == 32) {
            s->noise = 4;
        }
        int dr1 = ((s->ctrl3 & 0x07) == 4);
        int dr2 = (((s->ctrl3 >> 3) & 0x07) == 4);
        if (!s->dr_test_ack++) {
            if (dr1) {
                qemu_irq_pulse(s->irq[0]);
            }
            if (dr2) {
                qemu_irq_pulse(s->irq[1]);
            }
        } else if (s->dr_test_ack == 1 + (dr1 + dr2) * 3) {
            s->dr_test_ack = 0;
        }
    }
    return (uint8_t)v;
}

static int lis331dlh_rx(i2c_slave *i2c)
{
    LIS331DLHState *s = FROM_I2C_SLAVE(LIS331DLHState, i2c);
    int value = -1;
    int n = 0;
    switch (s->reg & 0x7f) {
        case 0x00 ... 0x0e:
        case 0x10 ... 0x1f:
        case 0x23 ... 0x26:
        case 0x28:
        case 0x2a:
        case 0x2c:
        case 0x2e ... 0x2f:
        case 0x3a:
            value = 0;
            TRACE_LIS331DLH("reg 0x%02x = 0x%02x (unused/reserved reg)",
                           s->reg & 0x7f, value);
            break;
        case 0x0f:
            value = 0x3b;
            TRACE_LIS331DLH("WHOAMI = 0x%02x", value);
            break;
        case 0x20:
            value = s->ctrl1;
            TRACE_LIS331DLH("CTRL1 = 0x%02x", value);
            break;
        case 0x21:
            value = s->ctrl2;
            TRACE_LIS331DLH("CTRL2 = 0x%02x", value);
            break;
        case 0x22:
            value = s->ctrl3;
            TRACE_LIS331DLH("CTRL3 = 0x%02x", value);
            break;
        case 0x27:
            value = s->status;
            TRACE_LIS331DLH("STATUS = 0x%02x", value);
            break;
        case 0x29:
            value = lis331dlh_readcoord(s, 0);
            TRACE_LIS331DLH("X = 0x%02x", value);
            break;
        case 0x2b:
            value = lis331dlh_readcoord(s, 1);
            TRACE_LIS331DLH("Y = 0x%02x", value);
            break;
        case 0x2d:
            value = lis331dlh_readcoord(s, 2);
            TRACE_LIS331DLH("Z = 0x%02x", value);
            break;
        case 0x34: n++;
        case 0x30:
            value = s->ff_wu[n].cfg;
            TRACE_LIS331DLH("FF_WU%d.CFG = 0x%02x", n + 1, value);
            break;
        case 0x35: n++;
        case 0x31:
            value = s->ff_wu[n].src;
            TRACE_LIS331DLH("FF_WU%d.SRC = 0x%02x", n + 1, value);
            s->ff_wu[n].src = 0; //&= ~0x40;
            lis331dlh_interrupt_update(s);
            break;
        case 0x36: n++;
        case 0x32:
            value = s->ff_wu[n].ths;
            TRACE_LIS331DLH("FF_WU%d.THS = 0x%02x", n + 1, value);
            break;
        case 0x37: n++;
        case 0x33:
            value = s->ff_wu[n].dur;
            TRACE_LIS331DLH("FF_WU%d.DUR = 0x%02x", n + 1, value);
            break;
        case 0x38:
            value = s->click.cfg;
            TRACE_LIS331DLH("CLICK_CFG = 0x%02x", value);
            break;
        case 0x39:
            value = s->click.src;
            TRACE_LIS331DLH("CLICK_SRC = 0x%02x", value);
            s->click.src &= ~0x40;
            lis331dlh_interrupt_update(s);
            break;
        case 0x3b:
            value = s->click.thsy_x;
            TRACE_LIS331DLH("CLICK_THSY_X = 0x%02x", value);
            break;
        case 0x3c:
            value = s->click.thsz;
            TRACE_LIS331DLH("CLICK_THSZ = 0x%02x", value);
            break;
        case 0x3d:
            value = s->click.timelimit;
            TRACE_LIS331DLH("CLICK_TIMELIMIT = 0x%02x", value);
            break;
        case 0x3e:
            value = s->click.latency;
            TRACE_LIS331DLH("CLICK_LATENCY = 0x%02x", value);
            break;
        case 0x3f:
            value = s->click.window;
            TRACE_LIS331DLH("CLICK_WINDOW = 0x%02x", value);
            break;
        default:
            hw_error("%s: unknown register 0x%02x", __FUNCTION__,
                     s->reg & 0x7f);
            value = 0;
            break;
    }
    if (s->reg & 0x80) { /* auto-increment? */
        s->reg = (s->reg + 1) | 0x80;
    }
    return value;
}

static int lis331dlh_tx(i2c_slave *i2c, uint8_t data)
{
    LIS331DLHState *s = FROM_I2C_SLAVE(LIS331DLHState, i2c);
    if (s->firstbyte) {
        s->reg = data;
        s->firstbyte = 0;
    } else {
        int n = 0;
        switch (s->reg & 0x7f) {
            case 0x20:
                TRACE_LIS331DLH("CTRL1 = 0x%02x", data);
                s->ctrl1 = data;
                break;
            case 0x21:
                TRACE_LIS331DLH("CTRL2 = 0x%02x", data);
                s->ctrl2 = data;
                break;
            case 0x22:
                TRACE_LIS331DLH("CTRL3 = 0x%02x", data);
                s->ctrl3 = data;
                lis331dlh_interrupt_update(s);
                break;
            case 0x34: n++;
            case 0x30:
                TRACE_LIS331DLH("FF_WU%d.CFG = 0x%02x", n + 1, data);
                s->ff_wu[n].cfg = data;
                break;
            case 0x36: n++;
            case 0x32:
                TRACE_LIS331DLH("FF_WU%d.THS = 0x%02x", n + 1, data);
                s->ff_wu[n].ths = data;
                break;
            case 0x37: n++;
            case 0x33:
                TRACE_LIS331DLH("FF_WU%d.DUR = 0x%02x", n + 1, data);
                s->ff_wu[n].dur = data;
                break;
            case 0x38:
                TRACE_LIS331DLH("CLICK_CFG = 0x%02x", data);
                s->click.cfg = data;
                break;
            case 0x39:
                TRACE_LIS331DLH("CLICK_SRC = 0x%02x", data);
                s->click.src = data;
                break;
            case 0x3b:
                TRACE_LIS331DLH("CLICK_THSY_X = 0x%02x", data);
                s->click.thsy_x = data;
                break;
            case 0x3c:
                TRACE_LIS331DLH("CLICK_THSZ = 0x%02x", data);
                s->click.thsz = data;
                break;
            case 0x3d:
                TRACE_LIS331DLH("CLICK_TIMELIMIT = 0x%02x", data);
                s->click.timelimit = data;
                break;
            case 0x3e:
                TRACE_LIS331DLH("CLICK_LATENCY = 0x%02x", data);
                s->click.latency = data;
                break;
            case 0x3f:
                TRACE_LIS331DLH("CLICK_WINDOW = 0x%02x", data);
                s->click.window = data;
                break;
            default:
                hw_error("%s: unknown register 0x%02x (value 0x%02x)",
                         __FUNCTION__, s->reg & 0x7f, data);
                break;
        }
        if (s->reg & 0x80) { /* auto-increment? */
            s->reg = (s->reg + 1) | 0x80;
        }
    }
    return 1;
}

static int lis331dlh_init(i2c_slave *i2c)
{
    LIS331DLHState *s = FROM_I2C_SLAVE(LIS331DLHState, i2c);
    s->axis_max = 58;
    s->axis_step = s->axis_max;// / 2;
    qdev_init_gpio_out(&i2c->qdev, s->irq, 2);
    return 0;
}

static I2CSlaveInfo lis331dlh_info = {
    .qdev.name = "lis331dlh",
    .qdev.size = sizeof(LIS331DLHState),
    .qdev.reset = lis331dlh_reset,
    .qdev.change = lis331dlh_change,
    .qdev.props = (Property[]) {
        DEFINE_PROP_INT32("x", LIS331DLHState, x, 0),
        DEFINE_PROP_INT32("y", LIS331DLHState, y, 0),
        DEFINE_PROP_INT32("z", LIS331DLHState, z, 0),
        DEFINE_PROP_END_OF_LIST(),
    },
    .init = lis331dlh_init,
    .event = lis331dlh_event,
    .recv = lis331dlh_rx,
    .send = lis331dlh_tx
};

struct milestone_s {
    struct omap_mpu_state_s *cpu;
    void *twl4030;
    DeviceState *nand;
    DeviceState *motlcd;
    DeviceState *tsc2005;
    DeviceState *bq2415x;
    DeviceState *tpa6130;
	DeviceState *lis331dlh;
    DeviceState *smc;
#ifdef CONFIG_GLES2
    void *gles2;
#endif
    int extended_key;
    int slide_open;
    int camera_cover_open;
    int headphone_connected;
    QEMUTimer *shutdown_timer;
};

#ifdef CONFIG_SKINNING
#include "skin/skin_switchstate.h"
static int milestone_switchstate_callback(void *opaque, const int keycode)
{
    struct milestone_s *s = opaque;
    switch (keycode) {
        case 0x3b: return s->slide_open;
        case 0x3d: return s->camera_cover_open;
        case 0x40: return s->headphone_connected;
        default: break;
    }
    return -1;
}
#endif

/* this takes care of the keys which are not located on the
 * n900 keypad (note that volume up/down keys are handled by
 * the keypad eventhough the keys are not located on the keypad)
 * as well as triggering some other hardware button/switch-like
 * events that are mapped to the host keyboard:
 *
 * escape ... power
 * f1 ....... keypad slider open/close
 * f2 ....... keypad lock
 * f3 ....... camera lens cover open/close
 * f4 ....... camera focus
 * f5 ....... camera take picture
 * f6 ....... stereo headphone connect/disconnect
 * kp1 ...... decrease accelerometer x axis value
 * kp2 ...... increase accelerometer x axis value
 * kp4 ...... decrease accelerometer y axis value
 * kp5 ...... increase accelerometer y axis value
 * kp7 ...... decrease accelerometer z axis value
 * kp8 ...... increase accelerometer z axis value
 */
static void milestone_key_handler(void *opaque, int keycode)
{
    struct milestone_s *s = opaque;
    if (!s->extended_key && keycode == 0xe0) {
        s->extended_key = 0x80;
    } else {
        int release = keycode & 0x80;
        keycode = (keycode & 0x7f) | s->extended_key;
        s->extended_key = 0;
        switch (keycode) {
            case 0x01: /* escape */
                twl4030_set_powerbutton_state(s->twl4030, !release);
                break;
            case 0x3b: /* f1 */
                if (release) {
                    s->slide_open = !s->slide_open;
                    qemu_set_irq(qdev_get_gpio_in(s->cpu->gpio,
                                                  MILESTONE_SLIDE_GPIO),
                                 !s->slide_open);
                }
                break;
            case 0x3c: /* f2 */
                qemu_set_irq(qdev_get_gpio_in(s->cpu->gpio, MILESTONE_KBLOCK_GPIO),
                             !!release);
                break;
            case 0x3d: /* f3 */
                if (release) {
                    s->camera_cover_open = !s->camera_cover_open;
                    qemu_set_irq(qdev_get_gpio_in(s->cpu->gpio,
                                                  MILESTONE_CAMCOVER_GPIO),
                                 s->camera_cover_open);
                }
                break;
            case 0x3e: /* f4 */
                qemu_set_irq(qdev_get_gpio_in(s->cpu->gpio,
                                              MILESTONE_CAMFOCUS_GPIO),
                             !!release);
                break;
            case 0x3f: /* f5 */
                qemu_set_irq(qdev_get_gpio_in(s->cpu->gpio,
                                              MILESTONE_CAMLAUNCH_GPIO),
                             !!release);
                break;
            case 0x40: /* f6 */
                if (release) {
                    s->headphone_connected = !s->headphone_connected;
                    qemu_set_irq(qdev_get_gpio_in(s->cpu->gpio,
                                                  MILESTONE_HEADPHONE_GPIO),
                                 !s->headphone_connected);
                }
                break;
			case 0x4f ... 0x50: /* kp1,2 */
                lis331dlh_step(s->lis331dlh, 0, keycode - 0x4f, !release);
                break;
            case 0x4b ... 0x4c: /* kp4,5 */
                lis331dlh_step(s->lis331dlh, 1, keycode - 0x4b, !release);
                break;
            case 0x47 ... 0x48: /* kp7,8 */
                lis331dlh_step(s->lis331dlh, 2, keycode - 0x47, !release);
                break;

            default:
                break;
        }
    }
}

static void milestone_reset(void *opaque)
{
    struct milestone_s *s = opaque;
    qemu_irq_raise(qdev_get_gpio_in(s->cpu->gpio, MILESTONE_KBLOCK_GPIO));
    qemu_set_irq(qdev_get_gpio_in(s->cpu->gpio, MILESTONE_HEADPHONE_GPIO),
                 !s->headphone_connected);
    qemu_irq_raise(qdev_get_gpio_in(s->cpu->gpio, MILESTONE_CAMLAUNCH_GPIO));
    qemu_irq_raise(qdev_get_gpio_in(s->cpu->gpio, MILESTONE_CAMFOCUS_GPIO));
    qemu_set_irq(qdev_get_gpio_in(s->cpu->gpio, MILESTONE_CAMCOVER_GPIO),
                 s->camera_cover_open);
    qemu_set_irq(qdev_get_gpio_in(s->cpu->gpio, MILESTONE_SLIDE_GPIO),
                 !s->slide_open);
    /* FIXME: hack - prevent reboot if shutdown was requested */
    if (s->shutdown_timer) {
        qemu_system_shutdown_request();
    }
}

static void milestone_shutdown_timer_callback(void *opaque)
{
    static int power_state = 0;
    struct milestone_s *s = opaque;
    power_state = !power_state;
    twl4030_set_powerbutton_state(s->twl4030, power_state);
    qemu_mod_timer(s->shutdown_timer,
                   qemu_get_clock(vm_clock)
                   + get_ticks_per_sec() * (power_state ? 5LL : 1LL));
}

static int milestone_display_close_callback(void *opaque)
{
    struct milestone_s *s = opaque;
    if (s->shutdown_timer == NULL) {
        s->shutdown_timer = qemu_new_timer(vm_clock,
                                           milestone_shutdown_timer_callback,
                                           s);
        milestone_shutdown_timer_callback(s);
    }
    return 0;
}

static uint16_t milestone_twl4030_madc_callback(twl4030_adc_type type, int ch)
{
    return 0x3ff;
}

static const TWL4030KeyMap milestone_twl4030_keymap[] = {
    {0x10, 0, 0}, /* Q */
    {0x11, 0, 1}, /* W */
    {0x12, 0, 2}, /* E */
    {0x13, 0, 3}, /* R */
    {0x14, 0, 4}, /* T */
    {0x15, 0, 5}, /* Y */
    {0x16, 0, 6}, /* U */
    {0x17, 0, 7}, /* I */
    {0x18, 1, 0}, /* O */
    {0x20, 1, 1}, /* D */
    {0x34, 1, 2}, /* . */
    {0x2f, 1, 3}, /* V */
    {0xd0, 1, 4}, /* DOWN */
    {0x41, 1, 7}, /* F7 -- volume/zoom down */
    {0x19, 2, 0}, /* P */
    {0x21, 2, 1}, /* F */
    {0xc8, 2, 2}, /* UP */
    {0x30, 2, 3}, /* B */
    {0xcd, 2, 4}, /* RIGHT */
    {0x42, 2, 7}, /* F8 -- volume/zoom up */
    {0x33, 3, 0}, /* , */
    {0x22, 3, 1}, /* G */
    {0x1c, 3, 2}, /* ENTER */
    {0x31, 3, 3}, /* N */
    {0x0e, 4, 0}, /* BACKSPACE */
    {0x23, 4, 1}, /* H */
    {0x32, 4, 3}, /* M */
    {0x1d, 4, 4}, /* LEFTCTRL */
    {0x9d, 4, 4}, /* RIGHTCTRL */
    {0x24, 5, 1}, /* J */
    {0x2c, 5, 2}, /* Z */
    {0x39, 5, 3}, /* SPACE */
    {0x38, 5, 4}, /* LEFTALT -- "fn" */
    {0xb8, 5, 4}, /* RIGHTALT -- "fn" */
    {0x1e, 6, 0}, /* A */
    {0x25, 6, 1}, /* K */
    {0x2d, 6, 2}, /* X */
    {0x39, 6, 3}, /* SPACE */
    {0x2a, 6, 4}, /* LEFTSHIFT */
    {0x36, 6, 4}, /* RIGHTSHIFT */
    {0x1f, 7, 0}, /* S */
    {0x26, 7, 1}, /* L */
    {0x2e, 7, 2}, /* C */
    {0xcb, 7, 3}, /* LEFT */
    //    {0x10, 0xff, 2}, /* F9 */
    //    {0x10, 0xff, 4}, /* F10 */
    //    {0x10, 0xff, 5}, /* F11 */
    {-1, -1, -1}
};

static MouseTransformInfo milestone_pointercal = {
    .x = 800,
    .y = 480,
    .a = {14114,  18, -2825064,  34,  -8765, 32972906, 65536},
};

static void milestone_init(ram_addr_t ram_size,
                      const char *boot_device,
                      const char *kernel_filename,
                      const char *kernel_cmdline,
                      const char *initrd_filename,
                      const char *cpu_model)
{
    struct milestone_s *s = qemu_mallocz(sizeof(*s));
    DriveInfo *dmtd = drive_get(IF_MTD, 0, 0);
    DriveInfo *dsd  = drive_get(IF_SD, 0, 0);

    if (!dmtd && !dsd) {
        hw_error("%s: SD or NAND image required", __FUNCTION__);
    }
#if MAX_SERIAL_PORTS < 3
#error MAX_SERIAL_PORTS must be at least 3!
#endif
    s->cpu = omap3_mpu_init(omap3430, 1, MILESTONE_SDRAM_SIZE,
                            serial_hds[1], serial_hds[2],
                            serial_hds[0], NULL);
    omap_lcd_panel_attach(s->cpu->dss);

    s->tsc2005 = spi_create_device(omap_mcspi_bus(s->cpu->mcspi, 0),
                                   "tsc2005", 0);
    qdev_connect_gpio_out(s->tsc2005, 0,
                          qdev_get_gpio_in(s->cpu->gpio,
                                           MILESTONE_TSC2005_IRQ_GPIO));
    tsc2005_set_transform(s->tsc2005, &milestone_pointercal, 600, 1500);
    cursor_hide = 0; // who wants to use touchscreen without a pointer?
    cursor_allow_grab = 0; // ...and please, don't stop the host cursor

    s->motlcd = spi_create_device_noinit(omap_mcspi_bus(s->cpu->mcspi, 0),
                                        "lcd_milestone", 2);
    qdev_prop_set_uint32(s->motlcd, "id", 0x101234);
    qdev_prop_set_uint8(s->motlcd, "milestone", 1);
    qdev_init_nofail(s->motlcd);

    s->nand = nand_init(NAND_MFR_HYNIX, 0xba, dmtd ? dmtd->bdrv : NULL);
	nand_setpins(s->nand, 0, 0, 0, 1, 0); /* no write-protect */
	omap_gpmc_attach(s->cpu->gpmc, MILESTONE_NAND_CS, s->nand, 0, 2);

    if (dsd) {
        omap3_mmc_attach(s->cpu->omap3_mmc[1], dsd->bdrv, 0, 1);
    }
    if ((dsd = drive_get(IF_SD, 0, 1)) != NULL) {
        omap3_mmc_attach(s->cpu->omap3_mmc[0], dsd->bdrv, 0, 0);
        //qemu_irq_raise(omap2_gpio_in_get(s->cpu->gpif, MILESTONE_SDCOVER_GPIO));
    }

    cpu_register_physical_memory(0x48058000, 0x3c00,
                                 cpu_register_io_memory(ssi_read_func,
                                                        ssi_write_func, 0,
                                                        DEVICE_NATIVE_ENDIAN));

    s->twl4030 = twl4030_init(omap_i2c_bus(s->cpu->i2c, 0),
                              s->cpu->irq[0][OMAP_INT_3XXX_SYS_NIRQ],
                              NULL, milestone_twl4030_keymap);
    twl4030_madc_attach(s->twl4030, milestone_twl4030_madc_callback);
    i2c_bus *i2c2 = omap_i2c_bus(s->cpu->i2c, 1);
    s->bq2415x = i2c_create_slave(i2c2, "bq2415x", 0x6b);
    s->tpa6130 = i2c_create_slave(i2c2, "tpa6130", 0x60);
    qdev_connect_gpio_out(s->cpu->gpio, MILESTONE_HEADPHONE_EN_GPIO,
                          qdev_get_gpio_in(s->tpa6130, 0));
	i2c_bus *i2c3 = omap_i2c_bus(s->cpu->i2c, 2);
    s->lis331dlh = i2c_create_slave(i2c3, "lis331dlh", 0x1d);
    qdev_connect_gpio_out(s->lis331dlh, 0,
                          qdev_get_gpio_in(s->cpu->gpio,
                                           MILESTONE_LIS331DLH_INT1_GPIO));
    qdev_connect_gpio_out(s->lis331dlh, 1,
                          qdev_get_gpio_in(s->cpu->gpio,
                                           MILESTONE_LIS331DLH_INT2_GPIO));
    
    int i;
    for (i = 0; i < nb_nics; i++) {
        if (!nd_table[i].model || !strcmp(nd_table[i].model, "smc91c111")) {
            break;
        }
    }
    if (i < nb_nics) {
        s->smc = qdev_create(NULL, "smc91c111");
        qdev_set_nic_properties(s->smc, &nd_table[i]);
        qdev_init_nofail(s->smc);
        sysbus_connect_irq(sysbus_from_qdev(s->smc), 0,
                           qdev_get_gpio_in(s->cpu->gpio, 54));
        omap_gpmc_attach(s->cpu->gpmc, 1, s->smc, 0, 0);
    } else {
        hw_error("%s: no NIC for smc91c111\n", __FUNCTION__);
    }
    qemu_add_kbd_event_handler(milestone_key_handler, s);
    qemu_set_display_close_handler(milestone_display_close_callback, s);

#ifdef CONFIG_GLES2
    s->gles2 = gles2_init(s->cpu->env);
#endif

    s->slide_open = 1;
    s->camera_cover_open = 0;
    s->headphone_connected = 0;
    
#ifdef CONFIG_SKINNING
    qemu_skin_add_switchstate_callback((switchstate_callback *)milestone_switchstate_callback, s);
#endif
	qemu_register_reset(milestone_reset, s);
}

static QEMUMachine milestone_machine = {
    .name = "milestone",
    .desc = "Motorola Milestone (OMAP3)",
    .init = milestone_init,
};

static void motodroid_register_devices(void)
{
    i2c_register_slave(&bq2415x_info);
    i2c_register_slave(&tpa6130_info);
	i2c_register_slave(&lis331dlh_info);
	spi_register_device(&motlcd_info);
}

static void motodroid_machine_init(void)
{
    qemu_register_machine(&milestone_machine);
}

device_init(motodroid_register_devices);
machine_init(motodroid_machine_init);

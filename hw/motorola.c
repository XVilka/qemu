/*
 * Motorola Droid, Milestone phones.
 *
 * Copyright (C) 2007 Nokia Corporation
 * Copyright (C) Anton Kochkov <anton.kochkov@gmail.com>
 * Written by Andrzej Zaborowski <andrew@openedhand.com>
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

#define MILESTONE_POWEROFF_GPIO      176
#define MILESTONE_ONENAND_GPIO       65
#define MILESTONE_CAMFOCUS_GPIO      68
#define MILESTONE_CAMLAUNCH_GPIO     69
#define MILESTONE_SLIDE_GPIO         71
#define MILESTONE_HEADPHONE_EN_GPIO  98
#define MILESTONE_CAMCOVER_GPIO      110
#define MILESTONE_KBLOCK_GPIO        113
#define MILESTONE_HEADPHONE_GPIO     177
/* Touch screen */
#define MILESTONE_ATMEGA324P_IRQ_GPIO   99
#define MILESTONE_ATMEGA324P_RESET_GPIO 164
/* Accelerometer */
#define MILESTONE_LIS331DLH_INT2_GPIO 180
#define MILESTONE_LIS331DLH_INT1_GPIO 181
/* Magnetometer */
#define MILESTONE_AKM8973_INT_GPIO	  175
#define MILESTONE_AKM8973_RESET_GPIO  28
/* Proximity sensor */
#define MILESTONE_SHM7743_INT_GPIO    180

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

struct milestone_s {
    struct omap_mpu_state_s *cpu;
    void *twl4030;
    DeviceState *nand;
    DeviceState *motlcd;
    DeviceState *atmega324p;
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
    /* Init HIGH SECURITY mode */
	s->cpu = omap3_mpu_init(omap3430, 1, MILESTONE_SDRAM_SIZE,
                            serial_hds[1], serial_hds[2],
                            serial_hds[0], NULL, 1);

	/* Initialize LCD Panel */
    omap_lcd_panel_attach(s->cpu->dss);

    s->atmega324p = spi_create_device(omap_mcspi_bus(s->cpu->mcspi, 0),
                                   "atmega324p", 0);
    qdev_connect_gpio_out(s->atmega324p, 0,
                          qdev_get_gpio_in(s->cpu->gpio,
                                           MILESTONE_ATMEGA324P_IRQ_GPIO));
    atmega324p_set_transform(s->atmega324p, &milestone_pointercal, 600, 1500);
    cursor_hide = 0; // who wants to use touchscreen without a pointer?
    cursor_allow_grab = 0; // ...and please, don't stop the host cursor

    s->motlcd = spi_create_device_noinit(omap_mcspi_bus(s->cpu->mcspi, 0),
                                        "lcd_milestone", 2);
    qdev_prop_set_uint32(s->motlcd, "id", 0x101234);
    qdev_prop_set_uint8(s->motlcd, "milestone", 1);
    qdev_init_nofail(s->motlcd);

    /* Initialize NAND */
	s->nand = nand_init(NAND_MFR_TOSHIBA, 0xbc, dmtd ? dmtd->bdrv : NULL);
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
	spi_register_device(&motlcd_info);
}

static void motodroid_machine_init(void)
{
    qemu_register_machine(&milestone_machine);
}

device_init(motodroid_register_devices);
machine_init(motodroid_machine_init);

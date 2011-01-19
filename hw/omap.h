/*
 * Texas Instruments OMAP processors.
 *
 * Copyright (C) 2006-2008 Andrzej Zaborowski  <balrog@zabor.org>
 * Copyright (c) 2009 Nokia Corporation
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
#ifndef hw_omap_h
# define hw_omap_h		"omap.h"

#include "sysemu.h"

# define OMAP_EMIFS_BASE	0x00000000
# define OMAP2_Q0_BASE		0x00000000
# define OMAP_CS0_BASE		0x00000000
# define OMAP_CS1_BASE		0x04000000
# define OMAP_CS2_BASE		0x08000000
# define OMAP_CS3_BASE		0x0c000000
# define OMAP_EMIFF_BASE	0x10000000
# define OMAP_IMIF_BASE		0x20000000
# define OMAP_LOCALBUS_BASE	0x30000000
# define OMAP2_Q1_BASE		0x40000000
# define OMAP2_L4_BASE		0x48000000
# define OMAP2_SRAM_BASE	0x40200000
# define OMAP2_L3_BASE		0x68000000
# define OMAP2_Q2_BASE		0x80000000
# define OMAP2_Q3_BASE		0xc0000000
# define OMAP3_Q1_BASE      0x40000000
# define OMAP3_L4_BASE      0x48000000
# define OMAP3_SRAM_BASE    0x40200000
# define OMAP3_L3_BASE      0x68000000
# define OMAP3_Q2_BASE      0x80000000
# define OMAP3_Q3_BASE      0xc0000000
# define OMAP_MPUI_BASE		0xe1000000

# define OMAP730_SRAM_SIZE      0x00032000
# define OMAP15XX_SRAM_SIZE     0x00030000
# define OMAP16XX_SRAM_SIZE     0x00004000
# define OMAP1611_SRAM_SIZE     0x0003e800
# define OMAP242X_SRAM_SIZE     0x000a0000
# define OMAP243X_SRAM_SIZE     0x00010000
# define OMAP3XXX_SRAM_SIZE     0x00010000
# define OMAP3XXX_BOOTROM_SIZE  0x00008000
# define OMAP_CS0_SIZE          0x04000000
# define OMAP_CS1_SIZE          0x04000000
# define OMAP_CS2_SIZE          0x04000000
# define OMAP_CS3_SIZE          0x04000000

/* omap_clk.c */
struct omap_mpu_state_s;
typedef struct clk *omap_clk;
omap_clk omap_findclk(struct omap_mpu_state_s *mpu, const char *name);
void omap_clk_init(struct omap_mpu_state_s *mpu);
void omap_clk_adduser(struct clk *clk, qemu_irq user);
void omap_clk_get(omap_clk clk);
void omap_clk_put(omap_clk clk);
void omap_clk_onoff(omap_clk clk, int on);
void omap_clk_canidle(omap_clk clk, int can);
void omap_clk_setrate(omap_clk clk, int divide, int multiply);
int64_t omap_clk_getrate(omap_clk clk);
void omap_clk_reparent(omap_clk clk, omap_clk parent);

/* omap_intc.c */
struct omap_intr_handler_s;
struct omap_intr_handler_s *omap_inth_init(target_phys_addr_t base,
                                           unsigned long size,
                                           unsigned char nbanks,
                                           qemu_irq **pins,
                                           qemu_irq parent_irq,
                                           qemu_irq parent_fiq,
                                           omap_clk clk);
struct omap_intr_handler_s *omap2_inth_init(struct omap_mpu_state_s *mpu,
                                            target_phys_addr_t base,
                                            int size, int nbanks,
                                            qemu_irq **pins,
                                            qemu_irq parent_irq,
                                            qemu_irq parent_fiq,
                                            omap_clk fclk, omap_clk iclk);
void omap_inth_reset(struct omap_intr_handler_s *s);
qemu_irq omap_inth_get_pin(struct omap_intr_handler_s *s, int n);

/* omap_sdrc.c */
struct omap_sdrc_s;
struct omap_sdrc_s *omap_sdrc_init(target_phys_addr_t base);
void omap_sdrc_reset(struct omap_sdrc_s *s);
void omap_sdrc_write_mcfg(struct omap_sdrc_s *s, uint32_t value, uint32_t cs);

/* omap_gpmc.c */
struct omap_gpmc_s;
struct nand_flash_s;
struct omap_gpmc_s *omap_gpmc_init(struct omap_mpu_state_s *mpu,
                                   target_phys_addr_t base, qemu_irq irq);
void omap_gpmc_reset(struct omap_gpmc_s *s);
void omap_gpmc_attach(struct omap_gpmc_s *s, int cs, int iomemtype,
                      void (*base_upd)(void *opaque, target_phys_addr_t new),
                      void (*unmap)(void *opaque), void *opaque,
                      int devicetype);

/* omap_l4.c */
struct omap_l4_s;
struct omap_l4_region_s;
typedef enum {
    L4TYPE_GENERIC = 0, /* not mapped by default, must be mapped separately */
    L4TYPE_IA,          /* initiator agent */
    L4TYPE_TA,          /* target agent */
    L4TYPE_LA,          /* link register agent */
    L4TYPE_AP           /* address protection */
} omap3_l4_region_type_t;
struct omap2_l4_agent_info_s {
    int ta;
    int region;
    int regions;
    int ta_region;
};
struct omap3_l4_agent_info_s {
    int agent_id;
    int first_region_id;
    int region_count;
};
struct omap_target_agent_s {
    struct omap_l4_s *bus;
    int regions;
    const struct omap_l4_region_s *start;
    target_phys_addr_t base;
    uint32_t component;
    uint32_t control;
    uint32_t control_h; /* OMAP3 only */
    uint32_t status;
};
struct omap_l4_region_s {
    target_phys_addr_t offset;
    size_t size;
    int access; /* omap3_l4_region_type_t for OMAP3 */
};
struct omap_l4_s *omap_l4_init(target_phys_addr_t base, int ta_num,
                               int region_count);
struct omap_target_agent_s *omap2_l4ta_init(
    struct omap_l4_s *bus,
    const struct omap_l4_region_s *regions,
    const struct omap2_l4_agent_info_s *agents,
    int cs);
struct omap_target_agent_s *omap3_l4ta_init(
    struct omap_l4_s *bus,
    const struct omap_l4_region_s *regions,
    const struct omap3_l4_agent_info_s *agents,
    int cs);
target_phys_addr_t omap_l4_attach(struct omap_target_agent_s *ta, int region,
                                  int iotype);
target_phys_addr_t omap_l4_base(struct omap_target_agent_s *ta, int region);
uint32_t omap_l4_size(struct omap_target_agent_s *ta, int region);
int l4_register_io_memory(CPUReadMemoryFunc * const *mem_read,
                          CPUWriteMemoryFunc * const *mem_write,
                          void *opaque);

/*
 * Common IRQ numbers for level 1 interrupt handler
 * See /usr/include/asm-arm/arch-omap/irqs.h in Linux.
 */
# define OMAP_INT_CAMERA		1
# define OMAP_INT_FIQ			3
# define OMAP_INT_RTDX			6
# define OMAP_INT_DSP_MMU_ABORT		7
# define OMAP_INT_HOST			8
# define OMAP_INT_ABORT			9
# define OMAP_INT_BRIDGE_PRIV		13
# define OMAP_INT_GPIO_BANK1		14
# define OMAP_INT_UART3			15
# define OMAP_INT_TIMER3		16
# define OMAP_INT_DMA_CH0_6		19
# define OMAP_INT_DMA_CH1_7		20
# define OMAP_INT_DMA_CH2_8		21
# define OMAP_INT_DMA_CH3		22
# define OMAP_INT_DMA_CH4		23
# define OMAP_INT_DMA_CH5		24
# define OMAP_INT_DMA_LCD		25
# define OMAP_INT_TIMER1		26
# define OMAP_INT_WD_TIMER		27
# define OMAP_INT_BRIDGE_PUB		28
# define OMAP_INT_TIMER2		30
# define OMAP_INT_LCD_CTRL		31

/*
 * Common OMAP-15xx IRQ numbers for level 1 interrupt handler
 */
# define OMAP_INT_15XX_IH2_IRQ		0
# define OMAP_INT_15XX_LB_MMU		17
# define OMAP_INT_15XX_LOCAL_BUS	29

/*
 * OMAP-1510 specific IRQ numbers for level 1 interrupt handler
 */
# define OMAP_INT_1510_SPI_TX		4
# define OMAP_INT_1510_SPI_RX		5
# define OMAP_INT_1510_DSP_MAILBOX1	10
# define OMAP_INT_1510_DSP_MAILBOX2	11

/*
 * OMAP-310 specific IRQ numbers for level 1 interrupt handler
 */
# define OMAP_INT_310_McBSP2_TX		4
# define OMAP_INT_310_McBSP2_RX		5
# define OMAP_INT_310_HSB_MAILBOX1	12
# define OMAP_INT_310_HSAB_MMU		18

/*
 * OMAP-1610 specific IRQ numbers for level 1 interrupt handler
 */
# define OMAP_INT_1610_IH2_IRQ		0
# define OMAP_INT_1610_IH2_FIQ		2
# define OMAP_INT_1610_McBSP2_TX	4
# define OMAP_INT_1610_McBSP2_RX	5
# define OMAP_INT_1610_DSP_MAILBOX1	10
# define OMAP_INT_1610_DSP_MAILBOX2	11
# define OMAP_INT_1610_LCD_LINE		12
# define OMAP_INT_1610_GPTIMER1		17
# define OMAP_INT_1610_GPTIMER2		18
# define OMAP_INT_1610_SSR_FIFO_0	29

/*
 * OMAP-730 specific IRQ numbers for level 1 interrupt handler
 */
# define OMAP_INT_730_IH2_FIQ		0
# define OMAP_INT_730_IH2_IRQ		1
# define OMAP_INT_730_USB_NON_ISO	2
# define OMAP_INT_730_USB_ISO		3
# define OMAP_INT_730_ICR		4
# define OMAP_INT_730_EAC		5
# define OMAP_INT_730_GPIO_BANK1	6
# define OMAP_INT_730_GPIO_BANK2	7
# define OMAP_INT_730_GPIO_BANK3	8
# define OMAP_INT_730_McBSP2TX		10
# define OMAP_INT_730_McBSP2RX		11
# define OMAP_INT_730_McBSP2RX_OVF	12
# define OMAP_INT_730_LCD_LINE		14
# define OMAP_INT_730_GSM_PROTECT	15
# define OMAP_INT_730_TIMER3		16
# define OMAP_INT_730_GPIO_BANK5	17
# define OMAP_INT_730_GPIO_BANK6	18
# define OMAP_INT_730_SPGIO_WR		29

/*
 * Common IRQ numbers for level 2 interrupt handler
 */
# define OMAP_INT_KEYBOARD		1
# define OMAP_INT_uWireTX		2
# define OMAP_INT_uWireRX		3
# define OMAP_INT_I2C			4
# define OMAP_INT_MPUIO			5
# define OMAP_INT_USB_HHC_1		6
# define OMAP_INT_McBSP3TX		10
# define OMAP_INT_McBSP3RX		11
# define OMAP_INT_McBSP1TX		12
# define OMAP_INT_McBSP1RX		13
# define OMAP_INT_UART1			14
# define OMAP_INT_UART2			15
# define OMAP_INT_USB_W2FC		20
# define OMAP_INT_1WIRE			21
# define OMAP_INT_OS_TIMER		22
# define OMAP_INT_OQN			23
# define OMAP_INT_GAUGE_32K		24
# define OMAP_INT_RTC_TIMER		25
# define OMAP_INT_RTC_ALARM		26
# define OMAP_INT_DSP_MMU		28

/*
 * OMAP-1510 specific IRQ numbers for level 2 interrupt handler
 */
# define OMAP_INT_1510_BT_MCSI1TX	16
# define OMAP_INT_1510_BT_MCSI1RX	17
# define OMAP_INT_1510_SoSSI_MATCH	19
# define OMAP_INT_1510_MEM_STICK	27
# define OMAP_INT_1510_COM_SPI_RO	31

/*
 * OMAP-310 specific IRQ numbers for level 2 interrupt handler
 */
# define OMAP_INT_310_FAC		0
# define OMAP_INT_310_USB_HHC_2		7
# define OMAP_INT_310_MCSI1_FE		16
# define OMAP_INT_310_MCSI2_FE		17
# define OMAP_INT_310_USB_W2FC_ISO	29
# define OMAP_INT_310_USB_W2FC_NON_ISO	30
# define OMAP_INT_310_McBSP2RX_OF	31

/*
 * OMAP-1610 specific IRQ numbers for level 2 interrupt handler
 */
# define OMAP_INT_1610_FAC		0
# define OMAP_INT_1610_USB_HHC_2	7
# define OMAP_INT_1610_USB_OTG		8
# define OMAP_INT_1610_SoSSI		9
# define OMAP_INT_1610_BT_MCSI1TX	16
# define OMAP_INT_1610_BT_MCSI1RX	17
# define OMAP_INT_1610_SoSSI_MATCH	19
# define OMAP_INT_1610_MEM_STICK	27
# define OMAP_INT_1610_McBSP2RX_OF	31
# define OMAP_INT_1610_STI		32
# define OMAP_INT_1610_STI_WAKEUP	33
# define OMAP_INT_1610_GPTIMER3		34
# define OMAP_INT_1610_GPTIMER4		35
# define OMAP_INT_1610_GPTIMER5		36
# define OMAP_INT_1610_GPTIMER6		37
# define OMAP_INT_1610_GPTIMER7		38
# define OMAP_INT_1610_GPTIMER8		39
# define OMAP_INT_1610_GPIO_BANK2	40
# define OMAP_INT_1610_GPIO_BANK3	41
# define OMAP_INT_1610_MMC2		42
# define OMAP_INT_1610_CF		43
# define OMAP_INT_1610_WAKE_UP_REQ	46
# define OMAP_INT_1610_GPIO_BANK4	48
# define OMAP_INT_1610_SPI		49
# define OMAP_INT_1610_DMA_CH6		53
# define OMAP_INT_1610_DMA_CH7		54
# define OMAP_INT_1610_DMA_CH8		55
# define OMAP_INT_1610_DMA_CH9		56
# define OMAP_INT_1610_DMA_CH10		57
# define OMAP_INT_1610_DMA_CH11		58
# define OMAP_INT_1610_DMA_CH12		59
# define OMAP_INT_1610_DMA_CH13		60
# define OMAP_INT_1610_DMA_CH14		61
# define OMAP_INT_1610_DMA_CH15		62
# define OMAP_INT_1610_NAND		63

/*
 * OMAP-730 specific IRQ numbers for level 2 interrupt handler
 */
# define OMAP_INT_730_HW_ERRORS		0
# define OMAP_INT_730_NFIQ_PWR_FAIL	1
# define OMAP_INT_730_CFCD		2
# define OMAP_INT_730_CFIREQ		3
# define OMAP_INT_730_I2C		4
# define OMAP_INT_730_PCC		5
# define OMAP_INT_730_MPU_EXT_NIRQ	6
# define OMAP_INT_730_SPI_100K_1	7
# define OMAP_INT_730_SYREN_SPI		8
# define OMAP_INT_730_VLYNQ		9
# define OMAP_INT_730_GPIO_BANK4	10
# define OMAP_INT_730_McBSP1TX		11
# define OMAP_INT_730_McBSP1RX		12
# define OMAP_INT_730_McBSP1RX_OF	13
# define OMAP_INT_730_UART_MODEM_IRDA_2	14
# define OMAP_INT_730_UART_MODEM_1	15
# define OMAP_INT_730_MCSI		16
# define OMAP_INT_730_uWireTX		17
# define OMAP_INT_730_uWireRX		18
# define OMAP_INT_730_SMC_CD		19
# define OMAP_INT_730_SMC_IREQ		20
# define OMAP_INT_730_HDQ_1WIRE		21
# define OMAP_INT_730_TIMER32K		22
# define OMAP_INT_730_MMC_SDIO		23
# define OMAP_INT_730_UPLD		24
# define OMAP_INT_730_USB_HHC_1		27
# define OMAP_INT_730_USB_HHC_2		28
# define OMAP_INT_730_USB_GENI		29
# define OMAP_INT_730_USB_OTG		30
# define OMAP_INT_730_CAMERA_IF		31
# define OMAP_INT_730_RNG		32
# define OMAP_INT_730_DUAL_MODE_TIMER	33
# define OMAP_INT_730_DBB_RF_EN		34
# define OMAP_INT_730_MPUIO_KEYPAD	35
# define OMAP_INT_730_SHA1_MD5		36
# define OMAP_INT_730_SPI_100K_2	37
# define OMAP_INT_730_RNG_IDLE		38
# define OMAP_INT_730_MPUIO		39
# define OMAP_INT_730_LLPC_LCD_CTRL_OFF	40
# define OMAP_INT_730_LLPC_OE_FALLING	41
# define OMAP_INT_730_LLPC_OE_RISING	42
# define OMAP_INT_730_LLPC_VSYNC	43
# define OMAP_INT_730_WAKE_UP_REQ	46
# define OMAP_INT_730_DMA_CH6		53
# define OMAP_INT_730_DMA_CH7		54
# define OMAP_INT_730_DMA_CH8		55
# define OMAP_INT_730_DMA_CH9		56
# define OMAP_INT_730_DMA_CH10		57
# define OMAP_INT_730_DMA_CH11		58
# define OMAP_INT_730_DMA_CH12		59
# define OMAP_INT_730_DMA_CH13		60
# define OMAP_INT_730_DMA_CH14		61
# define OMAP_INT_730_DMA_CH15		62
# define OMAP_INT_730_NAND		63

/*
 * OMAP-24xx common IRQ numbers
 */
# define OMAP_INT_24XX_STI		4
# define OMAP_INT_24XX_SYS_NIRQ		7
# define OMAP_INT_24XX_L3_IRQ		10
# define OMAP_INT_24XX_PRCM_MPU_IRQ	11
# define OMAP_INT_24XX_SDMA_IRQ0	12
# define OMAP_INT_24XX_SDMA_IRQ1	13
# define OMAP_INT_24XX_SDMA_IRQ2	14
# define OMAP_INT_24XX_SDMA_IRQ3	15
# define OMAP_INT_243X_MCBSP2_IRQ	16
# define OMAP_INT_243X_MCBSP3_IRQ	17
# define OMAP_INT_243X_MCBSP4_IRQ	18
# define OMAP_INT_243X_MCBSP5_IRQ	19
# define OMAP_INT_24XX_GPMC_IRQ		20
# define OMAP_INT_24XX_GUFFAW_IRQ	21
# define OMAP_INT_24XX_IVA_IRQ		22
# define OMAP_INT_24XX_EAC_IRQ		23
# define OMAP_INT_24XX_CAM_IRQ		24
# define OMAP_INT_24XX_DSS_IRQ		25
# define OMAP_INT_24XX_MAIL_U0_MPU	26
# define OMAP_INT_24XX_DSP_UMA		27
# define OMAP_INT_24XX_DSP_MMU		28
# define OMAP_INT_24XX_GPIO_BANK1	29
# define OMAP_INT_24XX_GPIO_BANK2	30
# define OMAP_INT_24XX_GPIO_BANK3	31
# define OMAP_INT_24XX_GPIO_BANK4	32
# define OMAP_INT_243X_GPIO_BANK5	33
# define OMAP_INT_24XX_MAIL_U3_MPU	34
# define OMAP_INT_24XX_WDT3		35
# define OMAP_INT_24XX_WDT4		36
# define OMAP_INT_24XX_GPTIMER1		37
# define OMAP_INT_24XX_GPTIMER2		38
# define OMAP_INT_24XX_GPTIMER3		39
# define OMAP_INT_24XX_GPTIMER4		40
# define OMAP_INT_24XX_GPTIMER5		41
# define OMAP_INT_24XX_GPTIMER6		42
# define OMAP_INT_24XX_GPTIMER7		43
# define OMAP_INT_24XX_GPTIMER8		44
# define OMAP_INT_24XX_GPTIMER9		45
# define OMAP_INT_24XX_GPTIMER10	46
# define OMAP_INT_24XX_GPTIMER11	47
# define OMAP_INT_24XX_GPTIMER12	48
# define OMAP_INT_24XX_PKA_IRQ		50
# define OMAP_INT_24XX_SHA1MD5_IRQ	51
# define OMAP_INT_24XX_RNG_IRQ		52
# define OMAP_INT_24XX_MG_IRQ		53
# define OMAP_INT_24XX_I2C1_IRQ		56
# define OMAP_INT_24XX_I2C2_IRQ		57
# define OMAP_INT_24XX_MCBSP1_IRQ_TX	59
# define OMAP_INT_24XX_MCBSP1_IRQ_RX	60
# define OMAP_INT_24XX_MCBSP2_IRQ_TX	62
# define OMAP_INT_24XX_MCBSP2_IRQ_RX	63
# define OMAP_INT_243X_MCBSP1_IRQ	64
# define OMAP_INT_24XX_MCSPI1_IRQ	65
# define OMAP_INT_24XX_MCSPI2_IRQ	66
# define OMAP_INT_24XX_SSI1_IRQ0	67
# define OMAP_INT_24XX_SSI1_IRQ1	68
# define OMAP_INT_24XX_SSI2_IRQ0	69
# define OMAP_INT_24XX_SSI2_IRQ1	70
# define OMAP_INT_24XX_SSI_GDD_IRQ	71
# define OMAP_INT_24XX_UART1_IRQ	72
# define OMAP_INT_24XX_UART2_IRQ	73
# define OMAP_INT_24XX_UART3_IRQ	74
# define OMAP_INT_24XX_USB_IRQ_GEN	75
# define OMAP_INT_24XX_USB_IRQ_NISO	76
# define OMAP_INT_24XX_USB_IRQ_ISO	77
# define OMAP_INT_24XX_USB_IRQ_HGEN	78
# define OMAP_INT_24XX_USB_IRQ_HSOF	79
# define OMAP_INT_24XX_USB_IRQ_OTG	80
# define OMAP_INT_24XX_VLYNQ_IRQ	81
# define OMAP_INT_24XX_MMC_IRQ		83
# define OMAP_INT_24XX_MS_IRQ		84
# define OMAP_INT_24XX_FAC_IRQ		85
# define OMAP_INT_24XX_MCSPI3_IRQ	91
# define OMAP_INT_243X_HS_USB_MC	92
# define OMAP_INT_243X_HS_USB_DMA	93
# define OMAP_INT_243X_CARKIT		94
# define OMAP_INT_34XX_GPTIMER12	95

/*
 * OMAP-3XXX common IRQ numbers
 */
#define OMAP_INT_3XXX_EMUINT        0  /* MPU emulation */
#define OMAP_INT_3XXX_COMMTX        1  /* MPU emulation */
#define OMAP_INT_3XXX_COMMRX        2  /* MPU emulation */
#define OMAP_INT_3XXX_BENCH         3  /* MPU emulation */
#define OMAP_INT_3XXX_MCBSP2_ST_IRQ 4  /* Sidetone MCBSP2 overflow */
#define OMAP_INT_3XXX_MCBSP3_ST_IRQ 5  /* Sidetone MCBSP3 overflow */
#define OMAP_INT_3XXX_SSM_ABORT_IRQ 6
#define OMAP_INT_3XXX_SYS_NIRQ      7  /* External source (active low) */
#define OMAP_INT_3XXX_D2D_FW_IRQ    8
#define OMAP_INT_3XXX_SMX_DBG_IRQ   9  /* L3 interconnect error for debug */
#define OMAP_INT_3XXX_SMX_APP_IRQ   10 /* L3 interconnect error for application */
#define OMAP_INT_3XXX_PRCM_MPU_IRQ  11 /* PRCM module IRQ */
#define OMAP_INT_3XXX_SDMA_IRQ0     12 /* System DMA request 0 */
#define OMAP_INT_3XXX_SDMA_IRQ1     13 /* System DMA request 1 */
#define OMAP_INT_3XXX_SDMA_IRQ2     14 /* System DMA request 2 */
#define OMAP_INT_3XXX_SDMA_IRQ3     15 /* System DMA request 3 */
#define OMAP_INT_3XXX_MCBSP1_IRQ    16 /* MCBSP module 1 IRQ */
#define OMAP_INT_3XXX_MCBSP2_IRQ    17 /* MCBSP module 2 IRQ */
#define OMAP_INT_3XXX_SR1_IRQ       18 /* SmartReflex 1 */
#define OMAP_INT_3XXX_SR2_IRQ       19 /* SmartReflex 2 */
#define OMAP_INT_3XXX_GPMC_IRQ      20 /* General-purpose memory controller module */
#define OMAP_INT_3XXX_SGX_IRQ       21 /* 2D/3D graphics module */
#define OMAP_INT_3XXX_MCBSP3_IRQ    22 /* MCBSP module 3 */
#define OMAP_INT_3XXX_MCBSP4_IRQ    23 /* MCBSP module 4 */
#define OMAP_INT_3XXX_CAM_IRQ0      24 /* Camera interface request 0 */
#define OMAP_INT_3XXX_DSS_IRQ       25 /* Display subsystem module */
#define OMAP_INT_3XXX_MAIL_U0_MPU   26 /* Mailbox user 0 request */
#define OMAP_INT_3XXX_MCBSP5_IRQ    27 /* MCBSP module 5 */
#define OMAP_INT_3XXX_IVA2_MMU_IRQ  28 /* IVA2 MMU */
#define OMAP_INT_3XXX_GPIO1_MPU_IRQ 29 /* GPIO module 1 */
#define OMAP_INT_3XXX_GPIO2_MPU_IRQ 30 /* GPIO module 2 */
#define OMAP_INT_3XXX_GPIO3_MPU_IRQ 31 /* GPIO module 3 */
#define OMAP_INT_3XXX_GPIO4_MPU_IRQ 32 /* GPIO module 4 */
#define OMAP_INT_3XXX_GPIO5_MPU_IRQ 33 /* GPIO module 5 */
#define OMAP_INT_3XXX_GPIO6_MPU_IRQ 34 /* GPIO module 6 */
#define OMAP_INT_3XXX_USIM_IRQ      35
#define OMAP_INT_3XXX_WDT3_IRQ      36 /* Watchdog timer module 3 overflow */
#define OMAP_INT_3XXX_GPT1_IRQ      37 /* General-purpose timer module 1 */
#define OMAP_INT_3XXX_GPT2_IRQ      38 /* General-purpose timer module 2 */
#define OMAP_INT_3XXX_GPT3_IRQ      39 /* General-purpose timer module 3 */
#define OMAP_INT_3XXX_GPT4_IRQ      40 /* General-purpose timer module 4 */
#define OMAP_INT_3XXX_GPT5_IRQ      41 /* General-purpose timer module 5 */
#define OMAP_INT_3XXX_GPT6_IRQ      42 /* General-purpose timer module 6 */
#define OMAP_INT_3XXX_GPT7_IRQ      43 /* General-purpose timer module 7 */
#define OMAP_INT_3XXX_GPT8_IRQ      44 /* General-purpose timer module 8 */
#define OMAP_INT_3XXX_GPT9_IRQ      45 /* General-purpose timer module 9 */
#define OMAP_INT_3XXX_GPT10_IRQ     46 /* General-purpose timer module 10 */
#define OMAP_INT_3XXX_GPT11_IRQ     47 /* General-purpose timer module 11 */
#define OMAP_INT_3XXX_MCSPI4_IRQ    48 /* MCSPI module 4 */
#define OMAP_INT_3XXX_SHA1MD52_IRQ  49
#define OMAP_INT_3XXX_FPKA_READY    50
#define OMAP_INT_3XXX_SHA1MD51_IRQ  51
#define OMAP_INT_3XXX_RNG_IRQ       52
#define OMAP_INT_3XXX_MG_IRQ        53
#define OMAP_INT_3XXX_MCBSP4_IRQ_TX 54 /* MCBSP module 4 transmit */
#define OMAP_INT_3XXX_MCBSP4_IRQ_RX 55 /* MCBSP module 4 receive */
#define OMAP_INT_3XXX_I2C1_IRQ      56 /* I2C module 1 */
#define OMAP_INT_3XXX_I2C2_IRQ      57 /* I2C module 2 */
#define OMAP_INT_3XXX_HDQ_IRQ       58 /* HDQ/1-Wire */
#define OMAP_INT_3XXX_MCBSP1_IRQ_TX 59 /* MCBSP module 1 transmit */
#define OMAP_INT_3XXX_MCBSP1_IRQ_RX 60 /* MCBSP module 1 receive */
#define OMAP_INT_3XXX_I2C3_IRQ      61 /* I2C module 3 */
#define OMAP_INT_3XXX_MCBSP2_IRQ_TX 62 /* MCBSP module 2 transmit */
#define OMAP_INT_3XXX_MCBSP2_IRQ_RX 63 /* MCBSP module 2 receive */
#define OMAP_INT_3XXX_FPKA_ERROR    64
#define OMAP_INT_3XXX_MCSPI1_IRQ    65 /* MCSPI module 1 */
#define OMAP_INT_3XXX_MCSPI2_IRQ    66 /* MCSPI module 2 */
/* IRQ67 is reserved */
/* IRQ68 is reserved */
/* IRQ69 is reserved */
/* IRQ70 is reserved */
/* IRQ71 is reserved */
#define OMAP_INT_3XXX_UART1_IRQ     72 /* UART module 1 */
#define OMAP_INT_3XXX_UART2_IRQ     73 /* UART module 2 */
#define OMAP_INT_3XXX_UART3_IRQ     74 /* UART module 3 (also infrared)*/
#define OMAP_INT_3XXX_PBIAS_IRQ     75 /* Merged interrupt for PBIASlite1 and 2 */
#define OMAP_INT_3XXX_OHCI_IRQ      76 /* OHCI controller HSUSB MP Host interrupt */
#define OMAP_INT_3XXX_EHCI_IRQ      77 /* EHCI controller HSUSB MP Host interrupt */
#define OMAP_INT_3XXX_TLL_IRQ       78 /* HSUSB MP TLL interrupt */
/* IRQ79 is reserved */
#define OMAP_INT_3XXX_UART4_IRQ     80 /* UART module 4 (OMAP3630 only) */
#define OMAP_INT_3XXX_MCBSP5_IRQ_TX 81 /* MCBSP module 5 transmit */
#define OMAP_INT_3XXX_MCBSP5_IRQ_RX 82 /* MCBSP module 5 receive */
#define OMAP_INT_3XXX_MMC1_IRQ      83 /* MMC/SD module 1 */
#define OMAP_INT_3XXX_MS_IRQ		84
/* IRQ85 is reserved */
#define OMAP_INT_3XXX_MMC2_IRQ		86 /* MMC/SD module 2 */
#define OMAP_INT_3XXX_MPU_ICR_IRQ   87 /* MPU ICR */
#define OMAP_INT_3XXX_D2DFRINT      88 /* 3G coprocessor */
#define OMAP_INT_3XXX_MCBSP3_IRQ_TX 89 /* MCBSP module 3 transmit */
#define OMAP_INT_3XXX_MCBSP3_IRQ_RX 90 /* MCBSP module 3 receive */
#define OMAP_INT_3XXX_MCSPI3_IRQ    91 /* MCSPI module 3 */
#define OMAP_INT_3XXX_HSUSB_MC      92 /* High-Speed USB OTG controller */
#define OMAP_INT_3XXX_HSUSB_DMA     93 /* High-Speed USB OTG DMA controller */
#define OMAP_INT_3XXX_MMC3_IRQ      94 /* MMC/SD module 3 */
#define OMAP_INT_3XXX_GPT12_IRQ     95 /* General-purpose timer module 12 */

/* omap_dma.c */
enum omap_dma_model {
    omap_dma_3_0,
    omap_dma_3_1,
    omap_dma_3_2,
    omap_dma_4,
};

struct soc_dma_s;
struct soc_dma_s *omap_dma_init(target_phys_addr_t base, qemu_irq *irqs,
                qemu_irq lcd_irq, struct omap_mpu_state_s *mpu, omap_clk clk,
                enum omap_dma_model model);
struct soc_dma_s *omap_dma4_init(target_phys_addr_t base, qemu_irq *irqs,
                struct omap_mpu_state_s *mpu, int fifo,
                int chans, omap_clk iclk, omap_clk fclk);
struct soc_dma_s *omap3_dma4_init(struct omap_target_agent_s *ta,
                                  struct omap_mpu_state_s *mpu,
                                  qemu_irq *irqs, int chans,
                                  omap_clk iclk, omap_clk fclk);
void omap_dma_reset(struct soc_dma_s *s);

struct dma_irq_map {
    int ih;
    int intr;
};

/* Only used in OMAP DMA 3.x gigacells */
enum omap_dma_port {
    emiff = 0,
    emifs,
    imif,	/* omap16xx: ocp_t1 */
    tipb,
    local,	/* omap16xx: ocp_t2 */
    tipb_mpui,
    __omap_dma_port_last,
};

typedef enum {
    constant = 0,
    post_incremented,
    single_index,
    double_index,
} omap_dma_addressing_t;

/* Only used in OMAP DMA 3.x gigacells */
struct omap_dma_lcd_channel_s {
    enum omap_dma_port src;
    target_phys_addr_t src_f1_top;
    target_phys_addr_t src_f1_bottom;
    target_phys_addr_t src_f2_top;
    target_phys_addr_t src_f2_bottom;

    /* Used in OMAP DMA 3.2 gigacell */
    unsigned char brust_f1;
    unsigned char pack_f1;
    unsigned char data_type_f1;
    unsigned char brust_f2;
    unsigned char pack_f2;
    unsigned char data_type_f2;
    unsigned char end_prog;
    unsigned char repeat;
    unsigned char auto_init;
    unsigned char priority;
    unsigned char fs;
    unsigned char running;
    unsigned char bs;
    unsigned char omap_3_1_compatible_disable;
    unsigned char dst;
    unsigned char lch_type;
    int16_t element_index_f1;
    int16_t element_index_f2;
    int32_t frame_index_f1;
    int32_t frame_index_f2;
    uint16_t elements_f1;
    uint16_t frames_f1;
    uint16_t elements_f2;
    uint16_t frames_f2;
    omap_dma_addressing_t mode_f1;
    omap_dma_addressing_t mode_f2;

    /* Destination port is fixed.  */
    int interrupts;
    int condition;
    int dual;

    int current_frame;
    target_phys_addr_t phys_framebuffer[2];
    qemu_irq irq;
    struct omap_mpu_state_s *mpu;
} *omap_dma_get_lcdch(struct soc_dma_s *s);

/*
 * DMA request numbers for OMAP1
 * See /usr/include/asm-arm/arch-omap/dma.h in Linux.
 */
# define OMAP_DMA_NO_DEVICE		0
# define OMAP_DMA_MCSI1_TX		1
# define OMAP_DMA_MCSI1_RX		2
# define OMAP_DMA_I2C_RX		3
# define OMAP_DMA_I2C_TX		4
# define OMAP_DMA_EXT_NDMA_REQ0		5
# define OMAP_DMA_EXT_NDMA_REQ1		6
# define OMAP_DMA_UWIRE_TX		7
# define OMAP_DMA_MCBSP1_TX		8
# define OMAP_DMA_MCBSP1_RX		9
# define OMAP_DMA_MCBSP3_TX		10
# define OMAP_DMA_MCBSP3_RX		11
# define OMAP_DMA_UART1_TX		12
# define OMAP_DMA_UART1_RX		13
# define OMAP_DMA_UART2_TX		14
# define OMAP_DMA_UART2_RX		15
# define OMAP_DMA_MCBSP2_TX		16
# define OMAP_DMA_MCBSP2_RX		17
# define OMAP_DMA_UART3_TX		18
# define OMAP_DMA_UART3_RX		19
# define OMAP_DMA_CAMERA_IF_RX		20
# define OMAP_DMA_MMC_TX		21
# define OMAP_DMA_MMC_RX		22
# define OMAP_DMA_NAND			23	/* Not in OMAP310 */
# define OMAP_DMA_IRQ_LCD_LINE		24	/* Not in OMAP310 */
# define OMAP_DMA_MEMORY_STICK		25	/* Not in OMAP310 */
# define OMAP_DMA_USB_W2FC_RX0		26
# define OMAP_DMA_USB_W2FC_RX1		27
# define OMAP_DMA_USB_W2FC_RX2		28
# define OMAP_DMA_USB_W2FC_TX0		29
# define OMAP_DMA_USB_W2FC_TX1		30
# define OMAP_DMA_USB_W2FC_TX2		31

/* These are only for 1610 */
# define OMAP_DMA_CRYPTO_DES_IN		32
# define OMAP_DMA_SPI_TX		33
# define OMAP_DMA_SPI_RX		34
# define OMAP_DMA_CRYPTO_HASH		35
# define OMAP_DMA_CCP_ATTN		36
# define OMAP_DMA_CCP_FIFO_NOT_EMPTY	37
# define OMAP_DMA_CMT_APE_TX_CHAN_0	38
# define OMAP_DMA_CMT_APE_RV_CHAN_0	39
# define OMAP_DMA_CMT_APE_TX_CHAN_1	40
# define OMAP_DMA_CMT_APE_RV_CHAN_1	41
# define OMAP_DMA_CMT_APE_TX_CHAN_2	42
# define OMAP_DMA_CMT_APE_RV_CHAN_2	43
# define OMAP_DMA_CMT_APE_TX_CHAN_3	44
# define OMAP_DMA_CMT_APE_RV_CHAN_3	45
# define OMAP_DMA_CMT_APE_TX_CHAN_4	46
# define OMAP_DMA_CMT_APE_RV_CHAN_4	47
# define OMAP_DMA_CMT_APE_TX_CHAN_5	48
# define OMAP_DMA_CMT_APE_RV_CHAN_5	49
# define OMAP_DMA_CMT_APE_TX_CHAN_6	50
# define OMAP_DMA_CMT_APE_RV_CHAN_6	51
# define OMAP_DMA_CMT_APE_TX_CHAN_7	52
# define OMAP_DMA_CMT_APE_RV_CHAN_7	53
# define OMAP_DMA_MMC2_TX		54
# define OMAP_DMA_MMC2_RX		55
# define OMAP_DMA_CRYPTO_DES_OUT	56

/*
 * DMA request numbers for the OMAP2
 */
# define OMAP24XX_DMA_NO_DEVICE		0
# define OMAP24XX_DMA_XTI_DMA		1	/* Not in OMAP2420 */
# define OMAP24XX_DMA_EXT_DMAREQ0	2
# define OMAP24XX_DMA_EXT_DMAREQ1	3
# define OMAP24XX_DMA_GPMC		4
# define OMAP24XX_DMA_GFX		5	/* Not in OMAP2420 */
# define OMAP24XX_DMA_DSS		6
# define OMAP24XX_DMA_VLYNQ_TX		7	/* Not in OMAP2420 */
# define OMAP24XX_DMA_CWT		8	/* Not in OMAP2420 */
# define OMAP24XX_DMA_AES_TX		9	/* Not in OMAP2420 */
# define OMAP24XX_DMA_AES_RX		10	/* Not in OMAP2420 */
# define OMAP24XX_DMA_DES_TX		11	/* Not in OMAP2420 */
# define OMAP24XX_DMA_DES_RX		12	/* Not in OMAP2420 */
# define OMAP24XX_DMA_SHA1MD5_RX	13	/* Not in OMAP2420 */
# define OMAP24XX_DMA_EXT_DMAREQ2	14
# define OMAP24XX_DMA_EXT_DMAREQ3	15
# define OMAP24XX_DMA_EXT_DMAREQ4	16
# define OMAP24XX_DMA_EAC_AC_RD		17
# define OMAP24XX_DMA_EAC_AC_WR		18
# define OMAP24XX_DMA_EAC_MD_UL_RD	19
# define OMAP24XX_DMA_EAC_MD_UL_WR	20
# define OMAP24XX_DMA_EAC_MD_DL_RD	21
# define OMAP24XX_DMA_EAC_MD_DL_WR	22
# define OMAP24XX_DMA_EAC_BT_UL_RD	23
# define OMAP24XX_DMA_EAC_BT_UL_WR	24
# define OMAP24XX_DMA_EAC_BT_DL_RD	25
# define OMAP24XX_DMA_EAC_BT_DL_WR	26
# define OMAP24XX_DMA_I2C1_TX		27
# define OMAP24XX_DMA_I2C1_RX		28
# define OMAP24XX_DMA_I2C2_TX		29
# define OMAP24XX_DMA_I2C2_RX		30
# define OMAP24XX_DMA_MCBSP1_TX		31
# define OMAP24XX_DMA_MCBSP1_RX		32
# define OMAP24XX_DMA_MCBSP2_TX		33
# define OMAP24XX_DMA_MCBSP2_RX		34
# define OMAP24XX_DMA_SPI1_TX0		35
# define OMAP24XX_DMA_SPI1_RX0		36
# define OMAP24XX_DMA_SPI1_TX1		37
# define OMAP24XX_DMA_SPI1_RX1		38
# define OMAP24XX_DMA_SPI1_TX2		39
# define OMAP24XX_DMA_SPI1_RX2		40
# define OMAP24XX_DMA_SPI1_TX3		41
# define OMAP24XX_DMA_SPI1_RX3		42
# define OMAP24XX_DMA_SPI2_TX0		43
# define OMAP24XX_DMA_SPI2_RX0		44
# define OMAP24XX_DMA_SPI2_TX1		45
# define OMAP24XX_DMA_SPI2_RX1		46

# define OMAP24XX_DMA_UART1_TX		49
# define OMAP24XX_DMA_UART1_RX		50
# define OMAP24XX_DMA_UART2_TX		51
# define OMAP24XX_DMA_UART2_RX		52
# define OMAP24XX_DMA_UART3_TX		53
# define OMAP24XX_DMA_UART3_RX		54
# define OMAP24XX_DMA_USB_W2FC_TX0	55
# define OMAP24XX_DMA_USB_W2FC_RX0	56
# define OMAP24XX_DMA_USB_W2FC_TX1	57
# define OMAP24XX_DMA_USB_W2FC_RX1	58
# define OMAP24XX_DMA_USB_W2FC_TX2	59
# define OMAP24XX_DMA_USB_W2FC_RX2	60
# define OMAP24XX_DMA_MMC1_TX		61
# define OMAP24XX_DMA_MMC1_RX		62
# define OMAP24XX_DMA_MS		63	/* Not in OMAP2420 */
# define OMAP24XX_DMA_EXT_DMAREQ5	64

/*
 * DMA request numbers for the OMAP3
 * Note that the numbers have to match the values that are
 * written to CCRi SYNCHRO_CONTROL bits, i.e. actual line
 * number plus one! Zero is a reserved value (defined as
 * NO_DEVICE here). Other missing values are reserved.
 */
#define OMAP3XXX_DMA_NO_DEVICE        0

#define OMAP3XXX_DMA_EXT_DMAREQ0      2
#define OMAP3XXX_DMA_EXT_DMAREQ1      3
#define OMAP3XXX_DMA_GPMC             4

#define OMAP3XXX_DMA_DSS_LINETRIGGER  6
#define OMAP3XXX_DMA_EXT_DMAREQ2      7

#define OMAP3XXX_DMA_SPI3_TX0         15
#define OMAP3XXX_DMA_SPI3_RX0         16
#define OMAP3XXX_DMA_MCBSP3_TX        17
#define OMAP3XXX_DMA_MCBSP3_RX        18
#define OMAP3XXX_DMA_MCBSP4_TX        19
#define OMAP3XXX_DMA_MCBSP4_RX        20
#define OMAP3XXX_DMA_MCBSP5_TX        21
#define OMAP3XXX_DMA_MCBSP5_RX        22
#define OMAP3XXX_DMA_SPI3_TX1         23
#define OMAP3XXX_DMA_SPI3_RX1         24
#define OMAP3XXX_DMA_I2C3_TX          25
#define OMAP3XXX_DMA_I2C3_RX          26
#define OMAP3XXX_DMA_I2C1_TX          27
#define OMAP3XXX_DMA_I2C1_RX          28
#define OMAP3XXX_DMA_I2C2_TX          29
#define OMAP3XXX_DMA_I2C2_RX          30
#define OMAP3XXX_DMA_MCBSP1_TX        31
#define OMAP3XXX_DMA_MCBSP1_RX        32
#define OMAP3XXX_DMA_MCBSP2_TX        33
#define OMAP3XXX_DMA_MCBSP2_RX        34
#define OMAP3XXX_DMA_SPI1_TX0         35
#define OMAP3XXX_DMA_SPI1_RX0         36
#define OMAP3XXX_DMA_SPI1_TX1         37
#define OMAP3XXX_DMA_SPI1_RX1         38
#define OMAP3XXX_DMA_SPI1_TX2         39
#define OMAP3XXX_DMA_SPI1_RX2         40
#define OMAP3XXX_DMA_SPI1_TX3         41
#define OMAP3XXX_DMA_SPI1_RX3         42
#define OMAP3XXX_DMA_SPI2_TX0         43
#define OMAP3XXX_DMA_SPI2_RX0         44
#define OMAP3XXX_DMA_SPI2_TX1         45
#define OMAP3XXX_DMA_SPI2_RX1         46
#define OMAP3XXX_DMA_MMC2_TX          47
#define OMAP3XXX_DMA_MMC2_RX          48
#define OMAP3XXX_DMA_UART1_TX         49
#define OMAP3XXX_DMA_UART1_RX         50
#define OMAP3XXX_DMA_UART2_TX         51
#define OMAP3XXX_DMA_UART2_RX         52
#define OMAP3XXX_DMA_UART3_TX         53
#define OMAP3XXX_DMA_UART3_RX         54

#define OMAP3XXX_DMA_MMC1_TX          61
#define OMAP3XXX_DMA_MMC1_RX          62
#define OMAP3XXX_DMA_MS               63
#define OMAP3XXX_DMA_EXT_DMAREQ3      64
#define OMAP3XXX_DMA_AES2_TX          65
#define OMAP3XXX_DMA_AES2_RX          66
#define OMAP3XXX_DMA_DES2_TX          67
#define OMAP3XXX_DMA_DES2_RX          68
#define OMAP3XXX_DMA_SHA1MD5_RX       69
#define OMAP3XXX_DMA_SPI4_TX0         70
#define OMAP3XXX_DMA_SPI4_RX0         71
#define OMAP3XXX_DMA_DSS0             72
#define OMAP3XXX_DMA_DSS1             73
#define OMAP3XXX_DMA_DSS2             74
#define OMAP3XXX_DMA_DSS3             75

#define OMAP3XXX_DMA_MMC3_TX          77
#define OMAP3XXX_DMA_MMC3_RX          78
#define OMAP3XXX_DMA_USIM_TX          79
#define OMAP3XXX_DMA_USIM_RX          80
#define OMAP3XXX_DMA_UART4_TX         81
#define OMAP3XXX_DMA_UART4_RX         82

/* omap_gpio.c */
struct omap_gpio_s;
struct omap_gpio_s *omap_gpio_init(target_phys_addr_t base,
                                   qemu_irq irq, omap_clk clk);
void omap_gpio_reset(struct omap_gpio_s *s);
qemu_irq *omap_gpio_in_get(struct omap_gpio_s *s);
void omap_gpio_out_set(struct omap_gpio_s *s, int line, qemu_irq handler);

struct omap_gpif_s;
struct omap_gpif_s *omap2_gpio_init(struct omap_mpu_state_s *mpu,
                                    struct omap_target_agent_s *ta,
                                    qemu_irq *irq, omap_clk *fclk,
                                    omap_clk iclk, int modules);
struct omap_gpif_s *omap3_gpio_init(struct omap_mpu_state_s *mpu,
                                    struct omap_target_agent_s **ta,
                                    qemu_irq *irq);
void omap_gpif_reset(struct omap_gpif_s *s);
qemu_irq omap2_gpio_in_get(struct omap_gpif_s *s, int line);
void omap2_gpio_out_set(struct omap_gpif_s *s, int line, qemu_irq handler);

/* omap_uart.c */
struct omap_uart_s;
struct omap_uart_s *omap_uart_init(target_phys_addr_t base,
                qemu_irq irq, omap_clk fclk, omap_clk iclk,
                qemu_irq txdma, qemu_irq rxdma,
                const char *label, CharDriverState *chr);
struct omap_uart_s *omap2_uart_init(struct omap_target_agent_s *ta,
                qemu_irq irq, omap_clk fclk, omap_clk iclk,
                qemu_irq txdma, qemu_irq rxdma,
                const char *label, CharDriverState *chr);
void omap_uart_reset(struct omap_uart_s *s);
void omap_uart_attach(struct omap_uart_s *s, CharDriverState *chr,
                      const char *label);

/* omap_gptimer.c */
struct omap_gp_timer_s;
struct omap_gp_timer_s *omap_gp_timer_init(struct omap_target_agent_s *ta,
                                           qemu_irq irq, omap_clk fclk,
                                           omap_clk iclk);
void omap_gp_timer_reset(struct omap_gp_timer_s *s);
void omap_gp_timer_change_clk(struct omap_gp_timer_s *timer);

/* omap1.c */
struct omap_mpuio_s;
qemu_irq *omap_mpuio_in_get(struct omap_mpuio_s *s);
void omap_mpuio_out_set(struct omap_mpuio_s *s, int line, qemu_irq handler);
void omap_mpuio_key(struct omap_mpuio_s *s, int row, int col, int down);

struct uWireSlave {
    uint16_t (*receive)(void *opaque);
    void (*send)(void *opaque, uint16_t data);
    void *opaque;
};
struct omap_uwire_s;
void omap_uwire_attach(struct omap_uwire_s *s,
                       uWireSlave *slave, int chipselect);

struct I2SCodec {
    void *opaque;

    /* The CPU can call this if it is generating the clock signal on the
     * i2s port.  The CODEC can ignore it if it is set up as a clock
     * master and generates its own clock.  */
    void (*set_rate)(void *opaque, int in, int out);

    void (*tx_swallow)(void *opaque);
    qemu_irq rx_swallow;
    qemu_irq tx_start;

    int tx_rate;
    int cts;
    int rx_rate;
    int rts;

    struct i2s_fifo_s {
        uint8_t *fifo;
        int len;
        int start;
        int size;
    } in, out;
};
struct omap_mcbsp_s;
struct omap_mcbsp_s *omap_mcbsp_init(target_phys_addr_t base,
                                     qemu_irq *irq, qemu_irq *dma, omap_clk clk);
void omap_mcbsp_i2s_attach(struct omap_mcbsp_s *s, I2SCodec *slave);

/* omap_synctimer.c */
struct omap_synctimer_s;
struct omap_synctimer_s *omap_synctimer_init(struct omap_target_agent_s *ta,
                                             struct omap_mpu_state_s *mpu,
                                             omap_clk fclk, omap_clk iclk);
void omap_synctimer_reset(struct omap_synctimer_s *s);

/* omap_tap.c */
void omap_tap_init(struct omap_target_agent_s *ta,
                   struct omap_mpu_state_s *mpu);

/* omap_dss.c */
struct omap_dss_s;
struct omap_dss_dispc_s;
struct rfbi_chip_s {
    void *opaque;
    void (*write)(void *opaque, int dc, uint16_t value);
    void (*block)(void *opaque, int dc, void *buf, size_t len, int pitch);
    uint16_t (*read)(void *opaque, int dc);
};
struct dsi_chip_s;
struct omap_dss_panel_s {
    void *opaque;
    void (*controlupdate)(void *opaque,
                          const struct omap_dss_dispc_s *dispc);
};
extern const int omap_lcd_Bpp[0x10];
void omap_dss_reset(struct omap_dss_s *s);
struct omap_dss_s *omap_dss_init(struct omap_target_agent_s *ta,
                                 struct omap_mpu_state_s *mpu,
                                 qemu_irq irq, qemu_irq drq,
                                 omap_clk fck1, omap_clk fck2, omap_clk ck54m,
                                 omap_clk ick1, omap_clk ick2);
struct omap_dss_s *omap3_dss_init(struct omap_target_agent_s *ta,
                                  struct omap_mpu_state_s *mpu,
                                  qemu_irq irq, qemu_irq line_trigger,
                                  qemu_irq dma0, qemu_irq dma1,
                                  qemu_irq dma2, qemu_irq dma3);
void omap_rfbi_attach(struct omap_dss_s *s, int cs, const struct rfbi_chip_s *chip);
void omap_dsi_attach(struct omap_dss_s *s, int vc, struct dsi_chip_s *chip);
void omap_lcd_panel_attach(struct omap_dss_s *s, const struct omap_dss_panel_s *p);

/* omap_lcdc.c */
struct omap_lcd_panel_s;
void omap_lcdc_reset(struct omap_lcd_panel_s *s);
struct omap_lcd_panel_s *omap_lcdc_init(target_phys_addr_t base, qemu_irq irq,
                                        struct omap_dma_lcd_channel_s *dma,
                                        ram_addr_t imif_base, ram_addr_t emiff_base, omap_clk clk);

/* omap3_lcd.c */
struct omap3_lcd_panel_s;
struct omap3_lcd_panel_s *omap3_lcd_panel_init(struct omap_dss_s *dss);
const struct omap_dss_panel_s *omap3_lcd_panel_get(struct omap3_lcd_panel_s *lcd);
void omap3_lcd_panel_layer_update(DisplayState *ds,
                                  uint32_t lcd_width, uint32_t lcd_height,
                                  uint32_t posx,
                                  int *posy, int *endy,
                                  uint32_t width, uint32_t height,
                                  uint32_t attrib,
                                  target_phys_addr_t addr,
                                  uint32_t *palette,
                                  int full_update);

/* omap_mmc.c */
struct omap_mmc_s;
struct omap_mmc_s *omap_mmc_init(target_phys_addr_t base,
                BlockDriverState *bd,
                qemu_irq irq, qemu_irq dma[], omap_clk clk);
struct omap_mmc_s *omap2_mmc_init(struct omap_target_agent_s *ta,
                BlockDriverState *bd, qemu_irq irq, qemu_irq dma[],
                omap_clk fclk, omap_clk iclk);
void omap_mmc_reset(struct omap_mmc_s *s);
void omap_mmc_handlers(struct omap_mmc_s *s, qemu_irq ro, qemu_irq cover);
void omap_mmc_enable(struct omap_mmc_s *s, int enable);

/* omap3_mmc.c */
void omap3_mmc_attach(DeviceState *dev, BlockDriverState *bs,
                      int is_spi, int is_mmc);

/* omap_i2c.c */
struct omap_i2c_s;
struct omap_i2c_s *omap_i2c_init(target_phys_addr_t base,
                                 qemu_irq irq, qemu_irq *dma, omap_clk clk);
struct omap_i2c_s *omap2_i2c_init(struct omap_target_agent_s *ta,
                                  qemu_irq irq, qemu_irq *dma, omap_clk fclk,
                                  omap_clk iclk);
struct omap_i2c_s *omap3_i2c_init(struct omap_target_agent_s *ta,
                                  struct omap_mpu_state_s *mpu,
                                  qemu_irq irq, qemu_irq *dma, omap_clk fclk,
                                  omap_clk iclk, int fifosize);
void omap_i2c_reset(struct omap_i2c_s *s);
i2c_bus *omap_i2c_bus(struct omap_i2c_s *s);

/* omap_spi.c */
struct omap_mcspi_s;
struct omap_mcspi_s *omap_mcspi_init(struct omap_target_agent_s *ta,
                                     struct omap_mpu_state_s *mpu,
                                     int chnum, qemu_irq irq, qemu_irq *drq,
                                     omap_clk fclk, omap_clk iclk);
void omap_mcspi_attach(struct omap_mcspi_s *s,
                       uint32_t (*txrx)(void *opaque, uint32_t, int),
                       void *opaque, int chipselect);
void omap_mcspi_reset(struct omap_mcspi_s *s);

/* omap_usb.c */
struct omap3_hsusb_s;
struct omap3_hsusb_s *omap3_hsusb_init(struct omap_target_agent_s *otg_ta,
                                       struct omap_target_agent_s *host_ta,
                                       struct omap_target_agent_s *tll_ta,
                                       qemu_irq mc_irq,
                                       qemu_irq dma_irq,
                                       qemu_irq ohci_irq,
                                       qemu_irq ehci_irq,
                                       qemu_irq tll_irq,
                                       void (*otg_stdby_cb)(void *, int),
                                       void *otg_stdby_opaque);
void omap3_hsusb_reset(struct omap3_hsusb_s *s);

/* usb-ohci.c */
void usb_ohci_init_omap(target_phys_addr_t base, uint32_t region_size,
                        int num_ports, qemu_irq irq);

# define cpu_is_omap310(cpu)		(cpu->mpu_model == omap310)
# define cpu_is_omap1510(cpu)		(cpu->mpu_model == omap1510)
# define cpu_is_omap1610(cpu)		(cpu->mpu_model == omap1610)
# define cpu_is_omap1710(cpu)		(cpu->mpu_model == omap1710)
# define cpu_is_omap2410(cpu)		(cpu->mpu_model == omap2410)
# define cpu_is_omap2420(cpu)		(cpu->mpu_model == omap2420)
# define cpu_is_omap2430(cpu)		(cpu->mpu_model == omap2430)
# define cpu_is_omap3430(cpu)		(cpu->mpu_model == omap3430)
# define cpu_is_omap3630(cpu)       (cpu->mpu_model == omap3630)

# define cpu_is_omap15xx(cpu)		\
        (cpu_is_omap310(cpu) || cpu_is_omap1510(cpu))
# define cpu_is_omap16xx(cpu)		\
        (cpu_is_omap1610(cpu) || cpu_is_omap1710(cpu))
# define cpu_is_omap24xx(cpu)		\
        (cpu_is_omap2410(cpu) || cpu_is_omap2420(cpu) || cpu_is_omap2430(cpu))

# define cpu_class_omap1(cpu)		\
        (cpu_is_omap15xx(cpu) || cpu_is_omap16xx(cpu))
# define cpu_class_omap2(cpu)		cpu_is_omap24xx(cpu)
# define cpu_class_omap3(cpu)		\
        (cpu_is_omap3430(cpu) || cpu_is_omap3630(cpu))

struct omap_mpu_state_s {
    enum omap_mpu_model {
        omap310,
        omap1510,
        omap1610,
        omap1710,
        omap2410,
        omap2420,
        omap2422,
        omap2423,
        omap2430,
        omap3430,
        omap3630,
    } mpu_model;

    CPUState *env;

    qemu_irq *irq[2];
    qemu_irq *drq;

    qemu_irq wakeup;

    struct omap_dma_port_if_s {
        uint32_t (*read[3])(struct omap_mpu_state_s *s,
                        target_phys_addr_t offset);
        void (*write[3])(struct omap_mpu_state_s *s,
                        target_phys_addr_t offset, uint32_t value);
        int (*addr_valid)(struct omap_mpu_state_s *s,
                        target_phys_addr_t addr);
    } port[__omap_dma_port_last];

    unsigned long sdram_size;
    unsigned long sram_size;

    /* MPUI-TIPB peripherals */
    struct omap_uart_s *uart[4];
    struct omap_gpio_s *gpio;
    struct omap_mcbsp_s *mcbsp1;
    struct omap_mcbsp_s *mcbsp3;

    /* MPU public TIPB peripherals */
    struct omap_32khz_timer_s *os_timer;
    struct omap_mmc_s *mmc;
    struct omap_mpuio_s *mpuio;
    struct omap_uwire_s *microwire;
    struct omap_pwl_s *pwl;
    struct omap_pwt_s *pwt;
    struct omap_i2c_s *i2c[3];
    struct omap_rtc_s *rtc;
    struct omap_mcbsp_s *mcbsp2;
    struct omap_lpg_s *led[2];

    /* MPU private TIPB peripherals */
    struct omap_intr_handler_s *ih[2];

    struct soc_dma_s *dma;

    struct omap_mpu_timer_s *timer[3];
    struct omap_watchdog_timer_s *wdt;

    struct omap_lcd_panel_s *lcd;

    uint32_t ulpd_pm_regs[21];
    int64_t ulpd_gauge_start;

    uint32_t func_mux_ctrl[14];
    uint32_t comp_mode_ctrl[1];
    uint32_t pull_dwn_ctrl[4];
    uint32_t gate_inh_ctrl[1];
    uint32_t voltage_ctrl[1];
    uint32_t test_dbg_ctrl[1];
    uint32_t mod_conf_ctrl[1];
    int compat1509;

    uint32_t mpui_ctrl;

    struct omap_tipb_bridge_s *private_tipb;
    struct omap_tipb_bridge_s *public_tipb;

    uint32_t tcmi_regs[17];

    struct dpll_ctl_s *dpll[3];

    omap_clk clks;
    struct omap_clkm_s *clkm;

    /* OMAP2-only peripherals */
    struct omap_l4_s *l4;

    struct omap_gp_timer_s *gptimer[12];
    struct omap_synctimer_s *synctimer;

    struct omap_prcm_s *prcm;
    struct omap_sdrc_s *sdrc;
    struct omap_gpmc_s *gpmc;
    struct omap_sysctl_s *sysc;

    struct omap_gpif_s *gpif;

    struct omap_mcspi_s *mcspi[4];

    struct omap_dss_s *dss;

    struct omap_eac_s *eac;

    /* OMAP3-only */
    struct omap3_prm_s *omap3_prm;
    struct omap3_cm_s *omap3_cm;
    struct omap3_wdt_s *omap3_mpu_wdt;
    struct omap3_l3_s *omap3_l3;
    struct omap3_scm_s *omap3_scm;
    struct omap3_sms_s *omap3_sms;
    DeviceState *omap3_mmc[3];
    DeviceState *omap3_usb_otg;
    DeviceState *omap3_usb_host;
    ram_addr_t bootrom_base;
};

/* omap1.c */
struct omap_mpu_state_s *omap310_mpu_init(unsigned long sdram_size,
                const char *core);

/* omap2.c */
struct omap_mpu_state_s *omap2420_mpu_init(unsigned long sdram_size,
                const char *core);

/* omap3.c */
struct omap_mpu_state_s *omap3_mpu_init(int model,
                                        unsigned long sdram_size,
                                        CharDriverState *chr_uart1,
                                        CharDriverState *chr_uart2,
                                        CharDriverState *chr_uart3,
                                        CharDriverState *chr_uart4);
void omap3_set_mem_type(struct omap_mpu_state_s *s, int bootfrom);

/* omap3_boot.c */
void omap3_boot_rom_emu(struct omap_mpu_state_s *s);

# if TARGET_PHYS_ADDR_BITS == 32
#  define OMAP_FMT_plx "0x%08x"
# elif TARGET_PHYS_ADDR_BITS == 64
#  define OMAP_FMT_plx "0x%08" PRIx64
# else
#  error TARGET_PHYS_ADDR_BITS undefined
# endif

uint32_t omap_badwidth_read8(void *opaque, target_phys_addr_t addr);
void omap_badwidth_write8(void *opaque, target_phys_addr_t addr,
                uint32_t value);
uint32_t omap_badwidth_read16(void *opaque, target_phys_addr_t addr);
void omap_badwidth_write16(void *opaque, target_phys_addr_t addr,
                uint32_t value);
uint32_t omap_badwidth_read32(void *opaque, target_phys_addr_t addr);
void omap_badwidth_write32(void *opaque, target_phys_addr_t addr,
                uint32_t value);

void omap_mpu_wakeup(void *opaque, int irq, int req);

# define OMAP_BAD_REG(paddr)		\
        fprintf(stderr, "%s: Bad register " OMAP_FMT_plx "\n",	\
                        __FUNCTION__, paddr)
# define OMAP_BAD_REGV(paddr, value) \
        fprintf(stderr, "%s: Bad register " OMAP_FMT_plx " (value 0x%08x)\n", \
                __FUNCTION__, paddr, value)
# define OMAP_RO_REG(paddr)		\
        fprintf(stderr, "%s: Read-only register " OMAP_FMT_plx "\n",	\
                        __FUNCTION__, paddr)
# define OMAP_RO_REGV(paddr, value) \
        fprintf(stderr, "%s: Read-only register " OMAP_FMT_plx " (value 0x%08x)\n", \
                __FUNCTION__, paddr, value)

/* OMAP-specific Linux bootloader tags for the ATAG_BOARD area
   (Board-specifc tags are not here)  */
#define OMAP_TAG_CLOCK		0x4f01
#define OMAP_TAG_MMC		0x4f02
#define OMAP_TAG_SERIAL_CONSOLE	0x4f03
#define OMAP_TAG_USB		0x4f04
#define OMAP_TAG_LCD		0x4f05
#define OMAP_TAG_GPIO_SWITCH	0x4f06
#define OMAP_TAG_UART		0x4f07
#define OMAP_TAG_FBMEM		0x4f08
#define OMAP_TAG_STI_CONSOLE	0x4f09
#define OMAP_TAG_CAMERA_SENSOR	0x4f0a
#define OMAP_TAG_PARTITION	0x4f0b
#define OMAP_TAG_TEA5761	0x4f10
#define OMAP_TAG_TMP105		0x4f11
#define OMAP_TAG_BOOT_REASON	0x4f80
#define OMAP_TAG_FLASH_PART_STR	0x4f81
#define OMAP_TAG_VERSION_STR	0x4f82

enum {
    OMAP_GPIOSW_TYPE_COVER	= 0 << 4,
    OMAP_GPIOSW_TYPE_CONNECTION	= 1 << 4,
    OMAP_GPIOSW_TYPE_ACTIVITY	= 2 << 4,
};

#define OMAP_GPIOSW_INVERTED	0x0001
#define OMAP_GPIOSW_OUTPUT	0x0002

# define TCMI_VERBOSE			1
//# define MEM_VERBOSE			1

# ifdef TCMI_VERBOSE
#  define OMAP_8B_REG(paddr)		\
        fprintf(stderr, "%s: 8-bit register " OMAP_FMT_plx "\n",	\
                        __FUNCTION__, paddr)
#  define OMAP_16B_REG(paddr)		\
        fprintf(stderr, "%s: 16-bit register " OMAP_FMT_plx "\n",	\
                        __FUNCTION__, paddr)
#  define OMAP_32B_REG(paddr)		\
        fprintf(stderr, "%s: 32-bit register " OMAP_FMT_plx "\n",	\
                        __FUNCTION__, paddr)
# else
#  define OMAP_8B_REG(paddr)
#  define OMAP_16B_REG(paddr)
#  define OMAP_32B_REG(paddr)
# endif

# define OMAP_MPUI_REG_MASK		0x000007ff

# ifdef MEM_VERBOSE
struct io_fn {
    CPUReadMemoryFunc * const *mem_read;
    CPUWriteMemoryFunc * const *mem_write;
    void *opaque;
    int in;
};

static uint32_t io_readb(void *opaque, target_phys_addr_t addr)
{
    struct io_fn *s = opaque;
    uint32_t ret;

    s->in ++;
    ret = s->mem_read[0](s->opaque, addr);
    s->in --;
    if (!s->in)
        fprintf(stderr, "%08x ---> %02x\n", (uint32_t) addr, ret);
    return ret;
}
static uint32_t io_readh(void *opaque, target_phys_addr_t addr)
{
    struct io_fn *s = opaque;
    uint32_t ret;

    s->in ++;
    ret = s->mem_read[1](s->opaque, addr);
    s->in --;
    if (!s->in)
        fprintf(stderr, "%08x ---> %04x\n", (uint32_t) addr, ret);
    return ret;
}
static uint32_t io_readw(void *opaque, target_phys_addr_t addr)
{
    struct io_fn *s = opaque;
    uint32_t ret;

    s->in ++;
    ret = s->mem_read[2](s->opaque, addr);
    s->in --;
    if (!s->in)
        fprintf(stderr, "%08x ---> %08x\n", (uint32_t) addr, ret);
    return ret;
}
static void io_writeb(void *opaque, target_phys_addr_t addr, uint32_t value)
{
    struct io_fn *s = opaque;

    if (!s->in)
        fprintf(stderr, "%08x <--- %02x\n", (uint32_t) addr, value);
    s->in ++;
    s->mem_write[0](s->opaque, addr, value);
    s->in --;
}
static void io_writeh(void *opaque, target_phys_addr_t addr, uint32_t value)
{
    struct io_fn *s = opaque;

    if (!s->in)
        fprintf(stderr, "%08x <--- %04x\n", (uint32_t) addr, value);
    s->in ++;
    s->mem_write[1](s->opaque, addr, value);
    s->in --;
}
static void io_writew(void *opaque, target_phys_addr_t addr, uint32_t value)
{
    struct io_fn *s = opaque;

    if (!s->in)
        fprintf(stderr, "%08x <--- %08x\n", (uint32_t) addr, value);
    s->in ++;
    s->mem_write[2](s->opaque, addr, value);
    s->in --;
}

static CPUReadMemoryFunc * const io_readfn[] = { io_readb, io_readh, io_readw, };
static CPUWriteMemoryFunc * const io_writefn[] = { io_writeb, io_writeh, io_writew, };

inline static int debug_register_io_memory(CPUReadMemoryFunc * const *mem_read,
                                           CPUWriteMemoryFunc * const *mem_write,
                                           void *opaque)
{
    struct io_fn *s = qemu_malloc(sizeof(struct io_fn));

    s->mem_read = mem_read;
    s->mem_write = mem_write;
    s->opaque = opaque;
    s->in = 0;
    return cpu_register_io_memory(io_readfn, io_writefn, s,
                                  DEVICE_NATIVE_ENDIAN);
}
#  define cpu_register_io_memory	debug_register_io_memory
# endif

#endif /* hw_omap_h */

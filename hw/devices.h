#ifndef QEMU_DEVICES_H
#define QEMU_DEVICES_H

/* Devices that have nowhere better to go.  */

/* smc91c111.c */
void smc91c111_init(NICInfo *, uint32_t, qemu_irq);

/* lan9118.c */
void lan9118_init(NICInfo *, uint32_t, qemu_irq);

/* tsc210x.c */
uWireSlave *tsc2102_init(qemu_irq pint);
I2SCodec *tsc210x_codec(uWireSlave *chip);
void tsc210x_set_transform(uWireSlave *chip, MouseTransformInfo *info);
void tsc2301_set_transform(DeviceState *qdev, MouseTransformInfo *info);
void tsc210x_key_event(uWireSlave *chip, int key, int down);
void tsc2301_key_event(DeviceState *qdev, int key, int down);

/* tsc2005.c */
void tsc2005_set_transform(DeviceState *qdev, MouseTransformInfo *info,
                           int z1_cons, int z2_cons);

/* stellaris_input.c */
void stellaris_gamepad_init(int n, qemu_irq *irq, const int *keycode);

/* blizzard.c */
void *s1d13745_init(qemu_irq gpio_int);
void s1d13745_write(void *opaque, int dc, uint16_t value);
void s1d13745_write_block(void *opaque, int dc,
                void *buf, size_t len, int pitch);
uint16_t s1d13745_read(void *opaque, int dc);

/* cbus.c */
typedef struct {
    qemu_irq clk;
    qemu_irq dat;
    qemu_irq sel;
} CBus;
CBus *cbus_init(qemu_irq dat_out);
void cbus_attach(CBus *bus, void *slave_opaque);

void *retu_init(qemu_irq irq, int vilma);
void *tahvo_init(qemu_irq irq, int betty);

void retu_key_event(void *retu, int state);

/* tc6393xb.c */
typedef struct TC6393xbState TC6393xbState;
#define TC6393XB_RAM	0x110000 /* amount of ram for Video and USB */
TC6393xbState *tc6393xb_init(uint32_t base, qemu_irq irq);
void tc6393xb_gpio_out_set(TC6393xbState *s, int line,
                    qemu_irq handler);
qemu_irq *tc6393xb_gpio_in_get(TC6393xbState *s);
qemu_irq tc6393xb_l3v_get(TC6393xbState *s);

/* sm501.c */
void sm501_init(uint32_t base, uint32_t local_mem_bytes, qemu_irq irq,
                CharDriverState *chr);

/* nseries.c */
void lis302dl_step(void *opaque, int axis, int high, int activate);
#endif

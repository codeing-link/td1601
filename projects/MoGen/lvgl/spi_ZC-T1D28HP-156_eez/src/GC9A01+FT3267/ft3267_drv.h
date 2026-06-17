#ifndef FT3267_DRV_H
#define FT3267_DRV_H

#include <stdint.h>

#define FT3267_ADDR                    0x38

#define FT3267_TOUCH_POINTS            0x02
#define FT3267_TOUCH1_XH               0x03
#define FT3267_TOUCH1_XL               0x04
#define FT3267_TOUCH1_YH               0x05
#define FT3267_TOUCH1_YL               0x06

void ft3267_init(void);
uint8_t ft3267_int_consume(void);
uint8_t ft3267_read_touch(uint16_t *x, uint16_t *y);

#endif

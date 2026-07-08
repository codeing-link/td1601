/*
 * Copyright (C) 2017-2019 NStar-Chip Holding Limited
 */


/******************************************************************************
 * @file     pinmux.c
 * @brief    source file for the pinmux
 * @version  V1.0
 * @date     02. June 2017
 ******************************************************************************/

#include <stdint.h>
#include <stddef.h>
#include <drv/pin.h>
#include <drv/gpio.h>
#include "soc.h"

#define HOBBIT_GIPO0_PORTCTL_REG 0x60030000U
#define HOBBIT_GIPO1_PORTCTL_REG 0x60030004U
#define HOBBIT_IOMUX0L_REG       0x60030008U
#define HOBBIT_IOMUX0H_REG       0x6003000CU
#define HOBBIT_IOMUX1L_REG       0x60030010U
#define HOBBIT_IOPU0_REG         0x60030020U
#define HOBBIT_IOPU1_REG         0x60030024U
#define HOBBIT_IOPD0_REG         0x60030028U
#define HOBBIT_IOPD1_REG         0x6003002cU
#define HOBBIT_IOOD0_REG         0x60030048U
#define HOBBIT_IOOD1_REG         0x6003004cU
#define HOBBIT_IOSR0_REG         0x60030060U
#define HOBBIT_IODS0L_REG        0x60030038U
#define HOBBIT_IODS0H_REG        0x6003003CU

#define HOBBIT1_GIPO0_PORTCTL_REG 0x60030100U
#define HOBBIT1_GIPO1_PORTCTL_REG 0x60030104U
#define HOBBIT1_IOMUX0L_REG       0x60030108U
#define HOBBIT1_IOMUX0H_REG       0x6003010CU
#define HOBBIT1_IOMUX1L_REG       0x60030110U
#define HOBBIT1_IOPU0_REG         0x60030120U
#define HOBBIT1_IOPU1_REG         0x60030124U
#define HOBBIT1_IOPD0_REG         0x60030128U
#define HOBBIT1_IOPD1_REG         0x6003012cU
#define HOBBIT1_IOOD0_REG         0x60030148U
#define HOBBIT1_IOOD1_REG         0x6003014cU
#define HOBBIT1_IOSR0_REG         0x60030160U
#define HOBBIT1_IODS0L_REG        0x60030138U
#define HOBBIT1_IODS0H_REG        0x6003013CU

#define CSKY_ANA_IO_EN_REG       (WJ_IOC_BASE + 0x70U)
#define CSKY_ANA1_IO_EN_REG       (WJ_IOC_BASE + 0x70U + 0x100U)
#define readl(addr) \
    ({ unsigned int __v = (*(volatile uint32_t *) (addr)); __v; })

#define writel(b,addr) ((*(volatile uint32_t *) (addr)) = (b))

#ifdef CONFIG_XIP
#define ATTRIBUTE_DATA __attribute__((section(".ram.code")))
#else
#define ATTRIBUTE_DATA
#endif

static int get_shift_bit(uint32_t val)
{
    if (val == 0)
        return -1; // 无bit置1，错误

    for (int i = 0; i < 32; i++)
    {
        if (val == (1U << i))
        {
            return i;
        }
    }
    return -2; // 多个bit同时置1
}

/*******************************************************************************
 * function: danica_ioreuse_inital
 *
 * description:
 *   initial danica_pinmux
 *******************************************************************************/
ATTRIBUTE_DATA csi_error_t csi_pin_set_mux(port_name_t port_name, pin_name_t pin_name, pin_func_t pin_func)
{
    uint32_t val = 0U;
    uint32_t reg_val = 0U;
    uint32_t reg_mux0 = 0U;
    csi_error_t ret = CSI_OK;

    uint32_t offset = get_shift_bit((uint32_t)pin_name);
    if (pin_func >= 5) {
        if (port_name == PORTA) {
            val = readl(CSKY_ANA_IO_EN_REG);
            val |= (uint32_t)1U << ((uint8_t)pin_func - 5U);
            writel(val, CSKY_ANA_IO_EN_REG);
        } else {
            val = readl(CSKY_ANA1_IO_EN_REG);
            val |= (uint32_t)1U << ((uint8_t)pin_func - 5U);
            writel(val, CSKY_ANA1_IO_EN_REG);            
        }
         ret = CSI_OK;
    }else if (pin_func == PIN_FUNC_GPIO) {
        if (port_name == PORTA) {
            val = readl(HOBBIT_GIPO0_PORTCTL_REG);
            val &= ~(pin_name);
            writel(val, HOBBIT_GIPO0_PORTCTL_REG);
        } else {
            val = readl(HOBBIT1_GIPO0_PORTCTL_REG);
            val &= ~(pin_name);
            writel(val, HOBBIT1_GIPO0_PORTCTL_REG);
        }
        if (port_name == PORTA) {
            /* if gpio use gpio_func need clear af_sel */
            if ((uint32_t)pin_name >= (uint32_t)PIN16 && (uint32_t)pin_name <= (uint32_t)PIN31) {
                reg_mux0 = HOBBIT_IOMUX0H_REG;
                offset = ((pin_name_t)((pin_name_t)offset - (pin_name_t)16U));
            } else if ((uint32_t)pin_name < (uint32_t)PIN16){
                reg_mux0 = HOBBIT_IOMUX0L_REG;
            }
        } else {
            if ((uint32_t)pin_name >= (uint32_t)PIN0 && (uint32_t)pin_name < (uint32_t)PIN16) {
                reg_mux0 = HOBBIT1_IOMUX0L_REG;
            } else {
                reg_mux0 = HOBBIT1_IOMUX0H_REG;
            }
        }

        reg_val = ((uint32_t)0x3U << ((uint8_t)offset * 2U));
        /* reuse function select */
        val = readl(reg_mux0);
        val &= ~(reg_val);
        val |= ((uint32_t)0U << (2U * (uint8_t)offset));
        writel(val, reg_mux0);
        ret = CSI_OK;
    }else{
        if (port_name == PORTA) {
            val = readl(HOBBIT_GIPO0_PORTCTL_REG);
            val |= pin_name;
            writel(val, HOBBIT_GIPO0_PORTCTL_REG);
        } else {
            val = readl(HOBBIT1_GIPO0_PORTCTL_REG);
            val |= pin_name;
            writel(val, HOBBIT1_GIPO0_PORTCTL_REG);
        }
        if (port_name == PORTA) {
            if ((uint32_t)pin_name >= (uint32_t)PIN16 && (uint32_t)pin_name <= (uint32_t)PIN31) {
                reg_mux0 = HOBBIT_IOMUX0H_REG;
                offset = ((pin_name_t)((pin_name_t)offset - (pin_name_t)16U));
            } else if ((uint32_t)pin_name < (uint32_t)PIN16) {
                reg_mux0 = HOBBIT_IOMUX0L_REG;
            }
        } else {
            if ((uint32_t)pin_name >= (uint32_t)PIN0 && (uint32_t)pin_name < (uint32_t)PIN16) {
                reg_mux0 = HOBBIT1_IOMUX0L_REG;
            } else {
                reg_mux0 = HOBBIT1_IOMUX0L_REG;
            }
        }
        reg_val = ((uint32_t)0x3U << ((uint8_t)offset * 2U));
        /* reuse function select */
        val = readl(reg_mux0);
        val &= ~(reg_val);
        val |= ((uint32_t)pin_func << (2U * (uint8_t)offset));
        writel(val, reg_mux0);
        ret = CSI_OK;
    }
    return ret;
}


csi_error_t csi_pin_mode(port_name_t port_name, pin_name_t pin_name, csi_gpio_mode_t mode)
{
    uint32_t reg = 0U;
    uint32_t val = 0U;
    csi_error_t ret = CSI_OK;

    switch (mode) {
        case GPIO_MODE_OPEN_DRAIN:
        case GPIO_MODE_PUSH_PULL:
            ret = CSI_UNSUPPORTED;
            break;
        case GPIO_MODE_PULLNONE:
            if (port_name == PORTA) {
                val = readl(HOBBIT_IOPD0_REG);
                if (val & (pin_name)) {
                    val &= ~(pin_name);
                    writel(val, HOBBIT_IOPD0_REG);
                }
                val = readl(HOBBIT_IOPU0_REG);
                if (val & (pin_name)) {
                    val &= ~(pin_name);
                    writel(val, HOBBIT_IOPU0_REG);
                }
            } else {
                val = readl(HOBBIT1_IOPD0_REG);
                if (val & (pin_name)) {
                    val &= ~(pin_name);
                    writel(val, HOBBIT1_IOPD0_REG);
                }
                val = readl(HOBBIT1_IOPU0_REG);
                if (val & (pin_name)) {
                    val &= ~(pin_name);
                    writel(val, HOBBIT1_IOPU0_REG);
                }                
            }
            ret = CSI_OK;
            break;
        case GPIO_MODE_PULLUP:
            if (port_name == PORTA) {
                val = readl(HOBBIT_IOPD0_REG);

                if (val & (pin_name)) {
                    val &= ~(pin_name);
                    writel(val, HOBBIT_IOPD0_REG);
                }
                reg = HOBBIT_IOPU0_REG;
            } else {
                val = readl(HOBBIT1_IOPD0_REG);

                if (val & (pin_name)) {
                    val &= ~(pin_name);
                    writel(val, HOBBIT1_IOPD0_REG);
                }
                reg = HOBBIT1_IOPU0_REG;  
            }
            ret = CSI_OK;
            break;

        case GPIO_MODE_PULLDOWN:
            if (port_name == PORTA) {
                val = readl(HOBBIT_IOPU0_REG);

                if (val & (pin_name)) {
                    val &= ~(pin_name);
                    writel(val, HOBBIT_IOPU0_REG);
                }
                reg = HOBBIT_IOPD0_REG;
            } else {
                val = readl(HOBBIT1_IOPU0_REG);

                if (val & (pin_name)) {
                    val &= ~(pin_name);
                    writel(val, HOBBIT1_IOPU0_REG);
                }
                reg = HOBBIT1_IOPD0_REG;                
            }
            ret = CSI_OK;
            break;
        default:
            ret = CSI_ERROR;
            break;
    }
    if((ret == CSI_OK) && (mode != GPIO_MODE_PULLNONE))
    {
        val = readl(reg);
        val |= (uint32_t)pin_name;
        writel(val, reg);
    }
    return ret;
}


/**
  \brief       get the pin function.
  \param[in]   pin       refs to pin_name_e.
  \return      pin function count
*/
pin_func_t csi_pin_get_mux(port_name_t port_name, pin_name_t   pin_name)
{
    uint32_t val = 0U;
    uint32_t reg_val = 0U;
    uint32_t reg_mux0 = 0U;
    uint8_t offset = (uint8_t)get_shift_bit((uint32_t)pin_name);
    uint32_t ret = 0U;

    /* gpio data source select */
    if (port_name == PORTA) {
        val = readl(HOBBIT_GIPO0_PORTCTL_REG);
        val &= ((uint32_t)1U << offset);

        if (val != ((uint32_t)1U << offset) ) {
            ret = (uint32_t)PIN_FUNC_GPIO;
        }else{
            if ((uint32_t)pin_name >= (uint32_t)PIN16) {
                offset = (uint8_t)offset - 16U;
                reg_mux0 = HOBBIT_IOMUX0H_REG;
            } else {
                reg_mux0 = HOBBIT_IOMUX0L_REG;
            }

            reg_val = ((uint32_t)0x3U << (offset * 2U));
            /* reuse function select */
            val = readl(reg_mux0);
            val &= (reg_val);
            ret = val >> (2U * offset);
        }
    } else {
        offset = offset - 32;
        val = readl(HOBBIT1_GIPO0_PORTCTL_REG);
        val &= ((uint32_t)1U << offset);

        if (val != ((uint32_t)1U << offset) ) {
            ret = (uint32_t)PIN_FUNC_GPIO;
        }else{
            if ((uint32_t)pin_name >= (uint32_t)PIN16) {
                offset = (uint8_t)offset - 16U;
                reg_mux0 = HOBBIT1_IOMUX0H_REG;
            } else {
                reg_mux0 = HOBBIT1_IOMUX0L_REG;
            }

            reg_val = ((uint32_t)0x3U << (offset * 2U));
            /* reuse function select */
            val = readl(reg_mux0);
            val &= (reg_val);
            ret = val >> (2U * offset);
        }
    }
    return (pin_func_t)ret;
}

/**
  \brief       set pin speed
  \param[in]   pin_name pin name, defined in soc.h.
  \param[in]   speed    io speed
  \return      error code
*/
csi_error_t csi_pin_speed(port_name_t port_name, pin_name_t pin_name, csi_pin_speed_t speed)
{
    uint32_t reg_val = 0U;
    csi_error_t ret = CSI_OK;

    if (speed > PIN_SPEED_LV1) {
        ret = CSI_UNSUPPORTED;
    }else{
        if (port_name == PORTA) {
            reg_val =  readl(HOBBIT_IOSR0_REG);
            if (speed == PIN_SPEED_LV0) {
                if (reg_val & (pin_name)) {
                    reg_val &= ~(pin_name);
                    writel(reg_val, HOBBIT_IOSR0_REG);
                }
            } else {
                reg_val |= pin_name;
                writel(reg_val, HOBBIT_IOSR0_REG);
            }
        } else {
            reg_val =  readl(HOBBIT1_IOSR0_REG);
            if (speed == PIN_SPEED_LV0) {
                if (reg_val & (pin_name)) {
                    reg_val &= ~(pin_name);
                    writel(reg_val, HOBBIT1_IOSR0_REG);
                }
            } else {
                reg_val |= pin_name;
                writel(reg_val, HOBBIT1_IOSR0_REG);
            }            
        }
        ret = CSI_OK;
    }
    return ret;
}


/**
  \brief       set pin drive
  \param[in]   pin_name pin name, defined in soc.h.
  \param[in]   drive    io drive
  \return      error code
*/
csi_error_t csi_pin_drive(port_name_t port_name, pin_name_t pin_name, csi_pin_drive_t drive)
{
    uint32_t val = 0U;
    uint32_t reg_val = 0U;
    uint32_t reg_mux0 = 0U;
    uint32_t offset = get_shift_bit((uint32_t)pin_name);
    if (port_name == PORTA) {
        if ((uint32_t)pin_name >= (uint32_t)PIN16 && (uint32_t)pin_name <= (uint32_t)PIN31) {
            reg_mux0 = HOBBIT_IODS0H_REG;
            offset = offset - 16;
        } else if ((uint32_t)pin_name < (uint32_t)PIN16){
            reg_mux0 = HOBBIT_IODS0L_REG;
        }
    } else {
        if ((uint32_t)pin_name >= (uint32_t)PIN0 && (uint32_t)pin_name < (uint32_t)PIN16) {
            reg_mux0 = HOBBIT1_IODS0L_REG;
        } else {
            reg_mux0 = HOBBIT1_IODS0H_REG;
            offset = offset - 16;
        }
    }
    reg_val = ((uint32_t)0x3U << ((uint8_t)offset * 2U));
    /* reuse function select */
    val = readl(reg_mux0);
    val &= ~(reg_val);
    val |= ((uint32_t)drive << (2U * (uint8_t)offset));
    writel(val, reg_mux0);
    return CSI_OK;
}

csi_error_t csi_pin_wakeup(port_name_t port_name, pin_name_t pin_name, bool enable)
{
    return CSI_OK;
}


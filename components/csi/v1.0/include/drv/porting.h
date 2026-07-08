

/******************************************************************************
 * @file     drv/porting.h
 * @brief    Header File for SOC Porting
 * @version  V1.0
 * @date     8. Apr 2020
 * @model    porting
 ******************************************************************************/

#ifndef _DRV_PORTING_H_
#define _DRV_PORTING_H_

#include <stdint.h>
#include <stdbool.h>
#include <drv/common.h>

/**
  \brief       Soc get device frequence.
  \param[in]   idx      Device index
  \return      frequence
*/
uint32_t soc_get_apb_freq(uint32_t idx);
uint32_t soc_get_ahb_freq(uint32_t idx);
uint32_t soc_get_cpu_freq(uint32_t idx);
uint32_t soc_get_uart_freq(uint32_t idx);
uint32_t soc_get_spi_freq(uint32_t idx);
uint32_t soc_get_iic_freq(uint32_t idx);
uint32_t soc_get_i2s_freq(uint32_t idx);
uint32_t soc_get_pwm_freq(uint32_t idx);
uint32_t soc_get_adc_freq(uint32_t idx);
uint32_t soc_get_qspi_freq(uint32_t idx);
uint32_t soc_get_usi_freq(uint32_t idx);
uint32_t soc_get_timer_freq(uint32_t idx);
uint32_t soc_get_rtc_freq(uint32_t idx);
uint32_t soc_get_wdt_freq(uint32_t idx);
uint32_t soc_get_sdio_freq(uint32_t idx);
uint32_t soc_get_emmc_freq(uint32_t idx);
uint32_t soc_get_usb_freq(uint32_t idx);
uint32_t soc_get_coretim_freq(void);
uint32_t soc_get_cur_cpu_freq(void);
uint32_t soc_get_ref_clk_freq(uint32_t idx);
uint32_t soc_get_avfs_freq(void);

typedef enum {
    PM_GNSS_DCU_MODE_CLKON_PON       = 0,   ///< clock on power on
    PM_GNSS_DCU_MODE_CLKOFF_PON,            ///< clock off power on
    PM_GNSS_DCU_MODE_CLKOFF_POFF            ///< clock off power off
} csi_pm_gnss_dcu_mode_t;

typedef enum {
  PM_MODE_HOLD_PWR_SUPPLY_SEL = 0,
  PM_MODE_HOLD_SCLK,
  PM_MODE_HOLD_AON_PWR,
  PM_MODE_HOLD_LP_RAM_LDO,
  PM_MODE_HOLD_MAIN_PWR,
  PM_MODE_HOLD_RF_PWR,
  PM_MODE_HOLD_PDU_DCU,
  PM_MODE_HOLD_ANA_DCU,
  PM_MODE_HOLD_RETU_DCU,
  PM_MODE_HOLD_GNSS_DCU,
} csi_pm_mode_hold_t;

/**
  \brief       Soc get device frequence.
  \param[in]   freq     CPU frequence
  \return      none
*/
void soc_set_sys_freq(uint32_t freq);

/*
  \brief       Soc enable device clock
  \param[in]   module   Clock module, defined in sys_clk.h, \ref clk_module_t
  \return      none
*/
void soc_clk_enable(int32_t module);

/*
  \brief       Soc disable device clock
  \param[in]   module   Clock module, defined in sys_clk.h, \ref clk_module_t
  \return      none
*/
void soc_clk_disable(int32_t module);

/*
  \brief       Soc reset module
  \param[in]   module   reset module, defined in power_manager.h, \ref rst_module_t
  \return      none
*/
void soc_module_reset(int32_t module);

/*
  \brief       Soc clear the sign of reset source
  \param[in]   module   Reset state module, defined in power_manager.h, \ref rst_state_t
  \return      none
*/
void soc_clear_reset_state(int32_t module);

/*
  \brief       Soc get the source of reset
  \param[in]   module   Reset state module, defined in power_manager.h, \ref rst_state_t
  \return      Reset state module
*/
uint32_t soc_get_reset_source(int32_t module);

/*
  \brief       Get CPU ID
  \return      CPU ID, the val is 0, 1, 2...
*/
uint32_t soc_get_cpu_id(void);

/**
  \brief       SOC Dcache clean & invalid by range.
  \return      None
*/
void soc_dcache_clean_invalid_range(unsigned long addr, uint32_t size);

/**
  \brief       SOC Dcache clean & invalid all.
  \return      None
*/
void soc_dcache_clean_invalid(void);
void soc_dcache_clean_invalid_all(void); /* please use soc_dcache_clean_invalid */

/**
  \brief       SOC Dcache invalid by range.
  \return      None
*/
void soc_dcache_invalid_range(unsigned long addr, uint32_t size);

/**
  \brief       SOC Dcache clean all.
  \return      None
*/
void soc_dcache_clean(void);

/**
  \brief       SOC Icache invalid all.
  \return      None
*/
void soc_icache_invalid(void);

/**
  \brief       SOC dma address remap.
  \return      Remaped address
*/
extern unsigned long soc_dma_address_remap(unsigned long addr);

/**
  \brief       SOC phys address to virt address.
  \param[in]   p_addr    phys address
  \return      Return virt address
*/
void *soc_phys_to_virt(unsigned long p_addr);

/**
  \brief       SOC virt address to phys address.
  \param[in]   v_addr    virt address
  \return      Return virt address
*/
unsigned long soc_virt_to_phys(void *v_addr);

#ifdef CONFIG_PM
/**
  \brief       SoC enter low-power mode, each chip's implementation is different
               called by csi_pm_enter_sleep
  \param[in]   mode        low-power mode
  \return      Error code
*/
csi_error_t soc_pm_enter_sleep(csi_pm_mode_t mode);

/**
  \brief       SoC the wakeup source.
  \param[in]   wakeup_num  Wakeup source num
  \param[in]   enable      Flag control the wakeup source is enable or not
  \return      Error code
*/
csi_error_t soc_pm_config_wakeup_source(uint32_t wakeup_num, bool enable);
#endif

csi_error_t soc_pm_config_gnss_dcu_mode(csi_pm_gnss_dcu_mode_t mode);

csi_error_t soc_pm_config_gnss_dcu_clock_gate(bool enable);

csi_error_t soc_pm_gnss_dcu_swreset(void);

csi_error_t soc_pm_config_mode_hold(csi_pm_mode_hold_t module);

#endif /* _DRV_PORTING_H_ */

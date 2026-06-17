


/******************************************************************************
 * @file     system.c
 * @brief    CSI Device System Source File
 * @version  V1.0
 * @date     02. Oct 2018
 ******************************************************************************/

#include <csi_config.h>
#include <soc.h>
#include <csi_core.h>
#include <drv/irq.h>
#include <drv/dma.h>
#include <drv/edma.h>
#include <drv/tick.h>
#include <drv/etb.h>
#include <drv/spiflash.h>
#include <drv/io.h>

csi_dma_t g_dma;
csi_edma_t g_edma;
csi_spiflash_t g_spiflash;

void section_data_copy(void);
void section_ram_code_copy(void);
void section_bss_clear(void);

void enable_isaee(void)
{
    uint32_t mxstatus = __get_MXSTATUS();
    mxstatus |= ((uint32_t)1U << 22U);
    __set_MXSTATUS(mxstatus);
}

static void sys_dma_init(void)
{
    csi_dma_init(&g_dma, 0);
    csi_edma_init(&g_edma, 0);
}

static void cache_init(void)
{
    csi_dcache_enable();
    csi_icache_enable();
}

static void section_init(void)
{
#ifdef CONFIG_XIP
    section_data_copy();
    section_ram_code_copy();
    csi_dcache_clean();
    csi_icache_invalid();
#endif

    section_bss_clear();
}

static void clic_init(void)
{
    int i;

//    CLIC->CLICCFG = 0x4UL;
    /* get interrupt level from info */
    CLIC->CLICCFG = (((CLIC->CLICINFO & CLIC_INFO_CLICINTCTLBITS_Msk) >> CLIC_INFO_CLICINTCTLBITS_Pos) << CLIC_CLICCFG_NLBIT_Pos);

    for (i = 0; i < CONFIG_IRQ_NUM; i++) {
        CLIC->CLICINT[i].IP = 0U;
        CLIC->CLICINT[i].ATTR = 1U; /* use vector interrupt */
    }

    /* config csitimer tick irq priority */
    csi_vic_set_prio(DW_TIMER0_IRQn, 3U);

    /* tspend use positive interrupt */
    CLIC->CLICINT[Machine_Software_IRQn].ATTR = 0x3U;
    csi_irq_enable((uint32_t)Machine_Software_IRQn);
}

static void interrupt_init(void)
{
    clic_init();
#ifdef CONFIG_KERNEL_NONE
    __enable_excp_irq();
#endif
}

static void sys_spiflash_init(void)
{
    csi_spiflash_qspi_init(&g_spiflash, 0U, NULL);
    csi_spiflash_config_data_line(&g_spiflash, SPIFLASH_DATA_4_LINES);
	csi_spiflash_frequence(&g_spiflash, 51000000U);
	//csi_spiflash_frequence(&g_spiflash, 20000000U);
}

/**
  * @brief  initialize the system
  *         Initialize the psr and vbr.
  * @param  None
  * @return None
  */
void SystemInit(void)
{
    enable_isaee();

    cache_init();

    section_init();

    csi_etb_init();

    sys_dma_init();

    interrupt_init();

    soc_set_sys_freq(CPU_204MHZ);

    csi_tick_init();
    sys_spiflash_init();

}

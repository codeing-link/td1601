

/******************************************************************************
 * @file     ck_efusec_v2.c
 * @brief    CSI Source File for efusec Driver
 * @version  V1.0
 * @date     22. Mar 2019
 ******************************************************************************/

#include <csi_config.h>
#include <stdio.h>
#include <wj_efuse_ll.h>
#include <drv/efuse.h>

/**
  \brief       Initialize EFUSEC Interface. 1. Initializes the resources needed for the EFUSEC interface
  \param[in]   idx device id
  \param[in]   cb_event  Pointer to \ref efuse_event_cb_t
  \return      pointer to efuse handle
*/
csi_error_t csi_efuse_init(csi_efuse_t *efuse, uint32_t idx)
{
    CSI_PARAM_CHK(efuse, CSI_ERROR);
    csi_error_t ret = CSI_OK;

    if (target_get(DEV_WJ_EFUSE_TAG, (uint32_t)idx, &efuse->dev)) {
        ret = CSI_ERROR;
    }


    efuse->info.start = 0U;
    efuse->info.size = WJ_EFUSE_MEM_SIZE;

    return ret;
}

/**
  \brief       De-initialize EFUSEC Interface. stops operation and releases the software resources used by the interface
  \param[in]   handle  efuse handle to operate.
  \return      error code
*/
void csi_efuse_uninit(csi_efuse_t *efuse)
{
    CSI_PARAM_CHK_NORETVAL(efuse);
}

/**
  \brief       Read data from Efuse.
  \param[in]   handle  efuse handle to operate.
  \param[in]   addr  Data address.
  \param[out]  data  Pointer to a buffer storing the data read from Efuse.
  \param[in]   size   Number of data items to read.
  \return      number of data items read or error code
*/
int32_t csi_efuse_read(csi_efuse_t *efuse, uint32_t addr, void *data, uint32_t size)
{
    CSI_PARAM_CHK(efuse, CSI_ERROR);
    CSI_PARAM_CHK(data, CSI_ERROR);
    csi_error_t ret = CSI_OK;
    wj_efuse_regs_t *efuse_base = (wj_efuse_regs_t *)HANDLE_REG_BASE(efuse);

    if ((efuse->info.start > addr) || ((efuse->info.start + efuse->info.size) <= addr)
        || (efuse->info.start > ((addr + size) - 1U)) || ((efuse->info.start + efuse->info.size) < ((addr + size)))) {
        ret = CSI_ERROR;
    }

    return ((CSI_OK == ret) ? wj_efuse_read(efuse_base, addr, (uint8_t *)data, size) : (int32_t)CSI_ERROR);
}

/**
  \brief       Program data to Efuse.
  \param[in]   handle  efuse handle to operate.
  \param[in]   addr  Data address.
  \param[in]   data  Pointer to a buffer containing the data to be programmed to Efuse..
  \param[in]   size  Number of data items to program.
  \return      number of data items programmed or error code
*/
int32_t csi_efuse_program(csi_efuse_t *efuse, uint32_t addr, const void *data, uint32_t size)
{
    CSI_PARAM_CHK(efuse, CSI_ERROR);
    CSI_PARAM_CHK(data, CSI_ERROR);
    csi_error_t ret = CSI_OK;
    wj_efuse_regs_t *efuse_base = (wj_efuse_regs_t *)HANDLE_REG_BASE(efuse);

    if ((efuse->info.start > addr) || ((efuse->info.start + efuse->info.size) <= addr)
        || (efuse->info.start > ((addr + size) - 1U)) || ((efuse->info.start + efuse->info.size) < ((addr + size)))) {
        ret = CSI_ERROR;
    }

    return ((CSI_OK == ret) ? wj_efuse_program(efuse_base, addr, (uint8_t *)data, size) : (int32_t)CSI_ERROR);
}

/**
  \brief       Get Efuse information.
  \param[in]   handle  efuse handle to operate.
  \return      Pointer to Efuse information \ref csi_efuse_info_t
*/
csi_error_t csi_efuse_get_info(csi_efuse_t *efuse, csi_efuse_info_t *info)
{
    CSI_PARAM_CHK(efuse, CSI_ERROR);
    CSI_PARAM_CHK(info, CSI_ERROR);

    info->start = efuse->info.start;
    info->size = efuse->info.size;

    return CSI_OK;
}



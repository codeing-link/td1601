

/******************************************************************************
 * @file     edma.h
 * @brief    header file for dma driver
 * @version  V1.0
 * @date     08. Apr 2020
 * @model    dma
 ******************************************************************************/

#ifndef _DRV_EDMA_H_
#define _DRV_EDMA_H_

#include <stdint.h>
#include <stdio.h>
#include <drv/common.h>
#include <drv/list.h>
#ifdef __cplusplus
extern "C" {
#endif

/****** DMA Event *****/
typedef enum {
    EDMA_EVENT_TRANSFER_DONE       = 0,  ///< transfer complete
    EDMA_EVENT_TRANSFER_HALF_DONE,       ///< transfer half done
    EDMA_EVENT_TRANSFER_ERROR,           ///< transfer error
} csi_edma_event_t;

typedef enum {
    EDMA_ADDR_INC    = 0,
    EDMA_ADDR_DEC,
    EDMA_ADDR_CONSTANT
} csi_edma_addr_inc_t;

typedef enum {
    EDMA_DATA_WIDTH_8_BITS  = 0,
    EDMA_DATA_WIDTH_16_BITS,
    EDMA_DATA_WIDTH_32_BITS
} csi_edma_data_width_t;

typedef enum {
    EDMA_MEM2MEM     = 0,
    EDMA_MEM2PERH,
    EDMA_PERH2MEM,
} csi_edma_trans_dir_t;

typedef struct {
    int8_t ctrl_idx;
    int8_t ch_idx;
} csi_edma_ch_desc_t;

typedef struct {
    uint16_t dev_tag;
    int8_t   ctrl_idx;
    const csi_edma_ch_desc_t *ch_list;
} csi_edma_ch_spt_list_t;
typedef struct {
    void                        *srcaddr;
    void                        *dstaddr;
    uint32_t                    length;
} csi_edma_link_list_item_t;

typedef struct {
    void                  *lli_buf;
    uint32_t               link_list_num;
} csi_edma_link_list_config_t;
typedef struct {
    csi_edma_addr_inc_t          src_inc;        ///< source address increment
    csi_edma_addr_inc_t          dst_inc;        ///< destination address increment
    csi_edma_data_width_t        src_tw;         ///< source transfer width in byte
    csi_edma_data_width_t        dst_tw;         ///< destination transfer width in byte
    csi_edma_trans_dir_t         trans_dir;      ///< transfer direction
    uint16_t                    handshake;      ///< handshake id
    uint16_t                    group_len;      ///< group transaction length (unit: bytes)
    uint8_t                     src_reload_en;  ///< 1:dma enable src addr auto reload, 0:disable
    uint8_t                     dst_reload_en;  ///< 1:dma enable dst addr auto reload, 0:disable
    uint8_t                     half_int_en;    ///< 1:dma enable half interrupt, 0: disable
    uint8_t                     link_list_en;     ///< 1:dma enable link list mode, 0: disable
    csi_edma_link_list_config_t  link_list_config; ///< link list config
} csi_edma_ch_config_t;


#ifndef DMA_LLI_SIZE
#define DMA_LLI_SIZE 48
#endif

#ifndef DEFINE_DESC_BUF
#define DEFINE_DESC_BUF(buf_name, num) uint8_t buf_name[num * EDMA_LLI_SIZE] __attribute__((aligned(64)));
#endif

typedef struct csi_edma_ch csi_edma_ch_t;

struct csi_edma_ch {
    void                *parent;
    int8_t              ctrl_id;
    int8_t              ch_id;
    void                (*callback)(csi_edma_ch_t *dma_ch, csi_edma_event_t event, void *arg);
    void                *arg;
    int16_t             etb_ch_id;
    slist_t             next;
    csi_edma_ch_t        *dma_ll_ch;
};

typedef struct {
    csi_dev_t           dev;
    slist_t             head;
    csi_edma_ch_t        *ch_list;
    uint32_t            alloc_status;
    uint32_t            ch_num;
    void                *priv;
} csi_edma_t;

/**
  \brief       Init dma controller
  \param[in]   dma     the dma controller operate handle
  \param[in]   ctrl_id  the dma controller id
  \return      csi error code
*/
csi_error_t csi_edma_init(csi_edma_t *dma, int8_t ctrl_id);

/**
  \brief       Uninit dma controller
  \param[in]   dma     the dma controller operate handle
  \return      none
*/
void csi_edma_uninit(csi_edma_t *dma);

/**
  \brief       Alloc a dma channel
  \param[in]   dma_ch   the dma channel operate handle
  \param[in]   ch_id    the channel id  of dma; when set -1, means auto alloc
  \param[in]   ctrl_id  the dma controller id; when set -1, means auto alloc
  \return      csi error code
*/
csi_error_t csi_edma_ch_alloc(csi_edma_ch_t *dma_ch, int8_t ch_id, int8_t ctrl_id);

/**
  \brief       Free a dma channel
  \param[in]   dma_ch   the dma channel operate handle
  \return      none
*/
void csi_edma_ch_free(csi_edma_ch_t *dma_ch);

/**
  \brief       Config a dma channel
  \param[in]   dma_ch   the dma channel operate handle
  \param[in]   config   the config structure for dma channel
  \return      csi error code
*/
csi_error_t csi_edma_ch_config(csi_edma_ch_t *dma_ch, csi_edma_ch_config_t *config);

/**
  \brief       Start a dma channel
  \param[in]   dma_ch       the dma channel operate handle
  \param[in]   psrcaddr     transfer source address
  \param[in]   pdstaddr     transfer destination address
  \param[in]   length       transfer length (unit: bytes), if set data_width is 16, the length should be the multiple of 2, and
                            if set data_width is 32, the length should be the multiple of 4
  \return      none
*/
void csi_edma_ch_start(csi_edma_ch_t *dma_ch, void *srcaddr, void *dstaddr, uint32_t length);

/**
  \brief       Stop a dma channel
  \param[in]   dma_ch       the dma channel operate handle
  \return      none
*/
void csi_edma_ch_stop(csi_edma_ch_t *dma_ch);

/**
  \brief       Attach the callback handler to DMA channel
  \param[in]   dma_ch     operate handle.
  \param[in]   callback   callback function
  \param[in]   arg        user can define it by himself as callback's param
  \return      error code
*/
csi_error_t csi_edma_ch_attach_callback(csi_edma_ch_t *dma_ch, void *callback, void *arg);

/**
  \brief       detach the callback handler
  \param[in]   uart  operate handle.
*/
void        csi_edma_ch_detach_callback(csi_edma_ch_t *dma_ch);

/**
  \brief       dma link list add item
  \param[in]   config  operate handle.
  \param[in]   item  new item.
  \return      error code
*/
csi_error_t csi_edma_add_link_list_item(csi_edma_ch_t *dma_ch, csi_edma_ch_config_t *config, csi_edma_link_list_item_t *item, uint8_t is_last_item);

/**
  \brief       enable dma power manage
  \param[in]   dma  dma handle to operate.
  \return      error code
*/
csi_error_t csi_edma_enable_pm(csi_edma_t *dma);

/**
  \brief       disable dma power manage
  \param[in]   dma  dma handle to operate.
*/
void        csi_edma_disable_pm(csi_edma_t *dma);

#ifdef __cplusplus
}
#endif

#endif /* _CSI_EDMA_H_ */
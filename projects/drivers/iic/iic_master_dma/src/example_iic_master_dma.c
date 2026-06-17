


/******************************************************************************
 * @file     example_iic_master_dma.c
 * @brief    the main function for the iic master dma driver
 * @version  V1.0
 * @date     5. Jan 2020
 ******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <csi_config.h>
#include <soc.h>
#include <board_config.h>
#include <board_init.h>
#include <drv/iic.h>
#include <drv/tick.h>
#include <drv/pin.h>

#define  IIC_SLAVE_ADDR            0x50
#define  EEPROM_PAGE_SIZE          0x20


csi_iic_t master_iic;
csi_dma_ch_t dma_ch_tx_handle;
static volatile uint8_t cb_transfer_flag = 0;

extern void mdelay(uint32_t ms);

static void iic_event_cb_fun(csi_iic_t *iic, csi_iic_event_t event, void *arg)
{
    if (event == IIC_EVENT_RECEIVE_COMPLETE) {
        cb_transfer_flag = 1;
    }

    if (event == IIC_EVENT_SEND_COMPLETE) {
        cb_transfer_flag = 1;
    }
}

void example_pin_iic_init(void)
{
    csi_pin_set_mux(EXAMPLE_PIN_IIC_SDA, EXAMPLE_PIN_IIC_SDA_FUNC);
    csi_pin_set_mux(EXAMPLE_PIN_IIC_SCL, EXAMPLE_PIN_IIC_SCL_FUNC);
}

int test_iic_eeprom(int32_t iic_idx)
{
    uint8_t write_data[EEPROM_PAGE_SIZE + 2] = {0x0, EEPROM_PAGE_SIZE, 0};///< the first two bytes is the eeprom memory address
    uint8_t read_data[EEPROM_PAGE_SIZE + 2] = {0x0, EEPROM_PAGE_SIZE, 0}; ///< the first two bytes is the eeprom memory address
    csi_error_t ret;
    uint32_t i;

    example_pin_iic_init();

    ret = csi_iic_init(&master_iic, iic_idx);

    if (ret != CSI_OK) {
        printf("csi_iic_initialize error\n");
        return -1;
    }

    ret = csi_iic_attach_callback(&master_iic, iic_event_cb_fun, NULL);

    if (ret != CSI_OK) {
        printf("csi_iic_attach_callback error\n");
        return -1;
    }

    ret = csi_iic_mode(&master_iic, IIC_MODE_MASTER);

    if (ret != CSI_OK) {
        printf("csi_iic_set_mode error\n");
        return -1;
    }

    ret = csi_iic_addr_mode(&master_iic, IIC_ADDRESS_7BIT);

    if (ret != CSI_OK) {
        printf("csi_iic_set_addr_mode error\n");
        return -1;
    }

    ret = csi_iic_speed(&master_iic, IIC_BUS_SPEED_STANDARD);

    if (ret != CSI_OK) {
        printf("csi_iic_set_speed error\n");
        return -1;
    }

    ret = csi_iic_link_dma(&master_iic, &dma_ch_tx_handle, NULL);    ///< use this interface connect iic with dma

    if (ret != CSI_OK) {
        printf("csi_iic_link_dma fail \n");
        return -1;
    }

    /* This is dma read write test */
    for (i = 2; i < sizeof(write_data); i++) {   ///< the first two bytes is the eeprom memory address
        write_data[i] = i;                       ///< init write_data value
    }

    cb_transfer_flag = 0;        ///< set transfer finish flag
    csi_iic_master_send_async(&master_iic, IIC_SLAVE_ADDR, write_data, sizeof(write_data));

    while (!cb_transfer_flag);   ///< wait transfer finish

    mdelay(50);

    cb_transfer_flag = 0;        ///< set transfer finish flag
    csi_iic_master_send_async(&master_iic, IIC_SLAVE_ADDR, read_data, 2);

    while (!cb_transfer_flag);   ///< wait transfer finish

    cb_transfer_flag = 0;        ///< set transfer finish flag
    csi_iic_master_receive_async(&master_iic, IIC_SLAVE_ADDR, read_data + 2, sizeof(read_data) - 2);

    while (!cb_transfer_flag);   ///< wait transfer finish

    printf("write_data,read_data:\n");

    for (i = 2; i < EEPROM_PAGE_SIZE + 2; i++) {
        printf("%x,%x \t", write_data[i], read_data[i]);

        if (((i + 3) % 4) == 0) {
            printf("\n");
        }

        if (write_data[i] != read_data[i]) {
            printf("\ntest at24c64 write and read failed\n");
            return -1;
        }
    }

    printf("\n test at24c64 send and receive dma passed\n");

    ret = csi_iic_link_dma(&master_iic, NULL, NULL);   ///< use this interface disconnect iic with dma

    if (ret != CSI_OK) {
        printf("csi_iic_unlink_dma fail \n");
        return -1;
    }

    csi_iic_detach_callback(&master_iic);

    csi_iic_uninit(&master_iic);

    printf("test_iic_eeprom successfully\n");
    return 0;
}

int example_iic(int32_t iic_idx)
{
    int ret;

    ret = test_iic_eeprom(iic_idx);

    if (ret < 0) {
        printf("test_iic_eeprom fail\n");
        CSI_EXAMPLE_RESULT(-1);
        return -1;
    }

    CSI_EXAMPLE_RESULT(1);
    return 0;
}

int main(void)
{
    board_init();

    return example_iic(EXAMPLE_IIC_IDX);
}


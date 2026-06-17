


/******************************************************************************
 * @file     example_iic_slave_inter.c
 * @brief    the main function for the IIC slave example
 * @version  V1.0
 * @date     5. Jan 2020
 ******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <csi_config.h>
#include <drv/iic.h>
#include <drv/tick.h>
#include <drv/pin.h>
#include <soc.h>
#include <board_config.h>
#include <board_init.h>

#define  IIC_SLAVE_ADDR            0x40
#define  TEST_DATA_SIZE            0x20

static csi_iic_t master_iic;
static csi_iic_t slave_iic;

csi_dma_ch_t dma_ch_tx_handle;
csi_dma_ch_t dma_ch_rx_handle;
csi_dma_ch_t dma_ch_slave_tx_handle;


static volatile uint8_t master_cb_transfer_done_flag = 0;
static volatile uint8_t slave_cb_transfer_done_flag = 0;

extern void mdelay(uint32_t ms);

static void master_iic_event_cb_fun(csi_iic_t *iic, csi_iic_event_t event, void *arg)
{
    if (event == IIC_EVENT_RECEIVE_COMPLETE) {
        master_cb_transfer_done_flag = 1;
    }

    if (event == IIC_EVENT_SEND_COMPLETE) {
        master_cb_transfer_done_flag = 1;
    }
}

static void slave_iic_event_cb_fun(csi_iic_t *iic, csi_iic_event_t event, void *arg)
{
    if (event == IIC_EVENT_RECEIVE_COMPLETE) {
        slave_cb_transfer_done_flag = 1;
    }

    if (event == IIC_EVENT_SEND_COMPLETE) {
        slave_cb_transfer_done_flag = 1;
    }
}

void example_pin_iic_init(void)
{
    csi_pin_set_mux(EXAMPLE_PIN_IIC_SDA,       EXAMPLE_PIN_IIC_SDA_FUNC);
    csi_pin_set_mux(EXAMPLE_PIN_IIC_SCL,       EXAMPLE_PIN_IIC_SCL_FUNC);
    csi_pin_set_mux(EXAMPLE_PIN_IIC_SLAVE_SDA, EXAMPLE_PIN_IIC_SLAVE_SDA_FUNC);
    csi_pin_set_mux(EXAMPLE_PIN_IIC_SLAVE_SCL, EXAMPLE_PIN_IIC_SLAVE_SCL_FUNC);
    csi_pin_mode(EXAMPLE_PIN_IIC_SDA, GPIO_MODE_PULLUP);
    csi_pin_mode(EXAMPLE_PIN_IIC_SCL, GPIO_MODE_PULLUP);
    csi_pin_mode(EXAMPLE_PIN_IIC_SLAVE_SDA, GPIO_MODE_PULLUP);
    csi_pin_mode(EXAMPLE_PIN_IIC_SLAVE_SCL, GPIO_MODE_PULLUP);

}

int test_iic_master_slave(void)
{
    printf("start test iic\n");
    uint8_t __attribute__((aligned(32))) write_data[TEST_DATA_SIZE] = {0x0};
    uint8_t __attribute__((aligned(32))) read_data[TEST_DATA_SIZE] = {0x0};
    uint8_t i = 0;
    int32_t ret;

    example_pin_iic_init();

    /* init iic master config */
    ret = csi_iic_init(&master_iic, EXAMPLE_IIC_IDX);

    if (ret != CSI_OK) {
        printf("master csi_iic_init error\n");
        return -1;
    }

    ret = csi_iic_attach_callback(&master_iic, master_iic_event_cb_fun, NULL);

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

    /* init iic slave config */
    ret = csi_iic_init(&slave_iic, EXAMPLE_IIC_SLAVE_IDX);

    if (ret != CSI_OK) {
        printf("slave csi_iic_init error\n");
        return -1;
    }

    ret = csi_iic_attach_callback(&slave_iic, slave_iic_event_cb_fun, NULL);

    if (ret != CSI_OK) {
        printf("csi_iic_attach_callback error\n");
        return -1;
    }

    ret = csi_iic_mode(&slave_iic, IIC_MODE_SLAVE);

    if (ret != CSI_OK) {
        printf("csi_iic_set_mode error\n");
        return -1;
    }

    ret = csi_iic_addr_mode(&slave_iic, IIC_ADDRESS_7BIT);

    if (ret != CSI_OK) {
        printf("csi_iic_set_addr_mode error\n");
        return -1;
    }

    ret = csi_iic_speed(&slave_iic, IIC_BUS_SPEED_STANDARD);

    if (ret != CSI_OK) {
        printf("csi_iic_set_speed error\n");
        return -1;
    }

    ret = csi_iic_own_addr(&slave_iic, IIC_SLAVE_ADDR);

    if (ret != CSI_OK) {
        printf("csi_iic_set_speed error\n");
        return -1;
    }

    ret = csi_iic_link_dma(&slave_iic, &dma_ch_slave_tx_handle, &dma_ch_rx_handle);    ///< use this interface connect iic with dma

    if (ret != CSI_OK) {
        printf("csi_iic_link_dma fail \n");
        return -1;
    }

    /* this is iic master inter send and iic slave inter receive test */
    for (i = 0; i < sizeof(write_data); i++) {
        write_data[i] = i;
    }

    for (i = 0; i < sizeof(read_data); i++) {
        read_data[i] = 0;
    }

    master_cb_transfer_done_flag = 0;
    slave_cb_transfer_done_flag  = 0;

    csi_iic_slave_receive_async(&slave_iic, read_data, sizeof(read_data));

    if (ret < 0) {
        printf("csi_iic_slave_receive error\n");
        return -1;
    }

    csi_iic_master_send_async(&master_iic, IIC_SLAVE_ADDR, write_data, sizeof(write_data));

    if (ret < 0) {
        printf("csi_iic_master_send error\n");
        return -1;
    }

    while (!slave_cb_transfer_done_flag);

    while (!master_cb_transfer_done_flag);

    for (i = 0; i < TEST_DATA_SIZE; i++) {
        printf("%x,%x \t", write_data[i], read_data[i]);

        if (((i + 3) % 4) == 0) {
            printf("\n");
        }

        if (write_data[i] != read_data[i]) {
            printf("\ntest master transmit and slave receive failed\n");
            return -1;
        }
    }

    printf("\ntest master send and slave receive success\n");

    /* this is iic master inter receive and iic slave inter send test */
    for (i = 0; i < sizeof(write_data); i++) {
        write_data[i] = i + 0xa;
    }

    for (i = 0; i < sizeof(read_data); i++) {
        read_data[i] = 0;
    }

    master_cb_transfer_done_flag = 0;
    slave_cb_transfer_done_flag  = 0;

    ret = csi_iic_slave_send_async(&slave_iic, write_data, sizeof(write_data));

    if (ret < 0) {
        printf("csi_iic_slave_send error\n");
        return -1;
    }

    ret = csi_iic_master_receive_async(&master_iic, IIC_SLAVE_ADDR, read_data, sizeof(read_data));

    if (ret < 0) {
        printf("csi_iic_master_receive error\n");
        return -1;
    }

    while (!master_cb_transfer_done_flag);

    while (!slave_cb_transfer_done_flag);

    for (i = 0; i < TEST_DATA_SIZE; i++) {
        printf("%x,%x \t", write_data[i], read_data[i]);

        if (((i + 3) % 4) == 0) {
            printf("\n");
        }

        if (write_data[i] != read_data[i]) {
            printf("\ntest slave send and master receive failed\n");
            return -1;
        }
    }

    printf("\ntest slave send and master receive success\n");

    ret = csi_iic_link_dma(&master_iic, NULL, NULL);   ///< use this interface disconnect iic with dma

    if (ret != CSI_OK) {
        printf("csi_iic_unlink_dma fail \n");
        return -1;
    }

    csi_iic_detach_callback(&master_iic);

    csi_iic_uninit(&master_iic);

    ret = csi_iic_link_dma(&slave_iic, NULL, NULL);   ///< use this interface disconnect iic with dma

    if (ret != CSI_OK) {
        printf("csi_iic_unlink_dma fail \n");
        return -1;
    }

    csi_iic_detach_callback(&slave_iic);

    csi_iic_uninit(&slave_iic);

    return 0;
}

int main(void)
{
    int32_t ret;
    board_init();
    ret = test_iic_master_slave();

    CSI_EXAMPLE_RESULT(ret);
    return ret;
}

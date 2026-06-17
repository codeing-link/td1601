


/******************************************************************************
 * @file     example_iic_memory_sync.c
 * @brief    the main function for the IIC memory sync driver
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

#define  IIC_SLAVE_ADDR             0x50
#define  EEPROM_PAGE_ADDR           0x0020
#define  EEPROM_PAGE_SIZE           0x20

static csi_iic_t master_iic;
static volatile uint8_t cb_transfer_flag = 0;

extern void mdelay(uint32_t ms);

void example_pin_iic_init(void)
{
    csi_pin_set_mux(EXAMPLE_PIN_IIC_SDA, EXAMPLE_PIN_IIC_SDA_FUNC);
    csi_pin_set_mux(EXAMPLE_PIN_IIC_SCL, EXAMPLE_PIN_IIC_SCL_FUNC);
}

int test_iic_eeprom(int32_t iic_idx)
{
    uint8_t write_data[EEPROM_PAGE_SIZE] = {0x0};
    uint8_t read_data[EEPROM_PAGE_SIZE] = {0x0};
    csi_error_t ret;
    uint32_t i, num;

    example_pin_iic_init();

    ret = csi_iic_init(&master_iic, iic_idx);

    if (ret != CSI_OK) {
        printf("csi_iic_initialize error\n");
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

    /* This is eeprom sync read sync write test */
    for (i = 0; i < sizeof(write_data); i++) {
        write_data[i] = i;    ///< init write_data value
    }

    num = csi_iic_mem_send(&master_iic, IIC_SLAVE_ADDR, EEPROM_PAGE_ADDR, IIC_MEM_ADDR_SIZE_16BIT, write_data, sizeof(write_data), 1000);

    if (num != sizeof(write_data)) {
        printf("csi_iic_mem_write error\n");
        return -1;
    }

    mdelay(50);

    num = csi_iic_mem_receive(&master_iic, IIC_SLAVE_ADDR, EEPROM_PAGE_ADDR, IIC_MEM_ADDR_SIZE_16BIT, read_data, sizeof(read_data), 1000);

    if (num != sizeof(read_data)) {
        printf("csi_iic_mem_read error\n");
        return -1;
    }

    printf("write_data,read_data:\n");

    for (i = 0; i < EEPROM_PAGE_SIZE; i++) {
        printf("%x,%x \t", write_data[i], read_data[i]);

        if (((i + 3) % 4) == 0) {
            printf("\n");
        }

        if (write_data[i] != read_data[i]) {
            printf("\ntest at24c64 write and read failed\n");
            return -1;
        }
    }

    printf("\n test at24c64 memory sync send and receive passed\n");

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


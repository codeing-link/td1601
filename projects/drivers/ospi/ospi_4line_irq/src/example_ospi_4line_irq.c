/******************************************************************************
 * @file     example_Ospi.c
 * @brief    the main function for the OSPI driver
 * @version  V1.0
 * @date     07. 5 2026
 ******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <soc.h>
#include <drv/ospi.h>
#include <drv/tick.h>
#include <drv/pin.h>
#include <csi_config.h>
#include <board_config.h>
#include <board_init.h>

#define EXAMPLE_FLASH_START_ADDRESS        0x60
#define EXAMPLE_FLASH_WRITE_SIZE           8

static csi_gpio_t gpio;
static csi_ospi_t ospi;
static csi_ospi_command_t cmd;
static volatile csi_ospi_event_t ospi_event; // 异步事件标志

static void example_pin_init(void)
{
    csi_pin_set_mux(PORTB, PIN4, PB4_SPI1_D0);
    csi_pin_set_mux(PORTB, PIN5, PB5_SPI1_D1);
    csi_pin_set_mux(PORTB, PIN6, PB6_SPI1_D2);
    csi_pin_set_mux(PORTB, PIN7, PB7_SPI1_D3);
    csi_pin_set_mux(PORTA, PIN28, PA28_SPI1_SCK);
}

static void soft_cs_pin_init(void)
{
    csi_pin_set_mux(EXAMPLE_PIN_SPI_CS_PORT,EXAMPLE_PIN_SPI_CS,   EXAMPLE_PIN_SPI_SOFT_CS_FUNC);
    csi_gpio_init(&gpio, EXAMPLE_PIN_SPI_CS_PORT);
    csi_gpio_dir(&gpio, EXAMPLE_PIN_SPI_CS, GPIO_DIRECTION_OUTPUT);
    csi_gpio_write(&gpio, EXAMPLE_PIN_SPI_CS, GPIO_PIN_HIGH);
}

// 异步回调
static void master_spi_event_cb(csi_ospi_t *ospi, csi_ospi_event_t event, void *arg)
{
    ospi_event = event;
}

static void init(void)
{
    example_pin_init();
    soft_cs_pin_init();
    csi_ospi_init(&ospi, 0);

    cmd.address.bus_width = OSPI_LINE_SINGLE;
    cmd.address.disabled = 0;
    cmd.address.size = OSPI_ADDRESS_24_BITS;
    cmd.alt.disabled = true;
    cmd.data.bus_width = OSPI_LINE_QUAD;
    cmd.data.disabled = 0;
    cmd.data.frame_len = 8;
    cmd.data.transfer_mode = OSPI_TRANSFER_SEND_ONLY;
    cmd.ddr_enable = 0;
    cmd.dummy_count = 0;
    cmd.instruction.disabled = 0;
    cmd.instruction.bus_width = OSPI_LINE_SINGLE;
    cmd.instruction.size = OSPI_INSTRUCTION_8_BITS;

    csi_ospi_config(&ospi, &cmd);
    csi_ospi_baud(&ospi, 1000000);
    csi_ospi_cp_format(&ospi, OSPI_FORMAT_CPOL0_CPHA0);

    // 绑定异步回调
    csi_ospi_attach_callback(&ospi, master_spi_event_cb, NULL);

    printf("ospi init ok\r\n");
}

uint8_t tx_data_test[EXAMPLE_FLASH_WRITE_SIZE] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
uint8_t rx_data_test[EXAMPLE_FLASH_WRITE_SIZE] = {0};

// 忙等待
void flash_wait_busy(void)
{
    uint8_t sta;
    cmd.instruction.bus_width = OSPI_LINE_SINGLE;
    cmd.instruction.size = OSPI_INSTRUCTION_8_BITS;
    cmd.instruction.value = 0x05;
    cmd.instruction.disabled = 0;

    cmd.address.disabled = 1;

    cmd.data.disabled = 0;
    cmd.data.frame_len = 8;
    cmd.data.bus_width = OSPI_LINE_SINGLE;
    cmd.data.transfer_mode = OSPI_TRANSFER_RECEIVE_ONLY;
    csi_ospi_config(&ospi, &cmd);
    do {
        
        csi_gpio_write(&gpio, EXAMPLE_PIN_SPI_CS, 0);
        csi_ospi_receive(&ospi, &sta, 1, 1000);
        csi_gpio_write(&gpio, EXAMPLE_PIN_SPI_CS, 1);
        
    } while(sta & 0x01); // BIT0 = 忙
}

// 写使能
void flash_write_enable(void)
{
    cmd.instruction.value = 0x06;
    cmd.instruction.bus_width  = OSPI_LINE_SINGLE;
    cmd.address.disabled = true;
    cmd.data.disabled = true;
    cmd.data.bus_width = OSPI_LINE_QUAD;
    cmd.data.transfer_mode     = OSPI_TRANSFER_SEND_ONLY; 
    csi_ospi_config(&ospi, &cmd);

    csi_gpio_write(&gpio, EXAMPLE_PIN_SPI_CS, 0);
    csi_ospi_send(&ospi, NULL, 0, 1000);
    csi_gpio_write(&gpio, EXAMPLE_PIN_SPI_CS, 1);
}

// 扇区擦除
void flash_sector_erase(void)
{
    flash_write_enable();

    cmd.instruction.bus_width = OSPI_LINE_SINGLE;
    cmd.instruction.size      = OSPI_INSTRUCTION_8_BITS;
    cmd.instruction.value     = 0x60;
    cmd.instruction.disabled  = false;
    cmd.address.disabled      = true;
    cmd.data.disabled         = true;
    cmd.alt.disabled          = true;

    csi_ospi_config(&ospi, &cmd);

    csi_gpio_write(&gpio, EXAMPLE_PIN_SPI_CS, 0);
    csi_ospi_send(&ospi, NULL, 0, 1000);
    csi_gpio_write(&gpio, EXAMPLE_PIN_SPI_CS, 1);
    flash_wait_busy();
}

int main(void)
{
    board_init();
    init();

    flash_sector_erase();
    printf("erase ok\r\n");

    flash_write_enable();

    // ==================== 3. 四线异步写 0x32 ====================
    cmd.instruction.value       = 0x32;
    cmd.instruction.bus_width   = OSPI_LINE_SINGLE;
    cmd.address.value           = EXAMPLE_FLASH_START_ADDRESS;
    cmd.address.bus_width       = OSPI_LINE_SINGLE;
    cmd.address.disabled        = false;
    cmd.data.disabled           = false;
    cmd.data.bus_width          = OSPI_LINE_QUAD;
    cmd.data.frame_len          = 0x8;
    cmd.data.transfer_mode      = OSPI_TRANSFER_SEND_ONLY;
    csi_ospi_config(&ospi, &cmd);

    ospi_event = -1;
    csi_gpio_write(&gpio, EXAMPLE_PIN_SPI_CS, 0);
    csi_ospi_send_async(&ospi, tx_data_test, EXAMPLE_FLASH_WRITE_SIZE);
    while(ospi_event != OSPI_EVENT_SEND_COMPLETE); // 等待异步完成
    csi_gpio_write(&gpio, EXAMPLE_PIN_SPI_CS, 1);

    printf("write ok\r\n");
    flash_wait_busy();

    // ==================== 4. 四线异步读 0xEB ====================
    cmd.instruction.value = 0xEB;
    cmd.address.value = EXAMPLE_FLASH_START_ADDRESS;
    cmd.address.bus_width = OSPI_LINE_QUAD;
    cmd.data.transfer_mode = OSPI_TRANSFER_RECEIVE_ONLY;
    cmd.data.bus_width = OSPI_LINE_QUAD;
    cmd.data.frame_len = 0x8;
    cmd.dummy_count        = 6;
    cmd.address.disabled = false;
    cmd.data.disabled = false;
    csi_ospi_config(&ospi, &cmd);

    ospi_event = -1;
    csi_gpio_write(&gpio, EXAMPLE_PIN_SPI_CS, 0);
    csi_ospi_receive_async(&ospi, rx_data_test, EXAMPLE_FLASH_WRITE_SIZE);
    while(ospi_event != OSPI_EVENT_RECEIVE_COMPLETE); // 等待异步完成
    csi_gpio_write(&gpio, EXAMPLE_PIN_SPI_CS, 1);

    printf("read ok\r\n");

    // ==================== 校验 ====================
    printf("Tx Data: ");
    for (int i = 0; i < EXAMPLE_FLASH_WRITE_SIZE; i++) {
        printf("0x%02X ", tx_data_test[i]);
    }
    printf("\r\n");

    printf("Rx Data: ");
    for (int i = 0; i < EXAMPLE_FLASH_WRITE_SIZE; i++) {
        printf("0x%02X ", rx_data_test[i]);
    }
    printf("\r\n");

    if (memcmp(tx_data_test, rx_data_test, EXAMPLE_FLASH_WRITE_SIZE) == 0) {
        printf("====== QSPI ASYNC TEST PASS ======\r\n");
    } else {
        printf("====== QSPI ASYNC TEST FAIL ======\r\n");
    }

    while (1);
    return 0;
}
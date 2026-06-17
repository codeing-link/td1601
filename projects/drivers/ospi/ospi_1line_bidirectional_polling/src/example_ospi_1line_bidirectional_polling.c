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

static csi_gpio_t gpio;
static csi_ospi_t  ospi;
static csi_ospi_command_t cmd;
static uint8_t tx_data[8] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
static uint8_t rx_data[8] = {0};

static void example_pin_init(void)
{
    csi_pin_set_mux(PORTB, PIN4, PB4_SPI1_D0);
    csi_pin_set_mux(PORTB, PIN5, PB5_SPI1_D1);
    csi_pin_set_mux(PORTA, PIN28, PA28_SPI1_SCK);
}
static void uninit()
{
    csi_ospi_detach_callback(&ospi);
    csi_ospi_uninit(&ospi);
}

static void soft_cs_pin_init(void)
{
    csi_pin_set_mux(EXAMPLE_PIN_SPI_CS_PORT,EXAMPLE_PIN_SPI_CS,   EXAMPLE_PIN_SPI_SOFT_CS_FUNC);
    csi_gpio_init(&gpio, EXAMPLE_PIN_SPI_CS_PORT);
    csi_gpio_dir(&gpio, EXAMPLE_PIN_SPI_CS, GPIO_DIRECTION_OUTPUT);
    csi_gpio_write(&gpio, EXAMPLE_PIN_SPI_CS, GPIO_PIN_HIGH);
}

static int init(void)
{
    int ret;

    example_pin_init();
    soft_cs_pin_init();

    ret = csi_ospi_init(&ospi, 0);
    if (ret != CSI_OK) 
    {
        return -1;
    }
    cmd.address.bus_width = OSPI_LINE_QUAD,
    cmd.address.disabled = true,
    cmd.address.size = OSPI_ADDRESS_24_BITS,
    cmd.alt.disabled = true,
    cmd.data.bus_width = OSPI_LINE_SINGLE,
    cmd.data.disabled = false,
    cmd.data.frame_len = 0x8,
    cmd.data.transfer_mode = OSPI_TRANSFER_SEND_RECEIVE,
    cmd.ddr_enable = 0,
    cmd.dummy_count = 0,
    cmd.instruction.disabled = true,
    cmd.instruction.bus_width = OSPI_LINE_SINGLE,
    cmd.instruction.size = OSPI_INSTRUCTION_8_BITS;
    csi_ospi_config(&ospi, &cmd);
    csi_ospi_baud(&ospi, 1000000);
    csi_ospi_cp_format(&ospi, OSPI_FORMAT_CPOL0_CPHA0);
    return 0;
}

static int ioctl(uint32_t value)
{
    csi_gpio_write(&gpio, EXAMPLE_PIN_SPI_CS, value);
    return 0;
}

int main(void)
{
    int ret = 0;
    board_init();
    ret = init();
    if (ret != 0) 
    {
        printf("OSPI init failed!\n");
        return -1;
    }
    csi_ospi_send_receive(&ospi, tx_data, rx_data, 8, 1000);
        printf("tx_data: ");
    for(int i = 0; i < 8; i++) {
        printf("0x%02X ", tx_data[i]);
    }
    printf("\n");

    printf("rx_data: ");
    for(int i = 0; i < 8; i++) {
        printf("0x%02X ", rx_data[i]);
    }
    printf("\n");

    if(memcmp(tx_data, rx_data, 8) == 0) {
        printf("=== OSPI 1-line bidirectional test passed ===\n");
    } else {
        printf("=== OSPI 1-line bidirectional test failed ===\n");
    }
    return 0;

}
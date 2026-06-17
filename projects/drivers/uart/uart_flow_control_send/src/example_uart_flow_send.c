/******************************************************************************
 * @file     example_uart_flow_send.c
 * @brief    UART1 hardware flow control (RTS/CTS) send test based on official driver
 * @version  V1.0
 * @date     20. Mar 2026
 ******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <drv/uart.h>
#include <drv/tick.h>
#include <drv/pin.h>
#include <soc.h>
#include <board_init.h>
#include <board_config.h>

/* -------------------------- 核心配置 -------------------------- */
#define EXAMPLE_UART_IDX1       1               // 串口1
#define EXAMPLE_UART_BAUDRATE   115200          // 波特率
#define TEST_DATA_LEN           1024            // 测试数据长度
#define SEND_TIMEOUT_MS         10000           // 发送超时
#define SEND_DELAY_MS           8000            // 两次发送间隔
#define LOOP_SEND_COUNT         3               // 循环发送次数

/* -------------------------- 全局变量 -------------------------- */
static csi_uart_t g_uart;                       // 串口1句柄
static uint8_t tx_test_data[TEST_DATA_LEN];     // 发送缓冲区
static uint8_t uart_inited = 0;                 // 初始化标志

/* -------------------------- 宏定义 -------------------------- */
#define UART_CHECK_RETURN(ret)                      \
    do {                                        \
        if (ret != CSI_OK) {                    \
            printf("UART1 Error: %d (line %d)\n", ret, __LINE__); \
            return -1;                          \
        }                                       \
    } while(0);

/**
 * @brief 生成1024字节测试数据
 * @param buf 输出缓冲区
 * @param type 0=可打印ASCII，1=递增字节
 */
void generate_test_data_1024(uint8_t *buf, uint8_t type)
{
    if (buf == NULL) {
        printf("Error: buffer is NULL!\n");
        return;
    }

    switch (type) {
        case 0: // 可打印ASCII
            for (uint32_t i = 0; i < TEST_DATA_LEN; i++) {
                buf[i] = 0x20 + (i % 95);
            }
            printf("Generate ASCII test data (0x20-0x7E)\n");
            break;
        case 1: // 递增字节
            for (uint32_t i = 0; i < TEST_DATA_LEN; i++) {
                buf[i] = i % 256;
            }
            printf("Generate incremental test data (0x00-0xFF)\n");
            break;
        default:
            generate_test_data_1024(buf, 0);
            break;
    }

    // 打印前16字节验证
    printf("First 16 bytes: ");
    for (int i = 0; i < 16; i++) {
        printf("%02X ", buf[i]);
    }
    printf("\n");
}

/**
 * @brief 串口引脚初始化
 */
void example_pin_uart_init(void)
{
    csi_pin_set_mux(EXAMPLE_PIN_UART_TX1_PORT,EXAMPLE_PIN_UART_TX1, EXAMPLE_PIN_UART_TX_FUNC1);
    csi_pin_set_mux(EXAMPLE_PIN_UART_RX1_PORT,EXAMPLE_PIN_UART_RX1, EXAMPLE_PIN_UART_RX_FUNC1);
    csi_pin_set_mux(EXAMPLE_PIN_UART_RTS1_PORT,EXAMPLE_PIN_UART_RTS1, EXAMPLE_PIN_UART_RTS_FUNC1);
    csi_pin_set_mux(EXAMPLE_PIN_UART_CTS1_PORT,EXAMPLE_PIN_UART_CTS1, EXAMPLE_PIN_UART_CTS_FUNC1);
    printf("UART1 pin mux init success\n");
}

/**
 * @brief 初始化UART1
 */
int uart1_init(void)
{
    if (uart_inited) {
        printf("UART1 already inited, skip\n");
        return 0;
    }

    csi_error_t ret;
    // 1. 初始化UART1
    ret = csi_uart_init(&g_uart, EXAMPLE_UART_IDX1);
    UART_CHECK_RETURN(ret);

    // 2. 配置波特率
    ret = csi_uart_baud(&g_uart, EXAMPLE_UART_BAUDRATE);
    UART_CHECK_RETURN(ret);

    // 3. 配置数据格式（8N1）
    ret = csi_uart_format(&g_uart, UART_DATA_BITS_8, UART_PARITY_NONE, UART_STOP_BITS_1);
    UART_CHECK_RETURN(ret);

    uart_inited = 1;
    printf("UART1 init success (115200 8N1)\n");
    return 0;
}

/**
 * @brief 测试1：开启硬件流控发送
 */
int uart_send_with_flow(void)
{
    printf("\n=========================================\n");
    printf("Test 1: Send with HW RTS/CTS Flow Control\n");
    printf("=========================================\n");

    // 1. 初始化UART1
    if (uart1_init() != 0) {
        return -1;
    }

    // 2. 启用硬件流控
    csi_error_t ret = csi_uart_flowctrl(&g_uart, UART_FLOWCTRL_RTS_CTS);
    UART_CHECK_RETURN(ret);
    printf("UART1 HW flow control (RTS+CTS) enabled\n");

    // 3. 生成测试数据
    generate_test_data_1024(tx_test_data, 1);

    // 4. 循环发送3次
    for (int i = 0; i < LOOP_SEND_COUNT; i++) {
        printf("\n=== Send round %d/%d ===\n", i+1, LOOP_SEND_COUNT);
        int32_t send_len = csi_uart_send(&g_uart, tx_test_data, TEST_DATA_LEN, SEND_TIMEOUT_MS);
        
        if (send_len == TEST_DATA_LEN) {
            printf("Send success: %d bytes\n", send_len);
        } else if (send_len > 0) {
            printf("Send partial: %d/%d bytes\n", send_len, TEST_DATA_LEN);
        } else {
            printf("Send failed: %d (timeout/hardware error)\n", send_len);
            return -1;
        }

        // 每次发送后延时1秒，避免发送过快
        mdelay(1000);
    }

    printf("\nTest 1 (with flow) completed\n");
    return 0;
}

/**
 * @brief 测试2：关闭硬件流控发送（循环发送3次）
 */
int uart_send_no_flow(void)
{
    printf("\n=========================================\n");
    printf("Test 2: Send without HW Flow Control\n");
    printf("=========================================\n");

    // 1. 确保UART1已初始化
    if (uart1_init() != 0) {
        return -1;
    }

    // 2. 关闭硬件流控
    csi_error_t ret = csi_uart_flowctrl(&g_uart, UART_FLOWCTRL_NONE);
    UART_CHECK_RETURN(ret);
    printf("UART1 HW flow control disabled\n");

    // 3. 循环发送3次（和开启流控保持一致）
    for (int i = 0; i < LOOP_SEND_COUNT; i++) {
        printf("\n=== Send round %d/%d ===\n", i+1, LOOP_SEND_COUNT);
        int32_t send_len = csi_uart_send(&g_uart, tx_test_data, TEST_DATA_LEN, SEND_TIMEOUT_MS);
        
        if (send_len == TEST_DATA_LEN) {
            printf("Send success: %d bytes\n", send_len);
        } else if (send_len > 0) {
            printf("Send partial: %d/%d bytes\n", send_len, TEST_DATA_LEN);
        } else {
            printf("Send failed: %d\n", send_len);
            return -1;
        }

        mdelay(1000);
    }

    printf("\nTest 2 (no flow) completed\n");
    return 0;
}

/**
 * @brief 主函数入口
 */
int main(void)
{
    // 1. 板级初始化 + 引脚初始化（必须最先执行）
    board_init();
    example_pin_uart_init();

    printf("Board initialized\n");
    printf("=========================================\n");
    printf("UART1 Flow Control Send Test \n");
    printf("=========================================\n");

    // 2. 先测开启流控发送
    uart_send_with_flow();

    // 3. 延时8秒
    printf("\nWait %dms for receiver to switch test...\n", SEND_DELAY_MS);
    mdelay(SEND_DELAY_MS);

    // 4. 再测关闭流控发送
    uart_send_no_flow();

    // 5. 死循环
    printf("\nAll send tests completed! Enter idle loop...\n");
    while (1) {
        mdelay(1000);
    }

    return 0;
}
/******************************************************************************
 * @file     example_uart_flow_receive.c
 * @brief    UART1 hardware flow control (RTS/CTS) receive test based on official driver
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

/* -------------------------- 配置参数 -------------------------- */
#define EXAMPLE_UART_IDX1        1              // 串口1
#define EXAMPLE_UART_BAUDRATE   115200          // 串口1波特率
#define RECV_BUF_LEN            1024            // 接收缓冲区长度（字节）
#define RECV_TIMEOUT_MS         -1              // 无限阻塞

/* -------------------------- 全局变量 -------------------------- */
static csi_uart_t g_uart;                       // 串口1设备句柄
static uint8_t recv_buf[RECV_BUF_LEN];          // 接收缓冲区
static uint32_t recv_bytes = 0;                 // 实际接收字节数
static uint8_t  tx_test_data[RECV_BUF_LEN];     // 发送对照缓冲区
/* -------------------------- 宏定义 -------------------------- */
#define UART_CHECK_RETURN(ret)                      \
    do {                                        \
        if (ret != CSI_OK) {                    \
            printf("UART1 Error: %d (at line %d)\n", ret, __LINE__); \
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
            for (uint32_t i = 0; i < RECV_BUF_LEN; i++) {
                buf[i] = 0x20 + (i % 95);
            }
            printf("Generate ASCII test data (0x20-0x7E)\n");
            break;
        case 1: // 递增字节
            for (uint32_t i = 0; i < RECV_BUF_LEN; i++) {
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
 * @brief 字节级对比接收数据和原始发送数据
 * @param rx_buf 接收缓冲区
 * @param rx_len 接收长度
 * @param tx_buf 原始发送数据缓冲区
 * @param tx_len 发送长度
 * @return 0: 完全一致，-1: 长度不一致，>0: 不一致的字节数
 */
static int compare_data_byte_by_byte(uint8_t *rx_buf, uint32_t rx_len, 
                                     uint8_t *tx_buf, uint32_t tx_len)
{
    if (rx_buf == NULL || tx_buf == NULL) {
        printf("[COMPARE] Error: buffer is NULL\n");
        return -2;
    }

    // 第一步：校验长度
    if (rx_len != tx_len) {
        printf("[COMPARE] Length mismatch: rx=%d bytes, tx=%d bytes\n", rx_len, tx_len);
        return -1;
    }

    // 第二步：逐字节对比
    uint32_t mismatch_count = 0;
    const uint32_t MAX_ERROR_SHOW = 10; // 最多显示前10个不一致的字节
    uint32_t error_shown = 0;

    printf("\n[COMPARE] Start byte-by-byte comparison (total %d bytes)...\n", tx_len);
    for (uint32_t i = 0; i < tx_len; i++) {
        if (rx_buf[i] != tx_buf[i]) {
            mismatch_count++;
            // 显示前10个不一致的字节（避免日志刷屏）
            if (error_shown < MAX_ERROR_SHOW) {
                printf("[COMPARE] Mismatch at byte %d: rx=0x%02X, tx=0x%02X\n", 
                       i, rx_buf[i], tx_buf[i]);
                error_shown++;
            }
        }
    }

    // 输出对比结果
    if (mismatch_count == 0) {
        printf("[COMPARE] ✅ All %d bytes match perfectly (no error)\n", tx_len);
        return 0;
    } else {
        printf("[COMPARE] ❌ Total mismatch bytes: %d/%d (%.2f%% error)\n", 
               mismatch_count, tx_len, (float)mismatch_count/tx_len*100);
        return mismatch_count;
    }
}
/**
 * @brief 串口引脚初始化（必须在UART初始化前执行）
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
 * @brief 打印接收的数据（仅打印可打印ASCII字符）
 * @param buf 数据缓冲区
 * @param len 数据长度
 */
static void print_recv_data(uint8_t *buf, uint32_t len)
{
    if (buf == NULL || len == 0) {
        printf("[UART1 RECV] Invalid buffer or length\n");
        return;
    }

    printf("\n[UART1 RECV] Received data (printable chars):\n");
    printf("------------------------------------------------\n");
    for (uint32_t i = 0; i < len; i++) {
        // 只打印可打印ASCII字符
        if (buf[i] >= 0x20 && buf[i] <= 0x7E) {
            printf("%c", buf[i]);
        } else {
            printf(".");
        }
        // 每行打印64个字符，便于查看
        if ((i + 1) % 64 == 0) {
            printf("\n");
        }
    }
    printf("\n------------------------------------------------\n");
    printf("[UART1 RECV] Total received: %d bytes\n", len);
}

/**
 * @brief 串口1流控接收测试（开启硬件RTS/CTS）
 * @return 0: 成功，-1: 失败
 */
int example_uart_flow_receive(void)
{
    csi_error_t ret;
    int32_t recv_part1 = 0, recv_part2 = 0;

    /* 初始化全局变量 */
    memset(recv_buf, 0, RECV_BUF_LEN);
    recv_bytes = 0;

    printf("\n=========================================\n");
    printf("Test 1: UART1 Receive with HW RTS/CTS Flow Control\n");
    printf("=========================================\n");

    /* 1. 初始化串口1设备 */
    ret = csi_uart_init(&g_uart, EXAMPLE_UART_IDX1);
    UART_CHECK_RETURN(ret);
    printf("UART1 init success \n");

    /* 2. 配置串口1波特率 */
    ret = csi_uart_baud(&g_uart, EXAMPLE_UART_BAUDRATE);
    UART_CHECK_RETURN(ret);
    printf("UART1 baudrate set to: %d\n", EXAMPLE_UART_BAUDRATE);

    /* 3. 配置串口1数据格式（8N1） */
    ret = csi_uart_format(&g_uart, UART_DATA_BITS_8, UART_PARITY_NONE, UART_STOP_BITS_1);
    UART_CHECK_RETURN(ret);
    printf("UART1 format set to: 8 data bits, no parity, 1 stop bit\n");

    /* 4. 启用UART1硬件流控（RTS+CTS）- 核心配置 */
    ret = csi_uart_flowctrl(&g_uart, UART_FLOWCTRL_RTS_CTS);
    UART_CHECK_RETURN(ret);
    printf("UART1 flow control enabled: RTS + CTS\n");
    generate_test_data_1024(tx_test_data, 1);
    /* 5. 分两次接收（修复指针错误：&recv_buf[512] 而非 recv_buf[512]） */
    printf("Start receive part 1 (512 bytes, infinite timeout)...\n");
    recv_part1 = csi_uart_receive(&g_uart, recv_buf, 512, RECV_TIMEOUT_MS);
    if (recv_part1 < 0) 
    {
        printf("UART1 receive part1 failed: %d\n", recv_part1);
        csi_uart_uninit(&g_uart);
        return -1;
    }

    mdelay(5); // 短延时，模拟接收间隔

    printf("Start receive part 2 (512 bytes, infinite timeout)...\n");
    recv_part2 = csi_uart_receive(&g_uart, &recv_buf[512], 512, RECV_TIMEOUT_MS); 
    if (recv_part2 < 0) 
    {
        printf("UART1 receive part2 failed: %d\n", recv_part2);
        csi_uart_uninit(&g_uart);
        return -1;
    }

    /* 6. 计算总接收字节数 */
    recv_bytes = recv_part1 + recv_part2;

    /* 7. 打印接收结果 */
    if (recv_bytes > 0) 
    {
        print_recv_data(recv_buf, recv_bytes);
    } 
    else 
    {
        printf("[UART1 RECV] No data received\n");
    }

    /* 8. 反初始化UART1 */
    csi_uart_uninit(&g_uart);

    /* 9. 返回测试结果 */
    int compare_result = compare_data_byte_by_byte(recv_buf, recv_bytes, tx_test_data, RECV_BUF_LEN);
    // 根据对比结果更新测试结论
    if (compare_result == 0) 
    {
        printf("\n=== UART1 flow control receive test SUCCESS (100%% data integrity) ===\n");
    } 
    else 
    {
        printf("\n=== UART1 flow control receive test FAILED (data mismatch) ===\n");
    }

}

/**
 * @brief 串口1普通接收测试
 * @return 0: 成功，-1: 失败
 */
int example_uart_receive(void)
{
    csi_error_t ret;
    int32_t recv_part1 = 0, recv_part2 = 0;

    /* 初始化全局变量 */
    memset(recv_buf, 0, RECV_BUF_LEN);
    recv_bytes = 0;

    printf("\n=========================================\n");
    printf("Test 2: UART1 Receive without HW Flow Control\n");
    printf("=========================================\n");

    /* 1. 初始化串口1设备 */
    ret = csi_uart_init(&g_uart, EXAMPLE_UART_IDX1);
    UART_CHECK_RETURN(ret);
    printf("UART1 init success \n");

    /* 2. 配置串口1波特率 */
    ret = csi_uart_baud(&g_uart, EXAMPLE_UART_BAUDRATE);
    UART_CHECK_RETURN(ret);
    printf("UART1 baudrate set to: %d\n", EXAMPLE_UART_BAUDRATE);

    /* 3. 配置串口1数据格式*/
    ret = csi_uart_format(&g_uart, UART_DATA_BITS_8, UART_PARITY_NONE, UART_STOP_BITS_1);
    UART_CHECK_RETURN(ret);
    printf("UART1 format set to: 8 data bits, no parity, 1 stop bit\n");

    /* 4. 关闭UART1硬件流控 - 核心配置 */
    ret = csi_uart_flowctrl(&g_uart, UART_FLOWCTRL_NONE);
    UART_CHECK_RETURN(ret);
    printf("UART1 flow control disabled: NONE\n");

    /* 5. 分两次接收 */
    printf("Start receive part 1 (512 bytes, infinite timeout)...\n");
    recv_part1 = csi_uart_receive(&g_uart, recv_buf, 512, RECV_TIMEOUT_MS);
    if (recv_part1 < 0) 
    {
        printf("UART1 receive part1 failed: %d\n", recv_part1);
        csi_uart_uninit(&g_uart);
        return -1;
    }

    mdelay(5);

    printf("Start receive part 2 (512 bytes, infinite timeout)...\n");
    recv_part2 = csi_uart_receive(&g_uart, &recv_buf[512], 512, RECV_TIMEOUT_MS); // 修复指针
    if (recv_part2 < 0) 
    {
        printf("UART1 receive part2 failed: %d\n", recv_part2);
        csi_uart_uninit(&g_uart);
        return -1;
    }

    /* 6. 计算总接收字节数 */
    recv_bytes = recv_part1 + recv_part2;

    /* 7. 打印接收结果 */
    if (recv_bytes > 0) 
    {
        print_recv_data(recv_buf, recv_bytes);
    } 
    else 
    {
        printf("[UART1 RECV] No data received\n");
    }

    csi_uart_uninit(&g_uart);

    /* 9. 返回测试结果 */
    int compare_result = compare_data_byte_by_byte(recv_buf, recv_bytes, tx_test_data, RECV_BUF_LEN);
    // 根据对比结果更新测试结论
    if (compare_result == 0) 
    {
        printf("\n=== UART1 flow control receive test SUCCESS (100%% data integrity) ===\n");
    } 
    else 
    {
        printf("\n=== UART1 flow control receive test FAILED (data mismatch) ===\n");
    }

}

/**
 * @brief 主函数入口
 */
int main(void)
{
    /* 板级初始化 */
    board_init();
    /* 引脚初始化 */
    example_pin_uart_init();

    printf("Board initialized\n");
    printf("=========================================\n");
    printf("UART1 Flow Control Receive Test\n");
    printf("=========================================\n");

    /* 运行串口1流控接收测试 */
    example_uart_flow_receive();

    printf("=========================================\n");
    printf("UART1 normal Receive Test\n");
    printf("=========================================\n");

    /* 运行串口1普通接收测试 */
    example_uart_receive();

    printf("\nAll tests completed, enter idle loop...\n");
    while (1) {
        mdelay(1000);
    }

    return 0;
}
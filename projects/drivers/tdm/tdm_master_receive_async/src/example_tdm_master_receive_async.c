

/******************************************************************************
 * @file     example_tdm.c
 * @brief    the main function for the TDM driver
 * @version  V1.0
 * @date     18. Jan 2021
 ******************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include "board_config.h"
#include "board_init.h"
#include <drv/uart.h>
#include <drv/timer.h>
#include <drv/tdm.h>
#include <drv/iic.h>
#include <drv/pin.h>
#include <soc.h>
#include <es8311.h>




typedef struct __attribute__((packed))
{
    char chunkId[4];    // "RIFF"
    uint32_t chunkSize; // Size of the rest of this chunk
    char format[4];     // "WAVE"
} RiffChunk;

typedef struct __attribute__((packed))
{
    char subChunk1Id[4];    // "fmt "
    uint32_t subChunk1Size; // Size of the rest of this subchunk (usually 16 for PCM)
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
} WavFmtChunk;

typedef struct __attribute__((packed))
{
    char Data1Id[4]; // "fmt "
    uint32_t DataSize;
} WavDataChunk;



csi_tdm_t s_tdm;

csi_dma_ch_t dma_ch_rx_handle0;
static csi_iic_t master_iic;
#define TDM_RX_BUF_SIZE 2048
static uint32_t tdm_rx_buf0[TDM_RX_BUF_SIZE];

csi_ringbuffer_t rx_ring_buffer0;

#define READ_BUF_SIZE 16
int16_t read_data0[READ_BUF_SIZE];

int16_t read_data1[READ_BUF_SIZE];

int16_t dest_buf[READ_BUF_SIZE];

uint8_t ping, pong;
int16_t *data_ptr;

volatile uint8_t cb_master_transfer_flag = 0;
uint8_t     end_ping, end_pong;
static void tdm_master_event_cb_fun(csi_tdm_t *tdm, csi_tdm_event_t event, void *arg)
{
    if (event == TDM_EVENT_RECEIVE_COMPLETE) {
        cb_master_transfer_flag = 1;
    }

    if (ping) {
        ping = 0;
        pong = 1;
        data_ptr = read_data0;
        end_pong = 1;
        end_ping = 0;
    } else if (pong) {
        pong = 0;
        ping = 1;
        data_ptr = read_data1;
        end_ping = 1;
        end_pong = 0;
    }
    csi_tdm_receive_async(&s_tdm, data_ptr, sizeof(read_data0));
}

void example_pin_tdm_init(void)
{
    csi_pin_set_mux(EXAMPLE_PIN_TDM_SCLK_PORT,EXAMPLE_PIN_TDM_SCLK, EXAMPLE_PIN_TDM_SCLK_FUNC);
    csi_pin_set_mux(EXAMPLE_PIN_TDM_WSCLK_PORT,EXAMPLE_PIN_TDM_WSCLK, EXAMPLE_PIN_TDM_WSCLK_FUNC);
    csi_pin_set_mux(EXAMPLE_PIN_TDM_SDA_PORT,EXAMPLE_PIN_TDM_SDA, EXAMPLE_PIN_TDM_SDA_FUNC);
    csi_pin_set_mux(EXAMPLE_PIN_TDM_SDA1_PORT,EXAMPLE_PIN_TDM_SDA1, EXAMPLE_PIN_TDM_SDA1_FUNC);
    csi_pin_set_mux(EXAMPLE_PIN_TDM_SDA2_PORT,EXAMPLE_PIN_TDM_SDA2, EXAMPLE_PIN_TDM_SDA2_FUNC);
    csi_pin_set_mux(EXAMPLE_PIN_TDM_SDA3_PORT,EXAMPLE_PIN_TDM_SDA3, EXAMPLE_PIN_TDM_SDA3_FUNC);

    csi_pin_mode(EXAMPLE_PIN_TDM_SDA1_PORT,EXAMPLE_PIN_TDM_SDA1, GPIO_MODE_PULLUP);
    csi_pin_mode(EXAMPLE_PIN_TDM_SDA3_PORT,EXAMPLE_PIN_TDM_SDA3, GPIO_MODE_PULLUP);
}
#if 0
static int test_tdm(void)
{
    csi_error_t ret;
    csi_tdm_format_t tdm_config;
    csi_pin_set_mux(EXAMPLE_I2S_MCLK,   EXAMPLE_I2S_MCLK_FUNC);
    csi_pin_set_mux(EXAMPLE_I2S_SCLK,   EXAMPLE_I2S_SCLK_FUNC);
    csi_pin_set_mux(EXAMPLE_I2S_WSCLK,  EXAMPLE_I2S_WSCLK_FUNC);
    example_pin_tdm_init();
#if 1
            // *(volatile uint32_t *)0x50040004=0x3;
            *(volatile uint32_t *)0x50040008=0x100;
            // // *(volatile uint32_t *)0x50040010=0x002;
            // // *(volatile uint32_t *)0x50040014=0x040;
            // // *(volatile uint32_t *)0x50040018=0x1200;
            // *(volatile uint32_t *)0x5004000c=0x10;
            // *(volatile uint32_t *)0x5004001c=0x10;
            *(volatile uint32_t *)0x50040090=0x7;
            *(volatile uint32_t *)0x50040098=0xf;
            *(volatile uint32_t *)0x50040000=0x1;
#endif

#if 0
    uint8_t *data = &tdm_rx_buf0[0];
    for (uint32_t i = 0; i < sizeof(tdm_rx_buf0); i++) {
        data[i] = i % 0xff;
    }

    *(volatile uint32_t *)0x40000514 = 0x1e00e;
    *(volatile uint32_t *)0x60044004 = 0x450031;
    *(volatile uint32_t *)0x60044008 = 0xff0;
    *(volatile uint32_t *)0x40000514 = 0x3e00e;

    // *(volatile uint32_t *)0x60030120 = 0XFCC00000;
    // *(volatile uint32_t *)0x40000404 = 0x1;

    // while(*(volatile uint32_t *)0x40000400 != 1);
    for (uint32_t i = 0; i < 16; i++) {
        *(volatile uint32_t *)0x60044010 = tdm_rx_buf0[i];
        *(volatile uint32_t *)0x60044010 = tdm_rx_buf0[i] >> 16;
        // *(volatile uint32_t *)0x60044010 = 0xffffffff;
    }
    uint32_t count = 0;
    while (count < 32) {
        while (((*(volatile uint32_t *)0x60044014) & 0x2));
        read_data0[count] = *(volatile uint32_t *)0x6004400c;
        count ++;
    }
    printf("tdm read data is:\r\n");
    for (uint32_t i = 0; i < 32; i++) {
        printf("%x ", read_data0[i]);
    }


    while(1);
#endif
    uint8_t *data = (uint8_t *)&tdm_rx_buf0[0];
    for (uint32_t i = 0; i < sizeof(tdm_rx_buf0); i++) {
        data[i] = i & 0xff;
    }
    ret = csi_tdm_init(&tdm, EXAMPLE_TDM_IDX);
   
    if (ret != CSI_OK) {
        printf("csi_tdm_init error\n");
        return -1;
    }
    csi_tdm_attach_callback(&tdm, tdm_master_event_cb_fun, NULL);
    tdm_config.rate = TDM_SAMPLE_RATE_48000;
    tdm_config.mode = TDM_MODE_SLAVE;
    tdm_config.work_mode = TDM_WORKING_MODE_I2S;
    tdm_config.protocol = TDM_PROTOCOL_STANDARD;
    tdm_config.polarity = TDM_LEFT_DATA_FIRST;
    tdm_config.sample_edge = TDM_SAMPLE_RISING_EDGE;
    tdm_config.width = TDM_SAMPLE_WIDTH_16BIT;
    tdm_config.sclk_nfs = TDM_SCLK_256FS;

    csi_tdm_format(&tdm, &tdm_config);

    memset(read_data0, 0, sizeof(read_data0));
    csi_tdm_enable(&tdm, true);
    csi_tdm_receive_async(&tdm, read_data0, sizeof(read_data0));

    while(!cb_master_transfer_flag);
    for (uint32_t i = 0; i < sizeof(read_data0)/4; i++) {
        if ((i % 8) == 0) {
            printf("\n");
        }

        printf("%4x ", read_data0[i]);
    }
    if (memcmp(read_data0, tdm_rx_buf0, sizeof(read_data0))) {
        printf("receive data test failed\r\n");
    } else {
        printf("tdm receive test success\r\n");
    }
    csi_tdm_enable(&tdm, false);
    csi_tdm_uninit(&tdm);

    return 0;
}
#endif

// static void platform_delay(uint32_t ms)
// {
//     mdelay(ms);
// }

void example_pin_iic_init(void)
{
    csi_pin_set_mux(EXAMPLE_PIN_IIC_SDA_PORT,EXAMPLE_PIN_IIC_SDA, EXAMPLE_PIN_IIC_SDA_FUNC);
    csi_pin_set_mux(EXAMPLE_PIN_IIC_SCL_PORT,EXAMPLE_PIN_IIC_SCL, EXAMPLE_PIN_IIC_SCL_FUNC);
}

// static int32_t platform_iic_write(void *handle, uint8_t reg, const uint8_t *bufp,
//                                   uint16_t len)
// {
//     uint8_t buff[32] = {0};
//     buff[0] = reg;
//     memcpy(&buff[1], bufp, len);
//     csi_iic_master_send(handle, LSM6DSO16IS_I2C_ADD_H, buff, len + 1, 100);

//     return 0;
// }

// static int32_t platform_iic_read(void *handle, uint8_t reg, uint8_t *bufp,
//                                  uint16_t len)
// {
//     csi_iic_master_send(handle, LSM6DSO16IS_I2C_ADD_H, &reg, 1, 100);
//     csi_iic_master_receive(handle, LSM6DSO16IS_I2C_ADD_H, bufp, len, 100);

//     return 0;
// }

static int iic_init(void)
{
    csi_error_t ret;

    example_pin_iic_init();

    ret = csi_iic_init(&master_iic, EXAMPLE_IIC_IDX);

    if (ret != CSI_OK)
    {
        printf("csi_iic_initialize error\n");
        return -1;
    }

    ret = csi_iic_mode(&master_iic, IIC_MODE_MASTER);

    if (ret != CSI_OK)
    {
        printf("csi_iic_set_mode error\n");
        return -1;
    }

    ret = csi_iic_addr_mode(&master_iic, IIC_ADDRESS_7BIT);

    if (ret != CSI_OK)
    {
        printf("csi_iic_set_addr_mode error\n");
        return -1;
    }

    ret = csi_iic_speed(&master_iic, IIC_BUS_SPEED_STANDARD);

    if (ret != CSI_OK)
    {
        printf("csi_iic_set_speed error\n");
        return -1;
    }

    return 0;
}

static void es8311_config(void)
{
#if 1 // es8311 codec test
    /* i2s simulation test */
    iic_init();
    uint8_t reg_addr = 0xFD;
    uint8_t reg_value = 0;
    csi_iic_master_send(&master_iic, ES8311_ADDRRES_0, &reg_addr, 1, 100);
    csi_iic_master_receive(&master_iic, ES8311_ADDRRES_0, &reg_value, 1, 100);
    printf("es8311 reg 0x%02X: 0x%02X\n", reg_addr, reg_value);

    reg_addr = 0xFE;
    csi_iic_master_send(&master_iic, ES8311_ADDRRES_0, &reg_addr, 1, 100);
    csi_iic_master_receive(&master_iic, ES8311_ADDRRES_0, &reg_value, 1, 100);
    printf("es8311 reg 0x%02X: 0x%02X\n", reg_addr, reg_value);

#define EXAMPLE_SAMPLE_RATE (48000)
#define EXAMPLE_MCLK_MULTIPLE (32) // If not using 24-bit data width, 256 should be enough
#define EXAMPLE_MCLK_FREQ_HZ (EXAMPLE_SAMPLE_RATE * EXAMPLE_MCLK_MULTIPLE)

    /* Initialize es8311 codec */
    // es8311_handle_t es_handle = es8311_create(&master_iic, ES8311_ADDRRES_0);
    // const es8311_clock_config_t es_clk = {
    //     .mclk_inverted = false,
    //     .sclk_inverted = false,
    //     .mclk_from_mclk_pin = true,
    //     .mclk_frequency = EXAMPLE_MCLK_FREQ_HZ,
    //     .sample_frequency = EXAMPLE_SAMPLE_RATE};

    // es8311_init(es_handle, &es_clk, ES8311_RESOLUTION_16, ES8311_RESOLUTION_16);
    // es8311_sample_frequency_config(es_handle, EXAMPLE_MCLK_FREQ_HZ, EXAMPLE_SAMPLE_RATE);
    // es8311_voice_volume_set(es_handle, 60, NULL); // 设置音量
    // es8311_voice_fade(es_handle, ES8311_FADE_4LRCK);
    // es8311_microphone_config(es_handle, false);

    es8311_handle_t es_handle = es8311_create(&master_iic, ES8311_ADDRRES_0);
    const es8311_clock_config_t es_clk = {
        .mclk_inverted = false,
        .sclk_inverted = false,
        .mclk_from_mclk_pin = false,
        .mclk_frequency = EXAMPLE_MCLK_FREQ_HZ,
        .sample_frequency = EXAMPLE_SAMPLE_RATE};

    es8311_init(es_handle, &es_clk, ES8311_RESOLUTION_16, ES8311_RESOLUTION_16);
    es8311_sample_frequency_config(es_handle, EXAMPLE_MCLK_FREQ_HZ, EXAMPLE_SAMPLE_RATE);
    es8311_voice_mute(es_handle, false);
    es8311_voice_volume_set(es_handle, 60, NULL); // 设置音量
    es8311_voice_fade(es_handle, ES8311_FADE_4LRCK);
    es8311_microphone_config(es_handle, false);
    es8311_microphone_gain_set(es_handle, ES8311_MIC_GAIN_24DB);
    mdelay(500);

    for (size_t i = 0; i < 0x32; i++)
    {
        reg_addr = i;
        csi_iic_master_send(&master_iic, ES8311_ADDRRES_0, &reg_addr, 1, 100);
        csi_iic_master_receive(&master_iic, ES8311_ADDRRES_0, &reg_value, 1, 100);
        printf("es8311 reg 0x%02X: 0x%02X\n", reg_addr, reg_value);
    }

#endif
}
uint16_t *wavDataAddr = NULL;
uint32_t wavDatalen = 0;

#if 0
__ASM(".section .wav, \"a\" \n"
      ".global wav_start, wav_end, lc3_start, lc3_end, bmp_start, bmp_end;"
      "wav_start: "
      //   ".incbin \"src/TRK11.wav\";"
      //   ".incbin \"src/sine_440hz_2s_fade.wav\";"
      // ".incbin \"src/Alarm02_mono.wav\";"
            // ".incbin \"sine_440_1s_44100.wav\";"
            ".incbin \"sine_440hz_0.6s.wav\";"
            
      "wav_end: "
      "lc3_start: "
      // ".incbin \"src/test_44100.lc3\";"
      //   ".incbin \"src/To_Alice.lc3\";"
      //".incbin \"src/sine_440_1s_44100.lc3\";"
      //   ".incbin \"src/Alarm02_mono.lc3\";"
      "lc3_end: "
      "bmp_start: "
      //   ".incbin \"src/NStar.bin\";"
      "bmp_end: ");
extern uint32_t wav_start, wav_end;
#endif
static int test_tdm1(void)
{
    csi_error_t ret;
    csi_tdm_format_t tdm_config;
    // csi_pin_set_mux(EXAMPLE_I2S_MCLK,   EXAMPLE_I2S_MCLK_FUNC);
    // csi_pin_set_mux(EXAMPLE_I2S_SCLK,   EXAMPLE_I2S_SCLK_FUNC);
    // csi_pin_set_mux(EXAMPLE_I2S_WSCLK,  EXAMPLE_I2S_WSCLK_FUNC);
    example_pin_tdm_init();
#if 0
            // *(volatile uint32_t *)0x50040004=0x3;
            *(volatile uint32_t *)0x50040008=0x100;
            // // *(volatile uint32_t *)0x50040010=0x002;
            // // *(volatile uint32_t *)0x50040014=0x040;
            // // *(volatile uint32_t *)0x50040018=0x1200;
            // *(volatile uint32_t *)0x5004000c=0x10;
            // *(volatile uint32_t *)0x5004001c=0x10;
            *(volatile uint32_t *)0x50040090=0x7;
            *(volatile uint32_t *)0x50040098=0xf;
            *(volatile uint32_t *)0x50040000=0x1;
#endif

#if 0
    uint8_t *data = &tdm_rx_buf0[0];
    for (uint32_t i = 0; i < sizeof(tdm_rx_buf0); i++) {
        data[i] = i % 0xff;
    }

    *(volatile uint32_t *)0x40000514 = 0x1e00e;
    *(volatile uint32_t *)0x60044004 = 0x450031;
    *(volatile uint32_t *)0x60044008 = 0xff0;
    *(volatile uint32_t *)0x40000514 = 0x3e00e;

    // *(volatile uint32_t *)0x60030120 = 0XFCC00000;
    // *(volatile uint32_t *)0x40000404 = 0x1;

    // while(*(volatile uint32_t *)0x40000400 != 1);
    for (uint32_t i = 0; i < 16; i++) {
        *(volatile uint32_t *)0x60044010 = tdm_rx_buf0[i];
        *(volatile uint32_t *)0x60044010 = tdm_rx_buf0[i] >> 16;
        // *(volatile uint32_t *)0x60044010 = 0xffffffff;
    }
    uint32_t count = 0;
    while (count < 32) {
        while (((*(volatile uint32_t *)0x60044014) & 0x2));
        read_data0[count] = *(volatile uint32_t *)0x6004400c;
        count ++;
    }
    printf("tdm read data is:\r\n");
    for (uint32_t i = 0; i < 32; i++) {
        printf("%x ", read_data0[i]);
    }


    while(1);
#endif
    // es8311_config();
    uint8_t *data = (uint8_t *)&tdm_rx_buf0[0];
    for (uint32_t i = 0; i < sizeof(tdm_rx_buf0); i++) {
        data[i] = i & 0xff;
    }
    ret = csi_tdm_init(&s_tdm, EXAMPLE_TDM_IDX);
   
    if (ret != CSI_OK) {
        printf("csi_tdm_init error\n");
        return -1;
    }
    csi_tdm_attach_callback(&s_tdm, tdm_master_event_cb_fun, NULL);
    tdm_config.rate = TDM_SAMPLE_RATE_48000;
    tdm_config.mode = TDM_MODE_MASTER;
    tdm_config.work_mode = TDM_WORKING_MODE_I2S;
    tdm_config.protocol = TDM_PROTOCOL_STANDARD;
    tdm_config.polarity = TDM_LEFT_DATA_FIRST;
    tdm_config.sample_edge = TDM_SAMPLE_RISING_EDGE;
    tdm_config.width = TDM_SAMPLE_WIDTH_16BIT;
    tdm_config.sclk_nfs = TDM_SCLK_256FS;

    csi_tdm_format(&s_tdm, &tdm_config);

    memset(read_data0, 0, sizeof(read_data0));
    csi_tdm_enable(&s_tdm, true);

    ping = 1;
    pong = 0;
    data_ptr = read_data1;
#if 1
    // csi_pin_set_mux(EXAMPLE_I2S_SCLK,   EXAMPLE_I2S_SCLK_FUNC);
    // csi_pin_set_mux(EXAMPLE_I2S_WSCLK,  EXAMPLE_I2S_WSCLK_FUNC);
    csi_tdm_receive_async(&s_tdm, data_ptr, sizeof(read_data0));
#else
    uint8_t *wav_start_addr = (uint8_t *)&wav_start;

    RiffChunk *riffChunk = (RiffChunk *)wav_start_addr;
    printf("Chunk ID: %.4s\r\n", riffChunk->chunkId);
    printf("Chunk Size: %u\r\n", riffChunk->chunkSize);
    printf("Format: %.4s\r\n", riffChunk->format);

    WavFmtChunk *wavFmtChunk = (WavFmtChunk *)(wav_start_addr + sizeof(RiffChunk));
    printf("SubChunk1 ID: %.4s\r\n", wavFmtChunk->subChunk1Id);
    printf("SubChunk1 Size: %u\r\n", wavFmtChunk->subChunk1Size);
    printf("Audio Format: %d\r\n", wavFmtChunk->audioFormat);
    printf("Num Channels: %d\r\n", wavFmtChunk->numChannels);
    printf("Sample Rate: %d\r\n", wavFmtChunk->sampleRate);
    printf("Byte Rate: %d\r\n", wavFmtChunk->byteRate);
    printf("block Align: %d\r\n", wavFmtChunk->blockAlign);
    printf("bits Per Sample: %d\r\n", wavFmtChunk->bitsPerSample);

    WavDataChunk *wavDataChunk = (WavDataChunk *)(wav_start_addr + sizeof(RiffChunk) + sizeof(WavFmtChunk));
    printf("Data1 ID: %.4s\r\n", wavDataChunk->Data1Id);
    printf("Data Size: %u\r\n", wavDataChunk->DataSize);

    wavDataAddr = (uint16_t *)(wav_start_addr + sizeof(RiffChunk) + sizeof(WavFmtChunk) + sizeof(WavDataChunk));
    // wavDatalen = wavDataChunk->DataSize / wavFmtChunk->blockAlign;
    wavDatalen = wavDataChunk->DataSize;
    printf("Data Len: %d\r\n", wavDatalen);

    while(1) {
    csi_tdm_send(&s_tdm, wavDataAddr, wavDatalen);
    }
    while(1);
#endif
    while (1) {
        while(!cb_master_transfer_flag);
        cb_master_transfer_flag = 0;

        if (end_ping == 1) {
            printf("1%hx %hx %hx %hx %hx %hx %hx %hx %hx %hx %hx %hx %hx %hx %hx %hx\n", read_data0[0], read_data0[1],  read_data0[2], read_data0[3], read_data0[4], read_data0[5],  read_data0[6], read_data0[7]
            ,read_data0[8], read_data0[9],  read_data0[10], read_data0[11], read_data0[12], read_data0[13],  read_data0[14], read_data0[15]);
            memcpy(dest_buf, read_data0, sizeof(read_data0));
            uint32_t write_size = csi_tdm_send(&s_tdm, dest_buf, sizeof(dest_buf));
        } else if (end_pong == 1) {
            printf("2%hx %hx %hx %hx %hx %hx %hx %hx %hx %hx %hx %hx %hx %hx %hx %hx\n", read_data1[0], read_data1[1],  read_data1[2], read_data1[3], read_data1[4], read_data1[5],  read_data1[6], read_data1[7]
            ,read_data1[8], read_data1[9],  read_data1[10], read_data1[11], read_data1[12], read_data1[13],  read_data1[14], read_data1[15]);
            memcpy(dest_buf, read_data1, sizeof(read_data1));
            uint32_t write_size = csi_tdm_send(&s_tdm, dest_buf, sizeof(dest_buf));
        }
    }

    for (uint32_t i = 0; i < sizeof(read_data0)/4; i++) {
        if ((i % 8) == 0) {
            printf("\n");
        }

        printf("%4x ", read_data0[i]);
    }
    if (memcmp(read_data0, tdm_rx_buf0, sizeof(read_data0))) {
        printf("receive data test failed\r\n");
    } else {
        printf("tdm receive test success\r\n");
    }
    // csi_tdm_enable(&tdm, false);
    // csi_tdm_uninit(&tdm);

    return 0;
}

int example_tdm(void)
{
    int ret;

    // ret = test_tdm();

    ret = test_tdm1();

    if (ret < 0) {
        printf("test_tdm fail\n");
        return -1;
    }

    return ret;
}

int main(void)
{
    board_init();

    printf("Example for tdm receive async\n");

    example_tdm();

    return 0;
}

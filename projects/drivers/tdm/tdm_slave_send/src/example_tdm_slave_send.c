

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
#include <drv/pin.h>
#include <soc.h>


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

csi_tdm_t tdm;

#define TDM_RX_BUF_SIZE 2048
static uint32_t tdm_tx_buf0[TDM_RX_BUF_SIZE];

#define READ_BUF_SIZE 2048
uint32_t read_data0[READ_BUF_SIZE];

volatile uint8_t cb_transfer_flag0 = 0U;

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
uint16_t *wavDataAddr = NULL;
uint32_t wavDatalen = 0;
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
extern const void wav_start, wav_end;
#endif

static int test_tdm(void)
{
    csi_error_t ret;
    csi_tdm_format_t tdm_config;
    example_pin_tdm_init();

    uint8_t *data = (uint8_t *)&tdm_tx_buf0[0];
    for (uint32_t i = 0; i < sizeof(tdm_tx_buf0); i++) {
        data[i] = i & 0xff;
    }
    ret = csi_tdm_init(&tdm, EXAMPLE_TDM_IDX);
   
    if (ret != CSI_OK) {
        printf("csi_tdm_init error\n");
        return -1;
    }

    tdm_config.rate = TDM_SAMPLE_RATE_48000;
    tdm_config.mode = TDM_MODE_SLAVE;
    tdm_config.work_mode = TDM_WORKING_MODE_I2S;
    tdm_config.protocol = TDM_PROTOCOL_STANDARD;
    tdm_config.polarity = TDM_LEFT_DATA_FIRST;
    tdm_config.sample_edge = TDM_SAMPLE_RISING_EDGE;
    tdm_config.width = TDM_SAMPLE_WIDTH_16BIT;
    tdm_config.sclk_nfs = TDM_SCLK_256FS;

    csi_tdm_format(&tdm, &tdm_config);

    csi_tdm_enable(&tdm, true);
#if 0
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
        csi_tdm_send(&tdm, wavDataAddr, wavDatalen);
    }
#else
    while(1) {
        uint32_t write_size = csi_tdm_send(&tdm, tdm_tx_buf0, sizeof(tdm_tx_buf0));
        if (write_size == sizeof(tdm_tx_buf0)) {
            printf("test_iis successfully\n");
            CSI_EXAMPLE_RESULT(1);
        } else {
            printf("test_iis failed\n");
            CSI_EXAMPLE_RESULT(-1);
        }
    }
#endif
    memset(read_data0, 0, sizeof(READ_BUF_SIZE));

    csi_tdm_enable(&tdm, false);
    csi_tdm_uninit(&tdm);

    return 0;
}

int example_tdm(void)
{
    int ret;

    ret = test_tdm();

    if (ret < 0) {
        printf("test_tdm fail\n");
        return -1;
    }

    return ret;
}

int main(void)
{
    board_init();

    printf("Example for tdm send sync\n");

    example_tdm();

    return 0;
}

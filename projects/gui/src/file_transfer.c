/******************************************************************************
 * @file     file_transfer.c
 * @brief    JPG 文件传输协议状态机
 *
 * 协议说明：
 *   - 多字节字段全部使用小端格式。
 *   - CRC16 为 CRC-16/CCITT-FALSE，覆盖除末尾 CRC16 字段外的整包内容。
 *   - file_crc32 为标准 CRC32，覆盖 JPG 文件原始字节。
 ******************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "file_transfer.h"
#include "fs_image.h"
#include "image_gallery.h"

#define FT_START_FIXED_LEN      19U
#define FT_DATA_FIXED_LEN       18U
#define FT_END_LEN              18U
#define FT_FORMAT_LEN           10U
#define FT_ACK_LEN              13U
#define FT_MAX_PACKET_SIZE      (FT_DATA_FIXED_LEN + FT_MAX_CHUNK_SIZE + 2U)
#define FT_POLL_READ_SIZE       64U

typedef struct {
    const transport_t *transport;
    file_transfer_state_t state;
    uint8_t pkt[FT_MAX_PACKET_SIZE];
    uint32_t pkt_len;
    uint32_t expected_len;
    uint16_t file_id;
    uint32_t file_size;
    uint32_t file_crc32;
    uint16_t chunk_size;
    char filename[FT_MAX_FILENAME_LEN + 1U];
    uint32_t expected_seq;
    uint32_t expected_offset;
    uint32_t received_size;
} file_transfer_ctx_t;

static file_transfer_ctx_t s_ft;

static uint16_t ft_rd16(const uint8_t *p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t ft_rd32(const uint8_t *p)
{
    return (uint32_t)p[0] |
           ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static void ft_wr16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)(v & 0xFFU);
    p[1] = (uint8_t)((v >> 8) & 0xFFU);
}

static void ft_wr32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v & 0xFFU);
    p[1] = (uint8_t)((v >> 8) & 0xFFU);
    p[2] = (uint8_t)((v >> 16) & 0xFFU);
    p[3] = (uint8_t)((v >> 24) & 0xFFU);
}

static uint16_t ft_crc16_update(uint16_t crc, const uint8_t *data, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t bit = 0; bit < 8U; bit++) {
            if (crc & 0x8000U) {
                crc = (uint16_t)((crc << 1) ^ 0x1021U);
            } else {
                crc <<= 1;
            }
        }
    }

    return crc;
}

static uint16_t ft_crc16(const uint8_t *data, uint32_t len)
{
    return ft_crc16_update(0xFFFFU, data, len);
}

static uint8_t ft_packet_crc_ok(const uint8_t *pkt, uint32_t len)
{
    if (len < 2U) {
        return 0U;
    }

    return ft_crc16(pkt, len - 2U) == ft_rd16(&pkt[len - 2U]);
}

static void ft_reset_parser(void)
{
    s_ft.pkt_len = 0U;
    s_ft.expected_len = 0U;
}

static int ft_send_ack_common(uint8_t cmd, uint16_t file_id,
                              uint32_t seq, uint8_t status)
{
    uint8_t ack[FT_ACK_LEN];

    ft_wr32(&ack[0], FT_MAGIC_VALUE);
    ack[4] = FT_VERSION;
    ack[5] = cmd;
    ft_wr16(&ack[6], file_id);
    ft_wr32(&ack[8], seq);
    ack[12] = status;

    if ((s_ft.transport == NULL) || (s_ft.transport->send == NULL)) {
        return -1;
    }

    return s_ft.transport->send(ack, sizeof(ack));
}

static void ft_send_ack(uint16_t file_id, uint32_t seq)
{
    (void)ft_send_ack_common(FT_CMD_ACK, file_id, seq, 0U);
}

static void ft_send_nack(uint16_t file_id, uint32_t seq, uint8_t err)
{
    (void)ft_send_ack_common(FT_CMD_NACK, file_id, seq, err);
}

static void ft_handle_start(const uint8_t *pkt, uint32_t len)
{
    uint8_t filename_len;
    int begin_ret;

    if (!ft_packet_crc_ok(pkt, len)) {
        ft_send_nack(0U, 0U, FT_ERR_CRC);
        printf("[ft] START crc error\r\n");
        return;
    }

    if ((len < FT_START_FIXED_LEN + 2U) || (pkt[4] != FT_VERSION) ||
        (pkt[5] != FT_CMD_START)) {
        ft_send_nack(0U, 0U, FT_ERR_PARAM);
        return;
    }

    filename_len = pkt[18];
    if ((filename_len > FT_MAX_FILENAME_LEN) ||
        (len != (uint32_t)FT_START_FIXED_LEN + filename_len + 2U)) {
        ft_send_nack(ft_rd16(&pkt[6]), 0U, FT_ERR_PARAM);
        printf("[ft] bad filename len %u\r\n", filename_len);
        return;
    }

    s_ft.file_id = ft_rd16(&pkt[6]);
    s_ft.file_size = ft_rd32(&pkt[8]);
    s_ft.file_crc32 = ft_rd32(&pkt[12]);
    s_ft.chunk_size = ft_rd16(&pkt[16]);

    memcpy(s_ft.filename, &pkt[19], filename_len);
    s_ft.filename[filename_len] = '\0';

    if ((s_ft.file_size == 0U) ||
        (s_ft.chunk_size == 0U) ||
        (s_ft.chunk_size > FT_MAX_CHUNK_SIZE)) {
        ft_send_nack(s_ft.file_id, 0U, FT_ERR_PARAM);
        printf("[ft] bad START size=%u chunk=%u\r\n",
               (unsigned)s_ft.file_size, (unsigned)s_ft.chunk_size);
        return;
    }

    fs_image_abort();
    begin_ret = fs_image_begin(s_ft.filename, s_ft.file_size);
    if (begin_ret == 1) {
        ft_send_nack(s_ft.file_id, 0U, FT_ERR_STORAGE_FULL);
        s_ft.state = FT_STATE_WAIT_START;
        return;
    }
    if (begin_ret != 0) {
        ft_send_nack(s_ft.file_id, 0U, FT_ERR_FS_WRITE);
        s_ft.state = FT_STATE_ERROR;
        return;
    }

    s_ft.expected_seq = 0U;
    s_ft.expected_offset = 0U;
    s_ft.received_size = 0U;
    s_ft.state = FT_STATE_RECV_DATA;

    printf("[ft] START file_id=%u size=%u crc=0x%08x chunk=%u name='%s'\r\n",
           (unsigned)s_ft.file_id, (unsigned)s_ft.file_size,
           (unsigned)s_ft.file_crc32, (unsigned)s_ft.chunk_size, s_ft.filename);
    ft_send_ack(s_ft.file_id, 0U);
}

static void ft_handle_data(const uint8_t *pkt, uint32_t len)
{
    uint16_t file_id;
    uint32_t seq;
    uint32_t offset;
    uint16_t payload_len;

    if (len < FT_DATA_FIXED_LEN + 2U) {
        return;
    }

    file_id = ft_rd16(&pkt[6]);
    seq = ft_rd32(&pkt[8]);
    offset = ft_rd32(&pkt[12]);
    payload_len = ft_rd16(&pkt[16]);

    if (!ft_packet_crc_ok(pkt, len)) {
        ft_send_nack(file_id, seq, FT_ERR_CRC);
        printf("[ft] DATA crc error seq=%u\r\n", (unsigned)seq);
        return;
    }

    if ((s_ft.state != FT_STATE_RECV_DATA) || (file_id != s_ft.file_id) ||
        (pkt[4] != FT_VERSION) || (pkt[5] != FT_CMD_DATA) ||
        (payload_len == 0U) || (payload_len > s_ft.chunk_size) ||
        (payload_len > FT_MAX_CHUNK_SIZE) ||
        (len != (uint32_t)FT_DATA_FIXED_LEN + payload_len + 2U)) {
        ft_send_nack(file_id, seq, FT_ERR_PARAM);
        return;
    }

    if (seq != s_ft.expected_seq) {
        ft_send_nack(file_id, seq, FT_ERR_SEQ);
        printf("[ft] seq error got=%u expected=%u\r\n",
               (unsigned)seq, (unsigned)s_ft.expected_seq);
        return;
    }

    if (offset != s_ft.expected_offset) {
        ft_send_nack(file_id, seq, FT_ERR_OFFSET);
        printf("[ft] offset error got=%u expected=%u\r\n",
               (unsigned)offset, (unsigned)s_ft.expected_offset);
        return;
    }

    if ((offset + payload_len) > s_ft.file_size) {
        ft_send_nack(file_id, seq, FT_ERR_PARAM);
        return;
    }

    if (fs_image_write(offset, &pkt[18], payload_len) != 0) {
        ft_send_nack(file_id, seq, FT_ERR_FS_WRITE);
        fs_image_abort();
        s_ft.state = FT_STATE_WAIT_START;
        return;
    }

    s_ft.received_size += payload_len;
    s_ft.expected_offset += payload_len;
    s_ft.expected_seq++;
    ft_send_ack(file_id, seq);
}

static void ft_handle_end(const uint8_t *pkt, uint32_t len)
{
    uint16_t file_id = ft_rd16(&pkt[6]);
    uint32_t total_chunks = ft_rd32(&pkt[8]);
    uint32_t file_crc32 = ft_rd32(&pkt[12]);

    if (!ft_packet_crc_ok(pkt, len)) {
        ft_send_nack(file_id, total_chunks, FT_ERR_CRC);
        printf("[ft] END crc error\r\n");
        return;
    }

    if ((s_ft.state != FT_STATE_RECV_DATA) || (pkt[4] != FT_VERSION) ||
        (pkt[5] != FT_CMD_END) || (file_id != s_ft.file_id)) {
        ft_send_nack(file_id, total_chunks, FT_ERR_PARAM);
        return;
    }

    if ((total_chunks != s_ft.expected_seq) ||
        (file_crc32 != s_ft.file_crc32) ||
        (s_ft.received_size != s_ft.file_size)) {
        ft_send_nack(file_id, total_chunks, FT_ERR_FILE_CRC);
        printf("[ft] END mismatch chunks=%u/%u size=%u/%u\r\n",
               (unsigned)total_chunks, (unsigned)s_ft.expected_seq,
               (unsigned)s_ft.received_size, (unsigned)s_ft.file_size);
        fs_image_abort();
        s_ft.state = FT_STATE_WAIT_START;
        return;
    }

    s_ft.state = FT_STATE_VERIFY_FILE;
    if (fs_image_finish(s_ft.file_crc32) != 0) {
        ft_send_nack(file_id, total_chunks, FT_ERR_FILE_CRC);
        s_ft.state = FT_STATE_WAIT_START;
        return;
    }

    /*
     * 先 ACK 再显示，避免 PC 端在 JPEG 解码刷屏期间等待超时。
     */
    ft_send_ack(file_id, total_chunks);
    s_ft.state = FT_STATE_DECODE_DISPLAY;
    (void)image_gallery_show_path(fs_image_get_final_path());
    s_ft.state = FT_STATE_WAIT_START;
}

static void ft_handle_format(const uint8_t *pkt, uint32_t len)
{
    uint16_t file_id;

    if ((len != FT_FORMAT_LEN) || (pkt[4] != FT_VERSION) ||
        (pkt[5] != FT_CMD_FORMAT)) {
        return;
    }

    file_id = ft_rd16(&pkt[6]);
    if (!ft_packet_crc_ok(pkt, len)) {
        ft_send_nack(file_id, 0U, FT_ERR_CRC);
        return;
    }

    fs_image_abort();
    if (fs_image_format() != 0) {
        ft_send_nack(file_id, 0U, FT_ERR_FS_WRITE);
        return;
    }

    s_ft.state = FT_STATE_WAIT_START;
    ft_send_ack(file_id, 0U);
}

static void ft_handle_packet(const uint8_t *pkt, uint32_t len)
{
    uint8_t cmd;

    if ((len < 6U) || (ft_rd32(pkt) != FT_MAGIC_VALUE)) {
        return;
    }

    cmd = pkt[5];
    switch (cmd) {
        case FT_CMD_START:
            ft_handle_start(pkt, len);
            break;
        case FT_CMD_DATA:
            ft_handle_data(pkt, len);
            break;
        case FT_CMD_END:
            ft_handle_end(pkt, len);
            break;
        case FT_CMD_FORMAT:
            ft_handle_format(pkt, len);
            break;
        default:
            printf("[ft] unknown cmd 0x%02x\r\n", cmd);
            break;
    }
}

static void ft_drop_first_byte(void)
{
    if (s_ft.pkt_len > 1U) {
        memmove(&s_ft.pkt[0], &s_ft.pkt[1], s_ft.pkt_len - 1U);
        s_ft.pkt_len--;
    } else {
        s_ft.pkt_len = 0U;
    }
    s_ft.expected_len = 0U;
}

static void ft_feed_byte(uint8_t b)
{
    if (s_ft.pkt_len >= sizeof(s_ft.pkt)) {
        ft_reset_parser();
    }

    s_ft.pkt[s_ft.pkt_len++] = b;

    while (s_ft.pkt_len > 0U) {
        uint8_t ok_prefix = 1U;
        const uint8_t magic[4] = { 'J', 'P', 'G', 'U' };

        for (uint32_t i = 0; i < s_ft.pkt_len && i < 4U; i++) {
            if (s_ft.pkt[i] != magic[i]) {
                ok_prefix = 0U;
                break;
            }
        }

        if (ok_prefix) {
            break;
        }
        ft_drop_first_byte();
    }

    if (s_ft.pkt_len < 6U) {
        return;
    }

    if (s_ft.expected_len == 0U) {
        uint8_t cmd = s_ft.pkt[5];

        if (cmd == FT_CMD_START && s_ft.pkt_len >= FT_START_FIXED_LEN) {
            uint8_t filename_len = s_ft.pkt[18];
            if (filename_len > FT_MAX_FILENAME_LEN) {
                ft_reset_parser();
                return;
            }
            s_ft.expected_len = FT_START_FIXED_LEN + filename_len + 2U;
        } else if (cmd == FT_CMD_DATA && s_ft.pkt_len >= FT_DATA_FIXED_LEN) {
            uint16_t payload_len = ft_rd16(&s_ft.pkt[16]);
            if ((payload_len == 0U) || (payload_len > FT_MAX_CHUNK_SIZE)) {
                ft_reset_parser();
                return;
            }
            s_ft.expected_len = FT_DATA_FIXED_LEN + payload_len + 2U;
        } else if (cmd == FT_CMD_END) {
            s_ft.expected_len = FT_END_LEN;
        } else if (cmd == FT_CMD_FORMAT) {
            s_ft.expected_len = FT_FORMAT_LEN;
        } else if (cmd != FT_CMD_START && cmd != FT_CMD_DATA) {
            ft_reset_parser();
            return;
        }
    }

    if ((s_ft.expected_len > 0U) && (s_ft.pkt_len >= s_ft.expected_len)) {
        ft_handle_packet(s_ft.pkt, s_ft.expected_len);
        ft_reset_parser();
    }
}

int file_transfer_init(const transport_t *transport)
{
    memset(&s_ft, 0, sizeof(s_ft));

    if ((transport == NULL) || (transport->init == NULL) ||
        (transport->send == NULL) || (transport->recv == NULL)) {
        return -1;
    }

    s_ft.transport = transport;
    s_ft.state = FT_STATE_WAIT_START;

    if (transport->init() != 0) {
        printf("[ft] transport init failed\r\n");
        return -1;
    }

    if (transport->set_rx_callback != NULL) {
        (void)transport->set_rx_callback(NULL);
    }

    return 0;
}

void file_transfer_poll(void)
{
    uint8_t buf[FT_POLL_READ_SIZE];

    if ((s_ft.transport == NULL) || (s_ft.transport->recv == NULL)) {
        return;
    }

    while (1) {
        int n = s_ft.transport->recv(buf, sizeof(buf));
        if (n <= 0) {
            break;
        }

        for (int i = 0; i < n; i++) {
            ft_feed_byte(buf[i]);
        }
    }
}

file_transfer_state_t file_transfer_get_state(void)
{
    return s_ft.state;
}

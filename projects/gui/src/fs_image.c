/******************************************************************************
 * @file     fs_image.c
 * @brief    图片文件 LittleFS 保存策略实现
 ******************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "fs_image.h"
#include "lfs_port.h"

#define FS_IMAGE_LEGACY_DIR     "/img"
#define FS_IMAGE_LEGACY_PATH    "/img/1.jpg"

static lfs_file_t s_tmp_file;
static uint8_t s_tmp_open;
static uint32_t s_expected_size;
static uint32_t s_written_size;
static char s_final_path[FS_IMAGE_MAX_PATH_LEN];

static int fs_image_is_valid_name_char(char ch)
{
    return ((ch >= '0' && ch <= '9') ||
            (ch >= 'A' && ch <= 'Z') ||
            (ch >= 'a' && ch <= 'z') ||
            ch == '.' || ch == '_' || ch == '-');
}

static int fs_image_make_path(const char *filename)
{
    uint32_t len;

    if (filename == NULL) {
        return -1;
    }

    len = (uint32_t)strlen(filename);
    if ((len == 0U) || (len > FS_IMAGE_MAX_NAME_LEN)) {
        return -1;
    }

    if ((strcmp(filename, ".") == 0) || (strcmp(filename, "..") == 0)) {
        return -1;
    }

    for (uint32_t i = 0; i < len; i++) {
        if (!fs_image_is_valid_name_char(filename[i])) {
            return -1;
        }
    }

    s_final_path[0] = '/';
    memcpy(&s_final_path[1], filename, len);
    s_final_path[len + 1U] = '\0';
    return 0;
}

static uint32_t fs_crc32_update(uint32_t crc, const uint8_t *data, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8U; bit++) {
            if (crc & 1U) {
                crc = (crc >> 1) ^ 0xEDB88320UL;
            } else {
                crc >>= 1;
            }
        }
    }

    return crc;
}

int fs_image_mount(void)
{
    if (lfs_port_get() != NULL) {
        return 0;
    }

    return lfs_port_mount();
}

int fs_image_format(void)
{
    lfs_port_unmount();

    if (lfs_port_format() != 0) {
        return -1;
    }

    return lfs_port_mount();
}

static void fs_image_cleanup_legacy(lfs_t *lfs)
{
    /*
     * 旧版内置图片调试流程会占用 /img/1.jpg。当前 DATA 模式按文件名保存
     * 到根目录，例如 /1.jpg、/2.jpg。这里清理旧调试文件释放空间。
     */
    lfs_remove(lfs, FS_IMAGE_LEGACY_PATH);
    lfs_remove(lfs, FS_IMAGE_LEGACY_DIR);
}

int fs_image_prepare_receive(const char *filename, uint32_t file_size,
                             uint32_t *free_bytes)
{
    lfs_t *lfs;
    lfs_ssize_t used_blocks;
    uint32_t free;
    uint32_t required;

    if ((file_size == 0U) || (file_size > FS_IMAGE_MAX_FILE_SIZE)) {
        printf("[fs_image] invalid file size %u\r\n", (unsigned)file_size);
        return -1;
    }

    if (fs_image_make_path(filename) != 0) {
        printf("[fs_image] invalid filename\r\n");
        return -1;
    }

    if (fs_image_mount() != 0) {
        printf("[fs_image] mount failed\r\n");
        return -1;
    }

    lfs = lfs_port_get();
    if (lfs == NULL) {
        return -1;
    }

    fs_image_cleanup_legacy(lfs);
    if (s_tmp_open) {
        lfs_file_close(lfs, &s_tmp_file);
        s_tmp_open = 0U;
    }
    lfs_remove(lfs, FS_IMAGE_TMP_PATH);

    used_blocks = lfs_fs_size(lfs);
    if (used_blocks < 0) {
        printf("[fs_image] lfs_fs_size failed (%d)\r\n", (int)used_blocks);
        return -1;
    }

    if ((uint32_t)used_blocks >= LFS_BLOCK_COUNT) {
        free = 0U;
    } else {
        free = (LFS_BLOCK_COUNT - (uint32_t)used_blocks) * LFS_SECTOR_SIZE;
    }

    if (free_bytes != NULL) {
        *free_bytes = free;
    }

    required = file_size + FS_IMAGE_SPACE_RESERVE_BYTES;
    if (free < required) {
        printf("[fs_image] storage full free=%u required=%u file=%u\r\n",
               (unsigned)free, (unsigned)required, (unsigned)file_size);
        return 1;
    }

    return 0;
}

int fs_image_begin(const char *filename, uint32_t file_size)
{
    lfs_t *lfs;
    uint32_t free_bytes = 0U;
    int prep = fs_image_prepare_receive(filename, file_size, &free_bytes);

    if (prep != 0) {
        return prep;
    }

    lfs = lfs_port_get();
    if (lfs == NULL) {
        return -1;
    }

    if (s_tmp_open) {
        lfs_file_close(lfs, &s_tmp_file);
        s_tmp_open = 0U;
    }

    int err = lfs_file_open(lfs, &s_tmp_file, FS_IMAGE_TMP_PATH,
                            LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC);
    if (err != LFS_ERR_OK) {
        printf("[fs_image] open tmp failed (%d)\r\n", err);
        return -1;
    }

    s_tmp_open = 1U;
    s_expected_size = file_size;
    s_written_size = 0U;
    printf("[fs_image] receive '%s' size=%u free=%u\r\n",
           s_final_path, (unsigned)file_size, (unsigned)free_bytes);
    return 0;
}

int fs_image_write(uint32_t offset, const uint8_t *data, uint32_t len)
{
    lfs_t *lfs = lfs_port_get();

    if ((lfs == NULL) || (!s_tmp_open) || (data == NULL)) {
        return -1;
    }

    if ((offset != s_written_size) || (len == 0U) ||
        ((offset + len) > s_expected_size)) {
        printf("[fs_image] bad write offset=%u len=%u written=%u size=%u\r\n",
               (unsigned)offset, (unsigned)len,
               (unsigned)s_written_size, (unsigned)s_expected_size);
        return -1;
    }

    lfs_ssize_t written = lfs_file_write(lfs, &s_tmp_file, data, len);
    if (written != (lfs_ssize_t)len) {
        printf("[fs_image] write failed (%d)\r\n", (int)written);
        return -1;
    }

    s_written_size += len;
    return 0;
}

int fs_image_crc32_file(const char *path, uint32_t *out_crc32)
{
    lfs_t *lfs = lfs_port_get();
    lfs_file_t file;
    uint8_t buf[256];
    uint32_t crc = 0xFFFFFFFFUL;

    if ((lfs == NULL) || (path == NULL) || (out_crc32 == NULL)) {
        return -1;
    }

    int err = lfs_file_open(lfs, &file, path, LFS_O_RDONLY);
    if (err != LFS_ERR_OK) {
        printf("[fs_image] open '%s' for crc failed (%d)\r\n", path, err);
        return -1;
    }

    while (1) {
        lfs_ssize_t n = lfs_file_read(lfs, &file, buf, sizeof(buf));
        if (n < 0) {
            printf("[fs_image] crc read failed (%d)\r\n", (int)n);
            lfs_file_close(lfs, &file);
            return -1;
        }
        if (n == 0) {
            break;
        }
        crc = fs_crc32_update(crc, buf, (uint32_t)n);
    }

    lfs_file_close(lfs, &file);
    *out_crc32 = crc ^ 0xFFFFFFFFUL;
    return 0;
}

int fs_image_finish(uint32_t expected_crc32)
{
    lfs_t *lfs = lfs_port_get();
    uint32_t actual_crc32 = 0U;

    if ((lfs == NULL) || (!s_tmp_open)) {
        return -1;
    }

    if (s_written_size != s_expected_size) {
        printf("[fs_image] size mismatch written=%u expected=%u\r\n",
               (unsigned)s_written_size, (unsigned)s_expected_size);
        fs_image_abort();
        return -1;
    }

    int err = lfs_file_sync(lfs, &s_tmp_file);
    if (err != LFS_ERR_OK) {
        printf("[fs_image] sync failed (%d)\r\n", err);
        fs_image_abort();
        return -1;
    }

    lfs_file_close(lfs, &s_tmp_file);
    s_tmp_open = 0U;

    if (fs_image_crc32_file(FS_IMAGE_TMP_PATH, &actual_crc32) != 0) {
        fs_image_abort();
        return -1;
    }

    if (actual_crc32 != expected_crc32) {
        printf("[fs_image] crc mismatch actual=0x%08x expected=0x%08x\r\n",
               (unsigned)actual_crc32, (unsigned)expected_crc32);
        fs_image_abort();
        return -1;
    }

    /* 校验成功后才替换正式文件，失败不会破坏旧图片。 */
    lfs_remove(lfs, s_final_path);
    err = lfs_rename(lfs, FS_IMAGE_TMP_PATH, s_final_path);
    if (err != LFS_ERR_OK) {
        printf("[fs_image] rename failed (%d)\r\n", err);
        lfs_remove(lfs, FS_IMAGE_TMP_PATH);
        return -1;
    }

    printf("[fs_image] updated '%s' (%u bytes)\r\n",
           s_final_path, (unsigned)s_written_size);
    return 0;
}

void fs_image_abort(void)
{
    lfs_t *lfs = lfs_port_get();

    if (lfs == NULL) {
        return;
    }

    if (s_tmp_open) {
        lfs_file_close(lfs, &s_tmp_file);
        s_tmp_open = 0U;
    }

    lfs_remove(lfs, FS_IMAGE_TMP_PATH);
    s_expected_size = 0U;
    s_written_size = 0U;
}

int fs_image_file_exists(const char *path)
{
    lfs_t *lfs = lfs_port_get();
    struct lfs_info info;

    if ((lfs == NULL) || (path == NULL)) {
        return 0;
    }

    return (lfs_stat(lfs, path, &info) == LFS_ERR_OK &&
            info.type == LFS_TYPE_REG);
}

const char *fs_image_get_final_path(void)
{
    return s_final_path[0] ? s_final_path : FS_IMAGE_FINAL_PATH;
}

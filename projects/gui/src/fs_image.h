/******************************************************************************
 * @file     fs_image.h
 * @brief    图片文件 LittleFS 保存策略：临时文件接收，校验成功后原子替换
 ******************************************************************************/

#ifndef FS_IMAGE_H
#define FS_IMAGE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FS_IMAGE_TMP_PATH       "/image_tmp.jpg"
#define FS_IMAGE_FINAL_PATH     "/image.jpg"
#define FS_IMAGE_MAX_NAME_LEN   64U
#define FS_IMAGE_MAX_PATH_LEN   (FS_IMAGE_MAX_NAME_LEN + 2U)

/* 128KB LittleFS 分区需要预留元数据空间，默认限制单个接收文件不超过 120KB。 */
#ifndef FS_IMAGE_MAX_FILE_SIZE
#define FS_IMAGE_MAX_FILE_SIZE  (120U * 1024U)
#endif

/* LittleFS 写入和元数据需要预留空间，避免估算刚好够但实际写入失败。 */
#ifndef FS_IMAGE_SPACE_RESERVE_BYTES
#define FS_IMAGE_SPACE_RESERVE_BYTES (4U * 1024U)
#endif

int fs_image_mount(void);
int fs_image_format(void);
int fs_image_prepare_receive(const char *filename, uint32_t file_size,
                             uint32_t *free_bytes);
int fs_image_begin(const char *filename, uint32_t file_size);
int fs_image_write(uint32_t offset, const uint8_t *data, uint32_t len);
int fs_image_finish(uint32_t expected_crc32);
void fs_image_abort(void);
int fs_image_file_exists(const char *path);
int fs_image_crc32_file(const char *path, uint32_t *out_crc32);
const char *fs_image_get_final_path(void);

#ifdef __cplusplus
}
#endif

#endif /* FS_IMAGE_H */

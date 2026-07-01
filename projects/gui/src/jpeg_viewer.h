/******************************************************************************
 * @file     jpeg_viewer.h
 * @brief    从 LittleFS 文件流式 JPEG 解码并显示到 LCD
 ******************************************************************************/

#ifndef JPEG_VIEWER_H
#define JPEG_VIEWER_H

#ifdef __cplusplus
extern "C" {
#endif

int jpeg_viewer_show_file(const char *path);

#ifdef __cplusplus
}
#endif

#endif /* JPEG_VIEWER_H */

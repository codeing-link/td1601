/******************************************************************************
 * @file     image_gallery.h
 * @brief    LittleFS 图片库：扫描 JPG、显示当前图片、上下滑切换
 ******************************************************************************/

#ifndef IMAGE_GALLERY_H
#define IMAGE_GALLERY_H

#include <stdint.h>
#include "CST816.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef IMAGE_GALLERY_MAX_IMAGES
#define IMAGE_GALLERY_MAX_IMAGES      16U
#endif

#define IMAGE_GALLERY_CURRENT_PATH    "/.current_image"

int image_gallery_init(void);
int image_gallery_refresh(void);
int image_gallery_show_saved_or_first(void);
int image_gallery_show_path(const char *path);
int image_gallery_next(void);
int image_gallery_prev(void);
void image_gallery_handle_touch(const cst816_data_t *touch);
uint32_t image_gallery_count(void);
const char *image_gallery_current_path(void);

#ifdef __cplusplus
}
#endif

#endif /* IMAGE_GALLERY_H */

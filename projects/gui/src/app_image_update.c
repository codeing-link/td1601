/******************************************************************************
 * @file     app_image_update.c
 * @brief    图片更新应用入口，连接 transport、file_transfer、LittleFS 和图片库
 ******************************************************************************/

#include <stdio.h>
#include "app_image_update.h"
#include "file_transfer.h"
#include "fs_image.h"
#include "image_gallery.h"
#include "CST816.h"

int image_update_init(const transport_t *transport)
{
    if (fs_image_mount() != 0) {
        printf("[image_update] fs mount failed\r\n");
        return -1;
    }

    if (file_transfer_init(transport) != 0) {
        printf("[image_update] file transfer init failed\r\n");
        return -1;
    }

    /* 上电先扫描 LittleFS 图片：有图显示上次记录的图片，无图则保持空屏 */
    (void)image_gallery_show_saved_or_first();

    printf("[image_update] ready\r\n");
    return 0;
}

void image_update_poll(void)
{
    cst816_data_t touch;

    file_transfer_poll();

    /* DATA 模式下左右滑切换 LittleFS 中已有 JPG 图片 */
    if (Touch_Poll(&touch)) {
        image_gallery_handle_touch(&touch);
    }
}

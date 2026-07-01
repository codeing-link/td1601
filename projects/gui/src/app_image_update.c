/******************************************************************************
 * @file     app_image_update.c
 * @brief    图片更新应用入口，连接 transport、file_transfer、LittleFS
 ******************************************************************************/

#include <stdio.h>
#include "app_image_update.h"
#include "file_transfer.h"
#include "fs_image.h"

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

    printf("[image_update] ready\r\n");
    return 0;
}

void image_update_poll(void)
{
    file_transfer_poll();
}

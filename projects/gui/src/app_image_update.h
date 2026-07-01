/******************************************************************************
 * @file     app_image_update.h
 * @brief    图片更新应用入口
 ******************************************************************************/

#ifndef APP_IMAGE_UPDATE_H
#define APP_IMAGE_UPDATE_H

#include "transport_if.h"

#ifdef __cplusplus
extern "C" {
#endif

int image_update_init(const transport_t *transport);
void image_update_poll(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_IMAGE_UPDATE_H */

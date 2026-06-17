#ifndef EEZ_LVGL_UI_IMAGES_H
#define EEZ_LVGL_UI_IMAGES_H

#include <lvgl/lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const lv_img_dsc_t img_popup_button_mode_cold_off_background_image;
extern const lv_img_dsc_t img_popup_button_fan_high_off_background_image;
extern const lv_img_dsc_t img_popup_button_fan_level_down_off_background_image;
extern const lv_img_dsc_t img_popup_button_kt_off_background_image;
extern const lv_img_dsc_t img_popup_button_mode_cold_on_background_image;
extern const lv_img_dsc_t img_popup_button_mode_hum_off_background_image;
extern const lv_img_dsc_t img_popup_button_mode_hum_on_background_image;
extern const lv_img_dsc_t img_popup_button_mode_flash_off_background_image;
extern const lv_img_dsc_t img_popup_button_mode_flash_on_background_image;
extern const lv_img_dsc_t img_popup_button_mode_hot_off_background_image;
extern const lv_img_dsc_t img_popup_button_mode_hot_on_background_image;
extern const lv_img_dsc_t img_popup_button_mode_set_back_background_image;

#ifndef EXT_IMG_DESC_T
#define EXT_IMG_DESC_T
typedef struct _ext_img_desc_t {
    const char *name;
    const lv_img_dsc_t *img_dsc;
} ext_img_desc_t;
#endif

extern const ext_img_desc_t images[12];

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_IMAGES_H*/
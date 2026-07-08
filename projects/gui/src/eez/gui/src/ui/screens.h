#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl/lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// Screens

enum ScreensEnum {
    _SCREEN_ID_FIRST = 1,
    SCREEN_ID_MODE = 1,
    SCREEN_ID_MAIN = 2,
    SCREEN_ID_COLD = 3,
    _SCREEN_ID_LAST = 3
};

typedef struct _objects_t {
    lv_obj_t *mode;
    lv_obj_t *main;
    lv_obj_t *cold;
    lv_obj_t *obj0;
    lv_obj_t *obj1;
    lv_obj_t *obj2;
    lv_obj_t *obj3;
    lv_obj_t *v;
    lv_obj_t *obj4;
    lv_obj_t *obj5;
    lv_obj_t *obj6;
    lv_obj_t *obj7;
    lv_obj_t *snow;
    lv_obj_t *obj8;
} objects_t;

extern objects_t objects;

void create_screen_mode();
void tick_screen_mode();

void create_screen_main();
void tick_screen_main();

void create_screen_cold();
void tick_screen_cold();

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/
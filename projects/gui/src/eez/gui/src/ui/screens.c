#include <string.h>

#include "screens.h"
#include "images.h"
#include "fonts.h"
#include "actions.h"
#include "vars.h"
#include "styles.h"
#include "ui.h"

#include <string.h>

objects_t objects;

static const char *screen_names[] = { "mode", "Main", "cold" };
static const char *object_names[] = { "mode", "main", "cold", "obj0", "obj1", "obj2", "obj3", "v", "obj4", "obj5", "obj6", "obj7", "snow", "obj8" };

//
// Event handlers
//

lv_obj_t *tick_value_change_obj;

static void event_handler_cb_mode_obj0(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    (void)flowState;
    
    if (event == LV_EVENT_PRESSED) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, 5, 0, e);
    }
}

static void event_handler_cb_main_main(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    (void)flowState;
    
    if (event == LV_EVENT_CLICKED) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, 1, 0, e);
    }
}

static void event_handler_cb_main_obj1(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    (void)flowState;
    
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target_obj(e);
        if (tick_value_change_obj != ta) {
            int32_t value = lv_arc_get_value(ta);
            assignIntegerProperty(flowState, 0, 3, value, "Failed to assign Value in Arc widget");
        }
    }
}

static void event_handler_cb_main_obj3(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    (void)flowState;
    
    if (event == LV_EVENT_CLICKED) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, 3, 0, e);
    }
}

static void event_handler_cb_main_obj4(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    (void)flowState;
    
    if (event == LV_EVENT_CLICKED) {
        e->user_data = (void *)0;
        action_action_screen_off(e);
    }
}

static void event_handler_cb_main_obj5(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    (void)flowState;
    
    if (event == LV_EVENT_CLICKED) {
        e->user_data = (void *)0;
        action_light_dec(e);
    }
}

static void event_handler_cb_main_obj6(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    (void)flowState;
    
    if (event == LV_EVENT_CLICKED) {
        e->user_data = (void *)0;
        action_light_add(e);
    }
}

static void event_handler_cb_main_obj7(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    (void)flowState;
    
    if (event == LV_EVENT_CLICKED) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, 8, 0, e);
    }
}

static void event_handler_cb_cold_obj8(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    (void)flowState;
    
    if (event == LV_EVENT_CLICKED) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, 2, 0, e);
    }
}

//
// Screens
//

void create_screen_mode() {
    void *flowState = getFlowState(0, 0);
    (void)flowState;
    lv_obj_t *obj = lv_obj_create(0);
    objects.mode = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 360, 360);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            lv_obj_set_pos(obj, 54, 51);
            lv_obj_set_size(obj, 252, 258);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_SCROLL_ONE);
            lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_ACTIVE);
            lv_obj_set_scroll_dir(obj, LV_DIR_VER);
            lv_obj_set_scroll_snap_x(obj, LV_SCROLL_SNAP_NONE);
            lv_obj_set_scroll_snap_y(obj, LV_SCROLL_SNAP_CENTER);
            lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_COLUMN, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_imagebutton_create(parent_obj);
                    lv_obj_set_pos(obj, 31, 12);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, 40);
                    lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_popup_button_mode_hum_off_background_image, NULL);
                }
                {
                    lv_obj_t *obj = lv_imagebutton_create(parent_obj);
                    lv_obj_set_pos(obj, 11, 70);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, 40);
                    lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_popup_button_mode_hot_off_background_image, NULL);
                }
                {
                    lv_obj_t *obj = lv_imagebutton_create(parent_obj);
                    lv_obj_set_pos(obj, 99, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, 40);
                    lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_popup_button_mode_flash_off_background_image, NULL);
                }
                {
                    lv_obj_t *obj = lv_imagebutton_create(parent_obj);
                    objects.obj0 = obj;
                    lv_obj_set_pos(obj, 160, 20);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, 40);
                    lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_popup_button_mode_cold_on_background_image, NULL);
                    lv_obj_add_event_cb(obj, event_handler_cb_mode_obj0, LV_EVENT_ALL, flowState);
                }
                {
                    lv_obj_t *obj = lv_imagebutton_create(parent_obj);
                    lv_obj_set_pos(obj, 31, 191);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, 40);
                    lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_popup_button_fan_level_down_off_background_image, NULL);
                }
                {
                    lv_obj_t *obj = lv_imagebutton_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 129);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, 40);
                    lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_popup_button_fan_high_off_background_image, NULL);
                }
            }
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 193, 184);
            lv_obj_set_size(obj, 85, 16);
            lv_label_set_text_static(obj, "hello Lierda");
        }
    }
    
    tick_screen_mode();
}

void tick_screen_mode() {
    void *flowState = getFlowState(0, 0);
    (void)flowState;
}

void create_screen_main() {
    void *flowState = getFlowState(0, 1);
    (void)flowState;
    lv_obj_t *obj = lv_obj_create(0);
    objects.main = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 360, 360);
    lv_obj_add_event_cb(obj, event_handler_cb_main_main, LV_EVENT_ALL, flowState);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_CHECKABLE);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_arc_create(parent_obj);
            objects.obj1 = obj;
            lv_obj_set_pos(obj, 66, 51);
            lv_obj_set_size(obj, 232, 225);
            lv_arc_set_range(obj, 0, 40);
            lv_arc_set_bg_start_angle(obj, 180);
            lv_arc_set_bg_end_angle(obj, 0);
            lv_obj_add_event_cb(obj, event_handler_cb_main_obj1, LV_EVENT_ALL, flowState);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_CHECKABLE);
            lv_obj_set_style_arc_color(obj, lv_color_hex(0x2196f3), LV_PART_INDICATOR | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj2 = obj;
            lv_obj_set_pos(obj, 159, 78);
            lv_obj_set_size(obj, 35, 25);
            lv_obj_set_style_text_font(obj, &ui_font_cn_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "制冷");
        }
        {
            lv_obj_t *obj = lv_imagebutton_create(parent_obj);
            objects.obj3 = obj;
            lv_obj_set_pos(obj, 50, 183);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, 40);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_popup_button_mode_cold_off_background_image, NULL);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_PRESSED, NULL, &img_popup_button_mode_cold_on_background_image, NULL);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_CHECKED_RELEASED, NULL, &img_popup_button_mode_cold_on_background_image, NULL);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_CHECKED_PRESSED, NULL, &img_popup_button_mode_cold_off_background_image, NULL);
            lv_obj_add_event_cb(obj, event_handler_cb_main_obj3, LV_EVENT_ALL, flowState);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_CHECKABLE);
        }
        {
            // v
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.v = obj;
            lv_obj_set_pos(obj, 153, 99);
            lv_obj_set_size(obj, 200, 30);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            lv_obj_t *obj = lv_imagebutton_create(parent_obj);
            objects.obj4 = obj;
            lv_obj_set_pos(obj, 162, 267);
            lv_obj_set_size(obj, 40, 40);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_popup_button_kt_off_background_image, NULL);
            lv_obj_add_event_cb(obj, event_handler_cb_main_obj4, LV_EVENT_ALL, flowState);
        }
        {
            lv_obj_t *obj = lv_imagebutton_create(parent_obj);
            objects.obj5 = obj;
            lv_obj_set_pos(obj, 159, 180);
            lv_obj_set_size(obj, 40, 40);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_popup_button_mode_hot_off_background_image, NULL);
            lv_obj_add_event_cb(obj, event_handler_cb_main_obj5, LV_EVENT_ALL, flowState);
        }
        {
            lv_obj_t *obj = lv_imagebutton_create(parent_obj);
            objects.obj6 = obj;
            lv_obj_set_pos(obj, 258, 183);
            lv_obj_set_size(obj, 40, 40);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_popup_button_mode_hot_on_background_image, NULL);
            lv_obj_add_event_cb(obj, event_handler_cb_main_obj6, LV_EVENT_ALL, flowState);
        }
        {
            lv_obj_t *obj = lv_imagebutton_create(parent_obj);
            objects.obj7 = obj;
            lv_obj_set_pos(obj, 66, 235);
            lv_obj_set_size(obj, 40, 40);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_popup_button_mode_set_back_background_image, NULL);
            lv_obj_add_event_cb(obj, event_handler_cb_main_obj7, LV_EVENT_ALL, flowState);
        }
    }
    
    tick_screen_main();
}

void tick_screen_main() {
    void *flowState = getFlowState(0, 1);
    (void)flowState;
    {
        int32_t new_val = evalIntegerProperty(flowState, 0, 3, "Failed to evaluate Value in Arc widget");
        int32_t cur_val = lv_arc_get_value(objects.obj1);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.obj1;
            lv_arc_set_value(objects.obj1, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = evalTextProperty(flowState, 4, 3, "Failed to evaluate Text in Label widget");
        const char *cur_val = lv_label_get_text(objects.v);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.v;
            lv_label_set_text(objects.v, new_val);
            tick_value_change_obj = NULL;
        }
    }
}

void create_screen_cold() {
    void *flowState = getFlowState(0, 2);
    (void)flowState;
    lv_obj_t *obj = lv_obj_create(0);
    objects.cold = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 360, 360);
    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // snow
            lv_obj_t *obj = lv_imagebutton_create(parent_obj);
            objects.snow = obj;
            lv_obj_set_pos(obj, 50, 140);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, 40);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_popup_button_mode_cold_off_background_image, NULL);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_PRESSED, NULL, &img_popup_button_mode_cold_on_background_image, NULL);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_CHECKED_RELEASED, NULL, &img_popup_button_mode_cold_on_background_image, NULL);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_CHECKED_PRESSED, NULL, &img_popup_button_mode_cold_off_background_image, NULL);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_CHECKABLE);
        }
        {
            lv_obj_t *obj = lv_imagebutton_create(parent_obj);
            objects.obj8 = obj;
            lv_obj_set_pos(obj, 37, 220);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, 40);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_popup_button_mode_set_back_background_image, NULL);
            lv_obj_add_event_cb(obj, event_handler_cb_cold_obj8, LV_EVENT_ALL, flowState);
        }
        {
            lv_obj_t *obj = lv_imagebutton_create(parent_obj);
            lv_obj_set_pos(obj, 96, 89);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, 40);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_popup_button_mode_hum_off_background_image, NULL);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_PRESSED, NULL, &img_popup_button_mode_hum_on_background_image, NULL);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_CHECKED_RELEASED, NULL, &img_popup_button_mode_hum_on_background_image, NULL);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_CHECKED_PRESSED, NULL, &img_popup_button_mode_hum_off_background_image, NULL);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_CHECKABLE);
        }
        {
            lv_obj_t *obj = lv_imagebutton_create(parent_obj);
            lv_obj_set_pos(obj, 160, 55);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, 40);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_popup_button_mode_flash_off_background_image, NULL);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_PRESSED, NULL, &img_popup_button_mode_flash_on_background_image, NULL);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_CHECKED_RELEASED, NULL, &img_popup_button_mode_flash_on_background_image, NULL);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_CHECKED_PRESSED, NULL, &img_popup_button_mode_flash_off_background_image, NULL);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_CHECKABLE);
        }
        {
            lv_obj_t *obj = lv_imagebutton_create(parent_obj);
            lv_obj_set_pos(obj, 227, 75);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, 40);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_popup_button_mode_hot_off_background_image, NULL);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_PRESSED, NULL, &img_popup_button_mode_hot_on_background_image, NULL);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_CHECKED_RELEASED, NULL, &img_popup_button_mode_hot_on_background_image, NULL);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_CHECKED_PRESSED, NULL, &img_popup_button_mode_hot_off_background_image, NULL);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_CHECKABLE);
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 184, 185);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text_static(obj, "Touch to light");
        }
    }
    
    tick_screen_cold();
}

void tick_screen_cold() {
    void *flowState = getFlowState(0, 2);
    (void)flowState;
}

typedef void (*tick_screen_func_t)();
tick_screen_func_t tick_screen_funcs[] = {
    tick_screen_mode,
    tick_screen_main,
    tick_screen_cold,
};
void tick_screen(int screen_index) {
    if (screen_index >= 0 && screen_index < 3) {
        tick_screen_funcs[screen_index]();
    }
}
void tick_screen_by_id(enum ScreensEnum screenId) {
    tick_screen(screenId - 1);
}

//
// Fonts
//

ext_font_desc_t fonts[] = {
    { "cn-14", &ui_font_cn_14 },
#if LV_FONT_MONTSERRAT_8
    { "MONTSERRAT_8", &lv_font_montserrat_8 },
#endif
#if LV_FONT_MONTSERRAT_10
    { "MONTSERRAT_10", &lv_font_montserrat_10 },
#endif
#if LV_FONT_MONTSERRAT_12
    { "MONTSERRAT_12", &lv_font_montserrat_12 },
#endif
#if LV_FONT_MONTSERRAT_14
    { "MONTSERRAT_14", &lv_font_montserrat_14 },
#endif
#if LV_FONT_MONTSERRAT_16
    { "MONTSERRAT_16", &lv_font_montserrat_16 },
#endif
#if LV_FONT_MONTSERRAT_18
    { "MONTSERRAT_18", &lv_font_montserrat_18 },
#endif
#if LV_FONT_MONTSERRAT_20
    { "MONTSERRAT_20", &lv_font_montserrat_20 },
#endif
#if LV_FONT_MONTSERRAT_22
    { "MONTSERRAT_22", &lv_font_montserrat_22 },
#endif
#if LV_FONT_MONTSERRAT_24
    { "MONTSERRAT_24", &lv_font_montserrat_24 },
#endif
#if LV_FONT_MONTSERRAT_26
    { "MONTSERRAT_26", &lv_font_montserrat_26 },
#endif
#if LV_FONT_MONTSERRAT_28
    { "MONTSERRAT_28", &lv_font_montserrat_28 },
#endif
#if LV_FONT_MONTSERRAT_30
    { "MONTSERRAT_30", &lv_font_montserrat_30 },
#endif
#if LV_FONT_MONTSERRAT_32
    { "MONTSERRAT_32", &lv_font_montserrat_32 },
#endif
#if LV_FONT_MONTSERRAT_34
    { "MONTSERRAT_34", &lv_font_montserrat_34 },
#endif
#if LV_FONT_MONTSERRAT_36
    { "MONTSERRAT_36", &lv_font_montserrat_36 },
#endif
#if LV_FONT_MONTSERRAT_38
    { "MONTSERRAT_38", &lv_font_montserrat_38 },
#endif
#if LV_FONT_MONTSERRAT_40
    { "MONTSERRAT_40", &lv_font_montserrat_40 },
#endif
#if LV_FONT_MONTSERRAT_42
    { "MONTSERRAT_42", &lv_font_montserrat_42 },
#endif
#if LV_FONT_MONTSERRAT_44
    { "MONTSERRAT_44", &lv_font_montserrat_44 },
#endif
#if LV_FONT_MONTSERRAT_46
    { "MONTSERRAT_46", &lv_font_montserrat_46 },
#endif
#if LV_FONT_MONTSERRAT_48
    { "MONTSERRAT_48", &lv_font_montserrat_48 },
#endif
};

//
//
//

void create_screens() {
    
    eez_flow_init_fonts(fonts, sizeof(fonts) / sizeof(ext_font_desc_t));

// Set default LVGL theme
    lv_display_t *dispp = lv_display_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), true, LV_FONT_DEFAULT);
    lv_display_set_theme(dispp, theme);
    
    // Initialize screens
    eez_flow_init_screen_names(screen_names, sizeof(screen_names) / sizeof(const char *));
    eez_flow_init_object_names(object_names, sizeof(object_names) / sizeof(const char *));
    
    // Create screens
    create_screen_mode();
    create_screen_main();
    create_screen_cold();
}
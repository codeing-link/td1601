#include <string.h>
#include <math.h>

#include "screens.h"
#include "images.h"
#include "fonts.h"
#include "actions.h"
#include "vars.h"
#include "styles.h"
#include "ui.h"

objects_t objects;
lv_obj_t *tick_value_change_obj;

static const char *screen_names[] = { "mode", "Main", "cold" };
static const char *object_names[] = { "mode", "main", "cold", "obj0", "obj1", "obj2", "obj3", "v", "snow", "obj4" };

//
// Event handlers
//

/* ---- cold screen fly-in animation ----
 * Total run time = (stagger of last icon + FLY_FRAMES) * timer_period.
 * timer_period is capped at LV_DEF_REFR_PERIOD (16 ms) since LVGL won't
 * refresh faster than that, so we speed the animation up by using fewer
 * frames per icon (FLY_FRAMES) and a tighter launch stagger (FLY_STAGGER).
 *   old:  (4*5 + 30) * 16ms ~= 800 ms
 *   prev: (4*3 + 20) * 16ms ~= 512 ms
 *   new:  (4*2 + 10) * 16ms ~= 288 ms  (~2x faster than prev)
 */
#define FLY_FRAMES   10    /* frames each icon takes to reach its target */
#define FLY_STAGGER   2    /* frame delay between consecutive icon launches */
#define FLY_SX       0
#define FLY_SY     220

static lv_obj_t  *fly_btns[5];
static int32_t    fly_tx[5], fly_ty[5], fly_tick[5];
static lv_timer_t *fly_timer = NULL;

static void cold_fly_cb(lv_timer_t *t) {
    (void)t;
    bool all_done = true;
    for (int i = 0; i < 5; i++) {
        int32_t f = fly_tick[i];
        if (f < 0)          { fly_tick[i]++; all_done = false; continue; }
        if (f > FLY_FRAMES) { continue; }
        all_done = false;
        float p  = (float)f / FLY_FRAMES;
        float ep = 1.0f - (1.0f - p) * (1.0f - p);
        float arc_p = 4.0f * ep * (1.0f - ep);
        int32_t x = (int32_t)(FLY_SX + (fly_tx[i] - FLY_SX) * ep + 0.5f);
        int32_t y = (int32_t)(FLY_SY + (fly_ty[i] - FLY_SY) * ep - 60.0f * arc_p + 0.5f);
        lv_obj_set_pos(fly_btns[i], x, y);
        fly_tick[i]++;
    }
    if (all_done) { lv_timer_delete(fly_timer); fly_timer = NULL; }
}

static void cold_screen_loaded_cb(lv_event_t *e) {
    (void)e;
    fly_btns[0] = lv_obj_get_child(objects.cold, 0); fly_tx[0]=24;  fly_ty[0]=88;
    fly_btns[1] = lv_obj_get_child(objects.cold, 2); fly_tx[1]=50;  fly_ty[1]=32;
    fly_btns[2] = lv_obj_get_child(objects.cold, 3); fly_tx[2]=120; fly_ty[2]=32;
    fly_btns[3] = lv_obj_get_child(objects.cold, 4); fly_tx[3]=180; fly_ty[3]=75;
    fly_btns[4] = lv_obj_get_child(objects.cold, 1); fly_tx[4]=44;  fly_ty[4]=148;
    for (int i = 0; i < 5; i++) {
        fly_tick[i] = -(i * FLY_STAGGER);
        lv_obj_set_pos(fly_btns[i], FLY_SX, FLY_SY);
    }
    if (fly_timer) { lv_timer_delete(fly_timer); fly_timer = NULL; }
    fly_timer = lv_timer_create(cold_fly_cb, 16, NULL);
}



static void event_handler_cb_mode_obj0(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    (void)flowState;

    if (event == LV_EVENT_PRESSED) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, 5, 0, e);
    }
}

/* ---- dynamic menu state ---- */
#define MODE_BTN_COUNT  6
#define MODE_BASE_SZ    60    /* icon size at center (px) */
#define MODE_STEP       70    /* virtual spacing between icon centers */
#define MODE_CENTER_X   120   /* screen center x */
#define MODE_CENTER_Y   120   /* screen center y */

static lv_obj_t *mode_btns[MODE_BTN_COUNT];
static float mode_offset   = 0.0f;
static float mode_velocity = 0.0f;
static int32_t mode_last_y = 0;
static bool mode_dragging  = false;

static void mode_layout_update(void) {
    for (int i = 0; i < MODE_BTN_COUNT; i++) {
        float dy = i * (float)MODE_STEP - mode_offset;  /* signed dist from center */
        float adist = dy < 0 ? -dy : dy;

        /* hide if more than 1.5 steps away */
        if (adist > (float)MODE_STEP * 1.5f) {
            lv_obj_add_flag(mode_btns[i], LV_OBJ_FLAG_HIDDEN);
            continue;
        }
        lv_obj_clear_flag(mode_btns[i], LV_OBJ_FLAG_HIDDEN);

        /* 3D effect: map distance to angle on a cylinder
         * angle 0 = facing forward, angle 90 = side-on (invisible)
         * use t = adist/STEP as normalized angle input */
        float angle = adist / (float)MODE_STEP * 70.0f;  /* degrees, max ~70 at ±1 step */
        float cos_a = cosf(angle * 3.14159f / 180.0f);   /* x-compression factor */
        float depth = 1.0f - adist / ((float)MODE_STEP * 3.0f); /* overall scale 1→0.67 */

        uint32_t scale_x = (uint32_t)(256.0f * cos_a * depth + 0.5f);
        uint32_t scale_y = (uint32_t)(256.0f * depth + 0.5f);
        if (scale_x < 32) scale_x = 32;
        if (scale_y < 32) scale_y = 32;

        lv_image_set_scale_x(mode_btns[i], scale_x);
        lv_image_set_scale_y(mode_btns[i], scale_y);

        /* x: compressed image centered horizontally */
        float vis_w = (float)MODE_BASE_SZ * scale_x / 256.0f;
        float vis_h = (float)MODE_BASE_SZ * scale_y / 256.0f;
        int32_t x = (int32_t)(MODE_CENTER_X - vis_w / 2.0f + 0.5f);
        int32_t y = (int32_t)(MODE_CENTER_Y + dy - vis_h / 2.0f + 0.5f);
        lv_obj_set_pos(mode_btns[i], x, y);
    }
}

/* inertia timer */
static lv_timer_t *mode_inertia_timer = NULL;

static void mode_inertia_cb(lv_timer_t *t) {
    (void)t;
    mode_offset += mode_velocity;
    mode_velocity *= 0.88f;

    /* clamp */
    float min_off = 0;
    float max_off = (MODE_BTN_COUNT - 1) * (float)MODE_STEP;
    if (mode_offset < min_off) { mode_offset = min_off; mode_velocity = 0; }
    if (mode_offset > max_off) { mode_offset = max_off; mode_velocity = 0; }

    mode_layout_update();

    if (mode_velocity > -0.5f && mode_velocity < 0.5f) {
        float step = (float)MODE_STEP;
        float nearest = (float)(int32_t)((mode_offset / step) + 0.5f) * step;
        mode_offset += (nearest - mode_offset) * 0.15f;
        mode_layout_update();
        if (mode_velocity == 0 &&
            (mode_offset > nearest - 0.5f) && (mode_offset < nearest + 0.5f)) {
            mode_offset = nearest;
            mode_layout_update();
            lv_timer_delete(mode_inertia_timer);
            mode_inertia_timer = NULL;
        }
    }
}

static int32_t mode_press_start_y = 0;

static void event_handler_mode_gesture(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_indev_t *indev = lv_indev_active();
    if (!indev) return;

    lv_point_t pt;
    lv_indev_get_point(indev, &pt);

    if (code == LV_EVENT_PRESSED) {
        mode_dragging = true;
        mode_last_y = pt.y;
        mode_press_start_y = pt.y;
        mode_velocity = 0;
        if (mode_inertia_timer) {
            lv_timer_delete(mode_inertia_timer);
            mode_inertia_timer = NULL;
        }
    } else if (code == LV_EVENT_PRESSING && mode_dragging) {
        int32_t dy = mode_last_y - pt.y;
        mode_last_y = pt.y;
        mode_velocity = (float)dy;
        mode_offset += (float)dy;

        float min_off = 0, max_off = (MODE_BTN_COUNT - 1) * (float)MODE_STEP;
        if (mode_offset < min_off) mode_offset = min_off;
        if (mode_offset > max_off) mode_offset = max_off;

        mode_layout_update();
    } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        mode_dragging = false;
        /* tap: total drag < 8px → fire the centered button's event */
        int32_t total = pt.y - mode_press_start_y;
        if (total < 0) total = -total;
        if (total < 8 && code == LV_EVENT_RELEASED) {
            void *fs = getFlowState(0, 0);
            flowPropagateValueLVGLEvent(fs, 5, 0, e);
        }
        if (mode_inertia_timer == NULL)
            mode_inertia_timer = lv_timer_create(mode_inertia_cb, 16, NULL);
    }
}

static void event_handler_cb_main_obj3(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    (void)flowState;
    
    if (event == LV_EVENT_CLICKED) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, 6, 0, e);
    }
}

static void event_handler_cb_cold_obj4(lv_event_t *e) {
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
    lv_obj_set_size(obj, 240, 240);
    lv_obj_set_style_bg_opa(obj, 0, 0);
    lv_obj_set_style_border_width(obj, 0, 0);

    lv_obj_t *cont = lv_obj_create(obj);
    lv_obj_set_pos(cont, 0, 0);
    lv_obj_set_size(cont, 240, 240);
    lv_obj_set_style_pad_all(cont, 0, 0);
    lv_obj_set_style_bg_opa(cont, 0, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_radius(cont, 0, 0);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(cont, LV_OBJ_FLAG_OVERFLOW_VISIBLE);

    /* gesture catcher — same size, sits on top */
    lv_obj_t *touch = lv_obj_create(obj);
    lv_obj_set_pos(touch, 0, 0);
    lv_obj_set_size(touch, 240, 240);
    lv_obj_set_style_bg_opa(touch, 0, 0);
    lv_obj_set_style_border_width(touch, 0, 0);
    lv_obj_set_style_radius(touch, 0, 0);
    lv_obj_clear_flag(touch, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(touch, event_handler_mode_gesture, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(touch, event_handler_mode_gesture, LV_EVENT_PRESSING, NULL);
    lv_obj_add_event_cb(touch, event_handler_mode_gesture, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(touch, event_handler_mode_gesture, LV_EVENT_PRESS_LOST, NULL);

    /* create buttons inside cont */
    const lv_img_dsc_t *srcs[MODE_BTN_COUNT] = {
        &img_popup_button_mode_hum_off_background_image,
        &img_popup_button_mode_hot_off_background_image,
        &img_popup_button_mode_flash_off_background_image,
        &img_popup_button_mode_cold_off_background_image,
        &img_popup_button_fan_level_down_off_background_image,
        &img_popup_button_fan_high_off_background_image,
    };
    for (int i = 0; i < MODE_BTN_COUNT; i++) {
        lv_obj_t *img = lv_image_create(cont);
        lv_image_set_src(img, srcs[i]);
        lv_image_set_pivot(img, 0, 0);
        lv_obj_set_style_bg_opa(img, 0, 0);
        lv_obj_set_style_border_width(img, 0, 0);
        lv_obj_add_flag(img, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_flag(img, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
        mode_btns[i] = img;
    }
    objects.obj0 = mode_btns[3];
    lv_obj_add_event_cb(mode_btns[3], event_handler_cb_mode_obj0, LV_EVENT_ALL, flowState);

    /* start with first item centered */
    mode_offset = 0;
    mode_velocity = 0;
    mode_layout_update();

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
    lv_obj_set_size(obj, 240, 240);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_CHECKABLE);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_arc_create(parent_obj);
            objects.obj1 = obj;
            lv_obj_set_pos(obj, 44, 41);
            lv_obj_set_size(obj, 155, 150);
            lv_arc_set_range(obj, 0, 40);
            lv_arc_set_value(obj, 25);
            lv_arc_set_bg_start_angle(obj, 180);
            lv_arc_set_bg_end_angle(obj, 0);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_CHECKABLE);
            lv_obj_set_style_arc_color(obj, lv_color_hex(0x2196f3), LV_PART_INDICATOR | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj2 = obj;
            lv_obj_set_pos(obj, 106, 72);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &ui_font_cn_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "制冷");
        }
        {
            lv_obj_t *obj = lv_image_create(parent_obj);
            lv_obj_set_pos(obj, 151, 136);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_popup_button_fan_high_off_background_image);
        }
        {
            lv_obj_t *obj = lv_image_create(parent_obj);
            lv_obj_set_pos(obj, 100, 120);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_popup_button_fan_level_down_off_background_image);
        }
        {
            lv_obj_t *obj = lv_image_create(parent_obj);
            lv_obj_set_pos(obj, 101, 160);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_popup_button_kt_off_background_image);
        }
        {
            lv_obj_t *obj = lv_imagebutton_create(parent_obj);
            objects.obj3 = obj;
            lv_obj_set_pos(obj, 32, 140);
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
            lv_obj_set_pos(obj, 107, 95);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
    }
    
    tick_screen_main();
}

void tick_screen_main() {
    void *flowState = getFlowState(0, 1);
    (void)flowState;
    {
        const char *new_val = evalTextProperty(flowState, 7, 3, "Failed to evaluate Text in Label widget");
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
    lv_obj_set_size(obj, 240, 240);
    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // snow
            lv_obj_t *obj = lv_imagebutton_create(parent_obj);
            objects.snow = obj;
            lv_obj_set_pos(obj, 24, 88);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, 40);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_popup_button_mode_cold_off_background_image, NULL);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_PRESSED, NULL, &img_popup_button_mode_cold_on_background_image, NULL);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_CHECKED_RELEASED, NULL, &img_popup_button_mode_cold_on_background_image, NULL);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_CHECKED_PRESSED, NULL, &img_popup_button_mode_cold_off_background_image, NULL);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_CHECKABLE);
        }
        {
            lv_obj_t *obj = lv_imagebutton_create(parent_obj);
            objects.obj4 = obj;
            lv_obj_set_pos(obj, 44, 148);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, 40);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_popup_button_mode_set_back_background_image, NULL);
            lv_obj_add_event_cb(obj, event_handler_cb_cold_obj4, LV_EVENT_ALL, flowState);
        }
        {
            lv_obj_t *obj = lv_imagebutton_create(parent_obj);
            lv_obj_set_pos(obj, 50, 32);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, 40);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_popup_button_mode_hum_off_background_image, NULL);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_PRESSED, NULL, &img_popup_button_mode_hum_on_background_image, NULL);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_CHECKED_RELEASED, NULL, &img_popup_button_mode_hum_on_background_image, NULL);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_CHECKED_PRESSED, NULL, &img_popup_button_mode_hum_off_background_image, NULL);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_CHECKABLE);
        }
        {
            lv_obj_t *obj = lv_imagebutton_create(parent_obj);
            lv_obj_set_pos(obj, 120, 32);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, 40);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_popup_button_mode_flash_off_background_image, NULL);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_PRESSED, NULL, &img_popup_button_mode_flash_on_background_image, NULL);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_CHECKED_RELEASED, NULL, &img_popup_button_mode_flash_on_background_image, NULL);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_CHECKED_PRESSED, NULL, &img_popup_button_mode_flash_off_background_image, NULL);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_CHECKABLE);
        }
        {
            lv_obj_t *obj = lv_imagebutton_create(parent_obj);
            lv_obj_set_pos(obj, 180, 75);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, 40);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_popup_button_mode_hot_off_background_image, NULL);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_PRESSED, NULL, &img_popup_button_mode_hot_on_background_image, NULL);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_CHECKED_RELEASED, NULL, &img_popup_button_mode_hot_on_background_image, NULL);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_CHECKED_PRESSED, NULL, &img_popup_button_mode_hot_off_background_image, NULL);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_CHECKABLE);
        }
    }
    
    tick_screen_cold();

    /* register: start fly-in when screen becomes active */
    lv_obj_add_event_cb(obj, cold_screen_loaded_cb, LV_EVENT_SCREEN_LOADED, NULL);
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
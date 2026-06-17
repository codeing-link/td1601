#include <stdio.h>
#include <board_init.h>
#include "lcd_drv.h"
#include "ft3267_drv.h"
#include "lv_port.h"
#include "lvgl/lvgl.h"
#include "ui.h"

int main(void)
{
    board_init();

    lcd_init();
    ft3267_init();

    lv_init();
    lv_port_init();
	
    printf("[DBG] ui_init...\n");
#if 1
    ui_init();
#else
    lv_obj_t *label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "Hello world");
    lv_obj_center(label);
#endif
	printf("[DBG] ui_init done\n");

    while (1) {
        ui_tick();
        /*
         * lv_timer_handler() returns the time (ms) until it next needs to run.
         * Sleep only that long (capped to a small value) instead of a fixed
         * 5 ms busy-wait, so refreshes aren't artificially throttled.
         */
        uint32_t idle = lv_timer_handler();
        if (idle == LV_NO_TIMER_READY || idle > 5) {
            idle = 5;
        }
        if (idle > 0) {
            mdelay(idle);
        }
    }

    return 0;
}

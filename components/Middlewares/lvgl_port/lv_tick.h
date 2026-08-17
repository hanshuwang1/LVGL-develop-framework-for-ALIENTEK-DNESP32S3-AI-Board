#ifndef _COMPONENTS_MIDDLEWARES_LVGL_PORT_LV_TICK_H
#define _COMPONENTS_MIDDLEWARES_LVGL_PORT_LV_TICK_H

#include "esp_timer.h"
#include "lvgl__lvgl/lvgl.h"

void lv_tick_task(void *arg);
void lv_tick_init(void);

#endif // _COMPONENTS_MIDDLEWARES_LVGL_PORT_LV_TICK_H

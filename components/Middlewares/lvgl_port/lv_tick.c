#include "lv_tick.h"

void lv_tick_task(void *arg)
{
    (void)arg;

    lv_tick_inc(1); /*Tell LVGL that 1 milliseconds were elapsed*/
}

void lv_tick_init(void)
{
    /*Initialize LVGL tick*/
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &lv_tick_task,
        .name = "lv_tick",
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .skip_unhandled_events = false
    };
    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, 1000)); /* 1000us = 1ms period */
}

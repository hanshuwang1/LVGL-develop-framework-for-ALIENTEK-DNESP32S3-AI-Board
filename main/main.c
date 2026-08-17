#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "nvs_flash.h"
#include "LED.h"
#include "myiic.h"
#include "xl9555.h"
#include "lcd.h"
#include "lv_port_disp.h"
#include "lv_tick.h"
#include "lv_port_indev.h"
#include "button.h"
#include "qma6100p.h"


qma6100p_rawdata_t rawdata;
qma6100p_rawdata_t filtered_data = {0};
static lv_display_rotation_t curr_disp_dir = LV_DISPLAY_ROTATION_0;
static lv_display_rotation_t target_disp_dir = LV_DISPLAY_ROTATION_0;
static SemaphoreHandle_t rotation_mutex = NULL;
static volatile bool rotation_pending = false;

/* 方向检测的稳定计数器 */
static uint8_t orientation_stable_count = 0;
static const uint8_t STABLE_THRESHOLD = 3; // 需要连续3次检测一致才确认

/**
 * @brief 根据pitch和roll角度判断屏幕方向
 */
static lv_display_rotation_t determine_orientation(float pitch, float roll)
{
    /* 横屏判断 */
    if (fabsf(roll) > 30.0f) {
        return roll > 0 ? LV_DISPLAY_ROTATION_90 : LV_DISPLAY_ROTATION_270;
    }
    
    /* 竖屏判断 */
    if (fabsf(pitch) > 30.0f) {
        return pitch > 0 ? LV_DISPLAY_ROTATION_180 : LV_DISPLAY_ROTATION_0;
    }
    
    /* 默认保持当前方向 */
    return curr_disp_dir;
}

static void accelerometer_task(void *pvParameter)
{
    lv_display_rotation_t new_rotation;
    lv_display_rotation_t temp_rotation;

    /* 初始化阶段：预热传感器和滤波器 */
    ESP_LOGI("accelerometer", "Initializing accelerometer...");
    for(int i = 0; i < 5; i++) {
        qma6100p_read_rawdata(&rawdata);
        
        /* 检查原始数据 */
        if(isnan(rawdata.pitch) || isnan(rawdata.roll)) {
            ESP_LOGW("accelerometer", "Invalid data during init, retry...");
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }
        
        /* 第一次直接使用原始值 */
        if(i == 0) {
            filtered_data = rawdata;
        } else {
            qma6100p_low_pass_filter(&rawdata, 0.5f, &filtered_data);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    ESP_LOGI("accelerometer", "Initialization complete: pitch=%.3f, roll=%.3f", 
             filtered_data.pitch, filtered_data.roll);

    for(;;)
    {
        qma6100p_read_rawdata(&rawdata);
        // ESP_LOGI("rawdata", "Pitch: %.3f, Roll: %.3f, Direction: %d", rawdata.pitch, rawdata.roll, curr_disp_dir);
        qma6100p_low_pass_filter(&rawdata, 0.7f, &filtered_data);
        new_rotation = determine_orientation(filtered_data.pitch, filtered_data.roll);
        if(new_rotation != curr_disp_dir){
            orientation_stable_count++;
            if(orientation_stable_count >= STABLE_THRESHOLD){
                /* 再次确认 */
                temp_rotation = determine_orientation(filtered_data.pitch, filtered_data.roll);
                if (temp_rotation == new_rotation) {
                    /* 获取互斥锁，设置目标旋转方向 */
                    if (xSemaphoreTake(rotation_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                        target_disp_dir = new_rotation;
                        rotation_pending = true;
                        xSemaphoreGive(rotation_mutex);
                        
                        // curr_disp_dir = new_rotation;
                        ESP_LOGI("accelerometer", "Direction changed to: %d", new_rotation);
                    }
                }
                orientation_stable_count = 0; // 重置计数器
            }
        } else{
            orientation_stable_count = 0; // 重置计数器
        }
        // ESP_LOGI("qma6100p pitch,roll,dir","%.3f, %.3f, %d",filtered_data.pitch, filtered_data.roll, new_rotation);
        vTaskDelay(pdMS_TO_TICKS(200)); // 200ms
    }
}

static void lvgl_task(void *pvParameter)
{
    lv_tick_init();
    lv_port_disp_init();
    lv_port_indev_init();
    button_show();

    while(1)
    {
        lv_task_handler();

        /* 检查是否有待处理的旋转操作 */
        if (rotation_pending) {
            if (xSemaphoreTake(rotation_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                if (rotation_pending && target_disp_dir != curr_disp_dir) {
                    /* LCD Hardware dir sync*/
                    lcd_sync_rotation(target_disp_dir);

                    /* 应用旋转 */
                    lv_display_set_rotation(lv_disp_get_default(), target_disp_dir);
                    ESP_LOGI("lvgl", "Display rotated to: %d", target_disp_dir);
                    
                    /* 刷新屏幕以应用新方向 */
                    lv_obj_invalidate(lv_scr_act());
                    curr_disp_dir = target_disp_dir;
                }
                rotation_pending = false;
                xSemaphoreGive(rotation_mutex);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(5));//任务让出来给核心0跑
    }
}

void app_main(void)
{
    esp_err_t ret;
    ret = nvs_flash_init();     /* 初始化NVS */
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    led_init();                 /* 初始化LED */
    myiic_init();               /* 初始化IIC0 */
    // xl9555_init();              /* 初始化XL9555 */
    qma6100p_init();            /* 初始化QMA6100P */

    /* 创建互斥锁用于任务同步 */
    rotation_mutex = xSemaphoreCreateMutex();
    if (rotation_mutex == NULL) {
        ESP_LOGE("main", "Failed to create rotation mutex");
        return;
    }

    xTaskCreatePinnedToCore(lvgl_task, "lvgl_task", 1024*8, NULL, 5, NULL, 1); /* CPU1 */
    xTaskCreatePinnedToCore(accelerometer_task, "accelerometer_task", 1024*8, NULL, 5, NULL, 0); /* CPU0 */
    while(1) /* CPU0 */
    {
        vTaskDelay(pdMS_TO_TICKS(500));
        // LED_TOGGLE();
    }
}

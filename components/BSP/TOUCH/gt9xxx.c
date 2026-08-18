/**
 ****************************************************************************************************
* @file        gt9xxx.c
* @author      正点原子团队(正点原子)
* @version     V1.0
* @date        2025-01-01
* @brief       5寸电容触摸屏-GT9xxx 驱动代码
*
* @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
****************************************************************************************************
* @attention
*
* 实验平台:正点原子 ESP32-S3 开发板
* 在线视频:www.yuanzige.com
* 技术论坛:www.openedv.com
* 公司网址:www.alientek.com
* 购买地址:openedv.taobao.com
*
****************************************************************************************************
*/

#include "gt9xxx.h"
/* 注意: 除了GT9271支持10点触摸之外, 其他触摸芯片只支持 5点触摸 */

const char * gt9xxx_tag = "GT911";
i2c_master_bus_handle_t gt9xxx_iic1_bus_handle;     /* 总线句柄 */
i2c_master_dev_handle_t gt9xxx_handle = NULL;
static esp_lcd_panel_io_handle_t s_gt9xxx_io_handle = NULL;
esp_lcd_touch_handle_t s_gt9xxx_touch_handle = NULL;

/**
 * @brief       初始化MYIIC1
 * @param       无
 * @retval      ESP_OK:初始化成功
 */
esp_err_t myiic1_init(void)
{
    i2c_master_bus_config_t i2c1_bus_config = {
        .clk_source                     = I2C_CLK_SRC_DEFAULT,  /* 时钟源 */
        .i2c_port                       = I2C_NUM_1,            /* I2C端口 */
        .scl_io_num                     = GT9XXX_IIC_SCL_PIN,          /* SCL管脚 */
        .sda_io_num                     = GT9XXX_IIC_SDA_PIN,          /* SDA管脚 */
        .glitch_ignore_cnt              = 7,                    /* 故障周期 */
        .flags.enable_internal_pullup   = true,                 /* 内部上拉 */
    };
    /* 新建I2C总线 */
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c1_bus_config, &gt9xxx_iic1_bus_handle));

    return ESP_OK;
}

/**
  * @brief GT911 init using offical driver
  * @param None
  * @retval ESP_OK if successful, otherwise an error code
  */
esp_err_t gt911_init(void)
{
    esp_err_t ret;
    if(gt9xxx_iic1_bus_handle == NULL) {
        myiic1_init();
    }
    /* GT911 I2C handler */
    // i2c_device_config_t gt9xxx_i2c_dev_conf = {
    //     .dev_addr_length = I2C_ADDR_BIT_LEN_7,      /* 从机地址长度 */
    //     .scl_speed_hz    = 400000,                  /* 传输速率 */
    //     .device_address  = GT9XXX_DEV_ID,           /* 从机7位的地址 */
    // };
    // /* GT911 to I2C bus */
    // ESP_ERROR_CHECK(i2c_master_bus_add_device(gt9xxx_iic1_bus_handle, &gt9xxx_i2c_dev_conf, &gt9xxx_handle));
    // ESP_LOGI(gt9xxx_tag, "GT911 I2C device added");
    /* GT911 地址选择配置 - 必须传入，否则驱动会跳过地址选择时序 */
    static esp_lcd_touch_io_gt911_config_t gt911_io_config = {
        .dev_addr = ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS,  /* 0x5D，对应 INT 拉低 */
    };

    /* panel io config */
    esp_lcd_panel_io_i2c_config_t io_config = {
        .dev_addr = ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS,
        .scl_speed_hz = 100000,
        .control_phase_bytes = 1,  /* 控制阶段字节数 */
        .lcd_cmd_bits = 16,        /* 命令位数 */
        .lcd_param_bits = 16,      /* 参数位数 */
        .flags = {
            .dc_low_on_data = 0,   /* 数据阶段DC引脚电平 */
            .disable_control_phase = 1, /* 禁用控制阶段 */
        },
    };
    // io_config =ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG ();

    /* panel io handler */
    ret = esp_lcd_new_panel_io_i2c(gt9xxx_iic1_bus_handle, &io_config, &s_gt9xxx_io_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(gt9xxx_tag, "Failed to create GT911 panel IO: %s", esp_err_to_name(ret));
        return 1;
    }

    /* touch config */
    esp_lcd_touch_config_t touch_config = {
        .x_max = MY_DISP_HOR_RES,
        .y_max = MY_DISP_VER_RES,
        .rst_gpio_num = GT9XXX_CT_RST_GPIO_PIN,  /* 复位引脚 */
        .int_gpio_num = GT9XXX_INT_GPIO_PIN,     /* 中断引脚 */
        .levels = {
            .reset = 0,
            .interrupt = 0,  /* if 1, gt911_dev_addr is 0x14 */
        },
        .flags = {
            .swap_xy = 0,   /* 不交换X/Y */
            .mirror_x = 0,  /* 不镜像X */
            .mirror_y = 0,  /* 不镜像Y */
        },
        .process_coordinates = NULL,    /* 坐标处理回调 */
        .interrupt_callback = NULL,     /* 中断回调 */
        .user_data = NULL,              /* 用户数据 */
        .driver_data = &gt911_io_config           /* 驱动数据 */
    };
    
    /* GT911 touch driver */
    ret = esp_lcd_touch_new_i2c_gt911(s_gt9xxx_io_handle, &touch_config, &s_gt9xxx_touch_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(gt9xxx_tag, "Failed to create GT911 touch driver: %s", esp_err_to_name(ret));
        return 1;
    }
    ESP_LOGI(gt9xxx_tag, "GT911 touch driver created successfully");
    return ESP_OK;
}

/**
  * @brief GT911 touch scan using offical driver
  * @param None
  * @retval 0: no touch, 1: touch detected
  */
uint8_t gt911_scan(void)
{
    static uint8_t t = 0;           /* 控制查询间隔，从而降低CPU占用率 */
    uint8_t res = 0;
    uint16_t tempsta;
    esp_lcd_touch_point_data_t touch_data[CONFIG_ESP_LCD_TOUCH_MAX_POINTS];  /* 官方触摸点数据结构 */;
    uint8_t point_num = 0;
    esp_err_t ret;
    bool pressed = false;
    t++;

    /* 空闲时，每进入10次函数才检测1次，从而节省CPU使用率 */
    if ((t % 10) != 0 && t >= 10) {
        if (t > 240) {
            t = 10;  /* 重新从10开始计数 */
        }
        return res;
    }

    /* 使用官方API读取触摸数据 */
    ret = esp_lcd_touch_read_data(s_gt9xxx_touch_handle);
    if (ret != ESP_OK) {
        return res;  /* 读取失败，返回无触摸 */
    }

    /* 获取触摸点数量和坐标 */
    // ret = esp_lcd_touch_get_data(s_gt9xxx_touch_handle, touch_data, &point_num, CONFIG_ESP_LCD_TOUCH_MAX_POINTS);
    // if (ret != ESP_OK) {
    //     ESP_LOGE(gt9xxx_tag, "Failed to get touch data: %s", esp_err_to_name(ret)); 
    //     return res;  /* 获取数据失败，返回无触摸 */
    // }
    pressed = esp_lcd_touch_get_coordinates(s_gt9xxx_touch_handle, tp_dev.x, tp_dev.y, NULL, &point_num, CONFIG_ESP_LCD_TOUCH_MAX_POINTS);
    if (pressed && point_num > 0) {
        /* 处理触摸点数据 */
        uint8_t valid_points = (point_num > CONFIG_ESP_LCD_TOUCH_MAX_POINTS) ? CONFIG_ESP_LCD_TOUCH_MAX_POINTS : point_num;
        
        /* 保存之前的状态 */
        tempsta = tp_dev.sta;
        
        /* 更新触摸状态 - 根据触摸点数设置状态位 */
        uint16_t touch_mask = 0xFFFF;
        if (valid_points < 16) {
            touch_mask = 0xFFFF << valid_points;
        }
        tp_dev.sta = (~touch_mask) | TP_PRES_DOWN | TP_CATH_PRES;
        
        /* 保存最后一个触摸点的数据 */
        tp_dev.x[CONFIG_ESP_LCD_TOUCH_MAX_POINTS - 1] = tp_dev.x[0];
        tp_dev.y[CONFIG_ESP_LCD_TOUCH_MAX_POINTS - 1] = tp_dev.y[0];

        /* 处理每个触摸点 */
        // for (uint8_t i = 0; i < valid_points; i++) {
        //     if (tp_dev.sta & (1 << i)) {
        //         // uint16_t raw_x = touch_data[i].x;
        //         // uint16_t raw_y = touch_data[i].y;
        //         // tp_dev.x[i] = raw_x;
        //         // tp_dev.y[i] = raw_y;
        //         // uint16_t strength = touch_data[i].strength;
        //         // uint8_t track_id = touch_data[i].track_id;
        //         /* LVGL 补偿 */
        //         // lv_display_rotation_t rotation = lv_display_get_rotation(lv_disp_get_default());
        //         // switch(rotation) {
        //         //     case LV_DISPLAY_ROTATION_0:
        //         //         // 不需要转换 也得有啊
        //         //         tp_dev.x[i] = raw_x;
        //         //         tp_dev.y[i] = raw_y;
        //         //         break;
        //         //     case LV_DISPLAY_ROTATION_90:
        //         //         tp_dev.x[i] = lcddev.width - raw_x;
        //         //         tp_dev.y[i] = lcddev.height - raw_y;
        //         //         break;
        //         //     case LV_DISPLAY_ROTATION_180:
        //         //         tp_dev.x[i] = raw_x;
        //         //         tp_dev.y[i] = raw_y;
        //         //         break;
        //         //     case LV_DISPLAY_ROTATION_270:
        //         //         tp_dev.x[i] = lcddev.width - raw_x;
        //         //         tp_dev.y[i] = lcddev.height - raw_y;
        //         //         break;
        //         // }
        //     }
        //     /* 记录触摸点坐标 方向补偿 */
        //     ESP_LOGI("GT9XXX", "x[%d]:%d,y[%d]:%d", i, tp_dev.x[i], i, tp_dev.y[i]);
        // }

        res = 1;

        /* 检查第一个触摸点的坐标是否合法 */
        if (tp_dev.x[0] > MY_DISP_HOR_RES || tp_dev.y[0] > MY_DISP_VER_RES) {
            if (point_num > 1) {
                /* 有其他触摸点，使用第二个触摸点的数据 */
                tp_dev.x[0] = tp_dev.x[1];
                tp_dev.y[0] = tp_dev.y[1];
                t = 0;  /* 触发连续监测 */
            } else {
                /* 非法数据，恢复之前的状态 */
                tp_dev.x[0] = tp_dev.x[CONFIG_ESP_LCD_TOUCH_MAX_POINTS - 1];
                tp_dev.y[0] = tp_dev.y[CONFIG_ESP_LCD_TOUCH_MAX_POINTS - 1];
                tp_dev.sta = tempsta;
                res = 0;  /* 标记为无触摸 */
            }
        } else {
            t = 0;  /* 触发连续监测 */
        }
    } else {
        /* 无触摸点按下 */
        if (tp_dev.sta & TP_PRES_DOWN) {
            /* 之前是被按下的，现在松开 */
            tp_dev.sta &= ~TP_PRES_DOWN;
        } else {
            /* 之前就没有被按下，清除所有触摸点标记 */
            tp_dev.x[0] = 0xffff;
            tp_dev.y[0] = 0xffff;
            tp_dev.sta &= 0XE000;  /* 清除触摸点有效标记 */
        }
        res = 0;
    }

    if (t > 240) {
        t = 10;  /* 重新从10开始计数 */
    }

    return res;
}

// components/BSP/LCD/lcd.c
#include "lcd.h"

static const char *TAG = "lcd";
static esp_lcd_panel_io_handle_t s_lcd_io_handle;  /* LCD IO句柄 */
// static esp_lcd_panel_handle_t s_lcd_panel_handle;  /* LCD面板句柄 */
static esp_lcd_i80_bus_handle_t s_i80_bus = NULL;

_lcd_dev_t lcddev;

/**
  * @brief lcd init
  * @param None
  * @retval None
  */
void lcd_init(void)
{
    lcddev = (_lcd_dev_t){
        .panel_handle = NULL,
        .width = MY_DISP_HOR_RES,
        .height = MY_DISP_VER_RES,
        .dir = LCD_DISPLAY_DIR,
        .id = LCD_DEVICE_ID,
        .ctrl = {
            .lcd_rst = LCD_RST_GPIO_PIN,
            .lcd_bl = -1,  /* 无背光控制 */
        },
    };
    /* 8080 bus config */
    gpio_config_t rd_cfg = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << LCD_RD_GPIO_PIN,
    };
    ESP_ERROR_CHECK(gpio_config(&rd_cfg));
    ESP_ERROR_CHECK(gpio_set_level(LCD_RD_GPIO_PIN, 1));

    esp_lcd_i80_bus_config_t bus_config = {
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .dc_gpio_num = LCD_DC_GPIO_PIN,
        .wr_gpio_num = LCD_WR_GPIO_PIN,
        .data_gpio_nums = {
            LCD_D0_GPIO, LCD_D1_GPIO, LCD_D2_GPIO, LCD_D3_GPIO,
            LCD_D4_GPIO, LCD_D5_GPIO, LCD_D6_GPIO, LCD_D7_GPIO,
            LCD_D8_GPIO, LCD_D9_GPIO, LCD_D10_GPIO, LCD_D11_GPIO,
            LCD_D12_GPIO, LCD_D13_GPIO, LCD_D14_GPIO, LCD_D15_GPIO,
        },
        .bus_width = LCD_BUS_WIDTH,
        .max_transfer_bytes = MY_DISP_VER_RES * MY_DISP_HOR_RES * 2,
    };

    ESP_ERROR_CHECK(esp_lcd_new_i80_bus(&bus_config, &s_i80_bus));

    esp_lcd_panel_io_i80_config_t io_config = {
        .cs_gpio_num = LCD_CS_GPIO_PIN,
        .pclk_hz = LCD_PIXEL_CLOCK_HZ,
        .trans_queue_depth = 10,
        .dc_levels = {
            .dc_idle_level = 0,
            .dc_cmd_level = 0,
            .dc_dummy_level = 0,
            .dc_data_level = 1,
        },
        .lcd_cmd_bits = 16,
        .lcd_param_bits = 16,
    };
    /* LCD device to 8080 bus */
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i80(s_i80_bus, &io_config, &s_lcd_io_handle));

    /* LCD device config */
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = LCD_RST_GPIO_PIN,
        .rgb_ele_order = COLOR_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
    };
    /* create LCD panel handler for NT35510, specify 8080 io handler */
    ESP_ERROR_CHECK(esp_lcd_new_panel_nt35510(s_lcd_io_handle, &panel_config, &lcddev.panel_handle));
    /* reset LCD */
    ESP_ERROR_CHECK(esp_lcd_panel_reset(lcddev.panel_handle));
    /* initialize LCD */
    ESP_ERROR_CHECK(esp_lcd_panel_init(lcddev.panel_handle));
    /* reverse display and turn on */
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(lcddev.panel_handle, true));

    ESP_LOGI(TAG, "lcd init done");
}

/**
  * @brief set LCD display direction
  * @param dir: 0: vertical screen; 1: horizontal screen
  * @retval None
  */
void lcd_set_dir(uint8_t dir)
{
    lcddev.dir = dir;
    if (lcddev.dir == 0) {
        lcddev.width = MY_DISP_HOR_RES;
        lcddev.height = MY_DISP_VER_RES;
        esp_lcd_panel_swap_xy(lcddev.panel_handle, false);
        esp_lcd_panel_mirror(lcddev.panel_handle, false, false);
    } else if (lcddev.dir == 1) {
        lcddev.width = MY_DISP_VER_RES;
        lcddev.height = MY_DISP_HOR_RES;
        esp_lcd_panel_swap_xy(lcddev.panel_handle, true);
        esp_lcd_panel_mirror(lcddev.panel_handle, true, false);
    }
}

/**
  * @brief lcd clear screen
  * @param color : color to fill the screen with
  * @retval None
  */
void lcd_clear(uint16_t color)
{
    uint16_t line[lcddev.width];
    for (int i = 0; i < lcddev.width; i++) {
        line[i] = color;
    }
    for (int y = 0; y < lcddev.height; y++) {
        esp_lcd_panel_draw_bitmap(lcddev.panel_handle, 0, y, lcddev.width, y + 1, line);
    }
}

/**
 * @brief       画点函数
 * @param       x,y   :写入坐标
 * @param       color :颜色值
 * @retval      无
 */
void _lcd_draw_point(uint16_t x, uint16_t y, uint16_t color)
{
    esp_lcd_panel_draw_bitmap(lcddev.panel_handle, x, y, x + 1, y + 1, (uint16_t *)&color);
}

/**
 * @brief       在指定位置显示一个字符
 * @param       x,y  :坐标
 * @param       chr  :要显示的字符:" "--->"~"
 * @param       size :字体大小 12/16/24/32
 * @param       mode :叠加方式(1); 非叠加方式(0);
 * @param       color:字体颜色
 * @retval      无
 */
void _lcd_show_char(uint16_t x, uint16_t y, char chr, uint8_t size, uint8_t mode, uint16_t color)
{
    uint8_t temp, t1, t;
    uint16_t y0 = y;
    uint8_t csize = 0;
    uint8_t *pfont = 0;

    csize = (size / 8 + ((size % 8) ? 1 : 0)) * (size / 2); /* 得到字体一个字符对应点阵集所占的字节数 */
    chr = (char)chr - ' ';      /* 得到偏移后的值（ASCII字库是从空格开始取模，所以-' '就是对应字符的字库） */

    switch (size)
    {
        // case 12:
        //     pfont = (uint8_t *)asc2_1206[(uint8_t)chr];     /* 调用1206字体 */
        //     break;
        // case 16:
        //     pfont = (uint8_t *)asc2_1608[(uint8_t)chr];     /* 调用1608字体 */
        //     break;
        // case 24:
        //     pfont = (uint8_t *)asc2_2412[(uint8_t)chr];     /* 调用2412字体 */
        //     break;
        case 32:
            pfont = (uint8_t *)asc2_3216[(uint8_t)chr];     /* 调用3216字体 */
            break;
        default:
            return ;
    }

    for (t = 0; t < csize; t++)
    {
        temp = pfont[t];                                    /* 获取字符的点阵数据 */
        for (t1 = 0; t1 < 8; t1++){                          /* 一个字节8个点 */
            if (temp & 0x80){                                /* 有效点,需要显示 */
                _lcd_draw_point(x, y, color);                /* 画点出来,要显示这个点 */
            }
            else if (mode == 0){                             /* 无效点,不显示 */
                _lcd_draw_point(x, y, g_back_color);         /* 画背景色,相当于这个点不显示(注意背景色由全局变量控制) */
            }
            temp <<= 1;                                     /* 移位, 以便获取下一个位的状态 */
            y++;
            if (y >= lcddev.height) return;                 /* 超区域了 */
            if ((y - y0) == size){                           /* 显示完一列了? */
                y = y0;                                     /* y坐标复位 */
                x++;                                        /* x坐标递增 */
                if (x >= lcddev.width){
                    return;                                 /* x坐标超区域了 */
                }
                break;
            }
        }
    }
}

/**
 * @brief       显示字符串
 * @param       x: 起始点横坐标
 * @param       y: 起始点纵坐标
 * @param       width: 区域大小
 * @param       height: 区域大小
 * @param       size        :选择字体 12/16/24/32
 * @param       p           :字符串首地址
 * @param       color       :字体颜色
 * @retval      无
 */
void lcd_show_string(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t size, char *p, uint16_t color)
{
    uint8_t x0 = x;
    width += x;
    height += y;
    while ((*p <= '~') && (*p >= ' '))   /* 判断是不是非法字符! */
    {
        if (x >= width)
        {
            x = x0;
            y += size;
        }
        if (y >= height)
        {
            break;                       /* 退出 */
        }
        _lcd_show_char(x, y, *p, size, 0, color);
        x += size / 2;
        p++;
    }
}

void lcd_sync_rotation(lv_display_rotation_t rotation)
{
    switch(rotation) {
        case LV_DISPLAY_ROTATION_0:   // 竖屏正向
            lcddev.dir = 0;
            lcddev.width = MY_DISP_HOR_RES;
            lcddev.height = MY_DISP_VER_RES;
            /* 显示区域 */
            esp_lcd_panel_swap_xy(lcddev.panel_handle, false);
            esp_lcd_panel_mirror(lcddev.panel_handle, false, false);
            /* 触摸区域 */
            // esp_lcd_touch_set_swap_xy(s_gt9xxx_touch_handle, false);
            // esp_lcd_touch_set_mirror_x(s_gt9xxx_touch_handle, false);
            // esp_lcd_touch_set_mirror_y(s_gt9xxx_touch_handle, false);
            break;
            
        case LV_DISPLAY_ROTATION_90:  // 横屏 顺时针90度
            lcddev.dir = 1;
            lcddev.width = MY_DISP_VER_RES;
            lcddev.height = MY_DISP_HOR_RES;
            /* 显示区域 */
            esp_lcd_panel_swap_xy(lcddev.panel_handle, true);
            esp_lcd_panel_mirror(lcddev.panel_handle, false, true);
            /* 触摸区域 */
            // esp_lcd_touch_set_swap_xy(s_gt9xxx_touch_handle, true);
            // esp_lcd_touch_set_mirror_x(s_gt9xxx_touch_handle, false);
            // esp_lcd_touch_set_mirror_y(s_gt9xxx_touch_handle, true);
            break;
            
        case LV_DISPLAY_ROTATION_180: // 竖屏倒置
            lcddev.dir = 0;
            lcddev.width = MY_DISP_HOR_RES;
            lcddev.height = MY_DISP_VER_RES;
            /* 显示区域 */
            esp_lcd_panel_swap_xy(lcddev.panel_handle, false);
            esp_lcd_panel_mirror(lcddev.panel_handle, true, true);
            /* 触摸区域 */
            // esp_lcd_touch_set_swap_xy(s_gt9xxx_touch_handle, false);
            // esp_lcd_touch_set_mirror_x(s_gt9xxx_touch_handle, true);
            // esp_lcd_touch_set_mirror_y(s_gt9xxx_touch_handle, true);
            break;
            
        case LV_DISPLAY_ROTATION_270: // 横屏 逆时针90度
            lcddev.dir = 1;
            lcddev.width = MY_DISP_VER_RES;
            lcddev.height = MY_DISP_HOR_RES;
            /* 显示区域 */
            esp_lcd_panel_swap_xy(lcddev.panel_handle, true);
            esp_lcd_panel_mirror(lcddev.panel_handle, true, false);
            /* 触摸区域 */
            // esp_lcd_touch_set_swap_xy(s_gt9xxx_touch_handle, true);
            // esp_lcd_touch_set_mirror_x(s_gt9xxx_touch_handle, false);
            // esp_lcd_touch_set_mirror_y(s_gt9xxx_touch_handle, true);
            break;
    }
    ESP_LOGI("lcd", "LCD synced to rotation: %d, dir: %d, size: %dx%d", 
             rotation, lcddev.dir, lcddev.width, lcddev.height);
}

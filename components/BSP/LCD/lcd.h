#ifndef _COMPONENTS_BSP_LCD_LCD_H
#define _COMPONENTS_BSP_LCD_LCD_H

#include <math.h>
#include <string.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_nt35510.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"
#include "driver/gpio.h"
#include "lcd_font.h"
#include "xl9555.h"
#include "touch.h"
#include "lvgl__lvgl/lvgl.h"


/* NT35510引脚定义 */
#define LCD_CS_GPIO_PIN   GPIO_NUM_4
#define LCD_DC_GPIO_PIN   GPIO_NUM_5
#define LCD_WR_GPIO_PIN   GPIO_NUM_6
#define LCD_RD_GPIO_PIN   GPIO_NUM_7
#define LCD_RST_GPIO_PIN  GPIO_NUM_15

typedef struct{
    esp_lcd_panel_handle_t panel_handle;  /* LCD面板句柄 */
    uint16_t width;      /* LCD宽度 */
    uint16_t height;     /* LCD高度 */
    uint8_t dir;         /* 显示方向:0,竖屏;1,横屏 */
    uint16_t id;         /* LCD ID */
    struct {
        gpio_num_t lcd_rst;  /* LCD复位引脚 */
        gpio_num_t lcd_bl;   /* LCD背光引脚 */
    } ctrl;
}_lcd_dev_t;
extern _lcd_dev_t lcddev;  /* LCD参数结构体 */

#define g_back_color WHITE  /* 背景色 */
#define LCD_DEVICE_ID 0
#define MY_DISP_HOR_RES 480
#define MY_DISP_VER_RES 800
#define LCD_PIXEL_CLOCK_HZ 8000000
#define LCD_DISPLAY_DIR 0  /* 0:竖屏,1:横屏 */

#define LCD_BUS_WIDTH 16

#if LCD_BUS_WIDTH == 8
    #define LCD_D0_GPIO   GPIO_NUM_16
    #define LCD_D1_GPIO   GPIO_NUM_17
    #define LCD_D2_GPIO   GPIO_NUM_18
    #define LCD_D3_GPIO   GPIO_NUM_3
    #define LCD_D4_GPIO   GPIO_NUM_46
    #define LCD_D5_GPIO   GPIO_NUM_9
    #define LCD_D6_GPIO   GPIO_NUM_10
    #define LCD_D7_GPIO   GPIO_NUM_14
    
#elif LCD_BUS_WIDTH == 16
    #define LCD_D0_GPIO   GPIO_NUM_16
    #define LCD_D1_GPIO   GPIO_NUM_17
    #define LCD_D2_GPIO   GPIO_NUM_18
    #define LCD_D3_GPIO   GPIO_NUM_3
    #define LCD_D4_GPIO   GPIO_NUM_46
    #define LCD_D5_GPIO   GPIO_NUM_9
    #define LCD_D6_GPIO   GPIO_NUM_10
    #define LCD_D7_GPIO   GPIO_NUM_14
    #define LCD_D8_GPIO   GPIO_NUM_47
    #define LCD_D9_GPIO   GPIO_NUM_48
    #define LCD_D10_GPIO  GPIO_NUM_45
    #define LCD_D11_GPIO  GPIO_NUM_39
    #define LCD_D12_GPIO  GPIO_NUM_38
    #define LCD_D13_GPIO  GPIO_NUM_35
    #define LCD_D14_GPIO  GPIO_NUM_36
    #define LCD_D15_GPIO  GPIO_NUM_37


#else
    #error "LCD_BUS_WIDTH must be 8 or 16"
#endif // LCD_BUS_WIDTH == 16

/* 16bit常用颜色值 */
#define WHITE           0xFFFF      /* 白色 */
#define BLACK           0x0000      /* 黑色 */
#define RED             0xF800      /* 红色 */
#define GREEN           0x07E0      /* 绿色 */
#define BLUE            0x001F      /* 蓝色 */ 
#define MAGENTA         0XF81F      /* 洋红色 */
#define YELLOW          0XFFE0      /* 黄色 */
#define CYAN            0X07FF      /* 蓝绿色 */

/* 16bit非常用颜色 */
#define BROWN           0XBC40      /* 棕色 */
#define BRRED           0XFC07      /* 棕红色 */
#define GRAY            0X8430      /* 灰色 */ 
#define DARKBLUE        0X01CF      /* 深蓝色 */
#define LIGHTBLUE       0X7D7C      /* 浅蓝色 */ 
#define GRAYBLUE        0X5458      /* 灰蓝色 */ 
#define LIGHTGREEN      0X841F      /* 浅绿色 */  
#define LGRAY           0XC618      /* 浅灰色(PANNEL),窗体背景色 */ 
#define LGRAYBLUE       0XA651      /* 浅灰蓝色(中间层颜色) */ 
#define LBBLUE          0X2B12      /* 浅棕蓝色(选择条目的反色) */ 

#ifdef __cplusplus
extern "C" {
#endif

void lcd_init(void);
void lcd_set_dir(uint8_t dir);
void lcd_clear(uint16_t color);
void lcd_show_string(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t size, char *p, uint16_t color);
void lcd_sync_rotation(lv_display_rotation_t rotation);

#ifdef __cplusplus
}
#endif



#endif // _COMPONENTS_BSP_LCD_LCD_H

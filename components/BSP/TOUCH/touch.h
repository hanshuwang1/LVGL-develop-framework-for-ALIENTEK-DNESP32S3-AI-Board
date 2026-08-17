/**
 ****************************************************************************************************
 * @file        touch.h
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2025-01-01
 * @brief       触摸屏 驱动代码
 * @note        支持电容式触摸屏
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

#ifndef __TOUCH_H
#define __TOUCH_H

#include "lcd.h"
#include "gt9xxx.h"
#include "myiic.h"

extern uint32_t POINT_COLOR_TBL[5];  /* 触控点的颜色 */

#define TP_PRES_DOWN    0x8000  /* 触屏被按下 */
#define TP_CATH_PRES    0x4000  /* 有按键按下了 */
#define CT_MAX_TOUCH    10      /* 电容屏支持的点数,固定为5点 */

/* 触摸屏控制器 */
typedef struct
{
    esp_err_t (*init)(void);    /* 初始化触摸屏控制器 */
    uint8_t (*scan)(void);          /* 扫描触摸屏 */
    uint16_t x[CT_MAX_TOUCH];   /* 当前坐标 */
    uint16_t y[CT_MAX_TOUCH];   /* 电容屏有最多10组坐标,电阻屏则用x[0],y[0]代表:此次扫描时,触屏的坐标,用
                                 * x[9],y[9]存储第一次按下时的坐标.
                                 */

    uint16_t sta;               /* 笔的状态
                                 * b15:按下1/松开0;
                                 * b14:0,没有按键按下;1,有按键按下.
                                 * b13~b10:保留
                                 * b9~b0:电容触摸屏按下的点数(0,表示未按下,1表示按下)
                                 */
    uint8_t touchtype;
} _m_tp_dev;

extern _m_tp_dev tp_dev;    /* 触屏控制器在touch.c里面定义 */

/* 函数声明 */
esp_err_t tp_init(void);      /* 触摸屏初始化 */


#endif

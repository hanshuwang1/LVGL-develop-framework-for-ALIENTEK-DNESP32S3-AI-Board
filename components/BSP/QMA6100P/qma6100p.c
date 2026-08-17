/**
 ****************************************************************************************************
* @file        qma6100p.c
* @author      正点原子团队(ALIENTEK)
* @version     V1.0
* @date        2025-01-01
* @brief       QMA6100P驱动代码
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

#include "qma6100p.h"


const char *qma6100p_tag = "qma6100p";
i2c_master_dev_handle_t qma6100p_handle = NULL;

#define QMA6100P_INT                gpio_get_level(qma6100p_self.qma_int)   /* 读取QMA6100P_INT的电平 */
#define I2C_MASTER_TIMEOUT_MS       1000
#define M_G                         9.80665f
#define RAD_TO_DEG                  (180.0f / M_PI)                         /* 0.017453292519943295 */

/**
 * @brief       读取qma6100p寄存器的数据
 * @param       reg_addr       : 要读取的寄存器地址
 * @param       data           : 读取的数据
 * @param       len           : 数据大小
 * @retval      错误值        ：0成功，其他值：错误
 */
esp_err_t qma6100p_register_read(const uint8_t reg, uint8_t *data, const size_t len)
{
    return i2c_master_transmit_receive(qma6100p_handle, &reg, 1, data, len, -1);
}


/**
 * @brief       向qma6100p寄存器写数据
 * @param       reg_addr       : 要写入的寄存器地址
 * @param       data           : 要写入的数据
 * @retval      错误值        ：0成功，其他值：错误
 */
static esp_err_t qma6100p_register_write_byte(uint8_t reg, uint8_t data)
{
    esp_err_t ret;

    uint8_t *buf = malloc(2);
    if (buf == NULL)
    {
        ESP_LOGE(qma6100p_tag, "%s memory failed", __func__);
        return ESP_ERR_NO_MEM;
    }

    buf[0] = reg;
    buf[1] = data;

    ret = i2c_master_transmit(qma6100p_handle, buf, 2, -1);

    free(buf);

    return ret;
}

uint8_t xyz_data[6] = {0};
short raw_data[3] = {0};
float accl_data[3];
float acc_normal;

/**
 * @brief       读取三轴数据(原始数据、加速度、俯仰角和翻滚角)
 * @param       rawdata：qma6100p数据结构体
 * @retval      无
 */
void qma6100p_read_rawdata(qma6100p_rawdata_t *rawdata)
{
    qma6100p_register_read(QMA6100P_REG_XOUTL, xyz_data, 6);
    
    raw_data[0] = (short)(((xyz_data[1] << 8)) | (xyz_data[0]));
    raw_data[1] = (short)(((xyz_data[3] << 8)) | (xyz_data[2]));
    raw_data[2] = (short)(((xyz_data[5] << 8)) | (xyz_data[4]));
    
    rawdata->acc_x = (float)((raw_data[0] >> 2) * M_G) / (1024);
    rawdata->acc_y = (float)((raw_data[1] >> 2) * M_G) / (1024);
    rawdata->acc_z = (float)((raw_data[2] >> 2) * M_G) / (1024);
    
    rawdata->acc_g = sqrt(rawdata->acc_x*rawdata->acc_x + rawdata->acc_y * rawdata->acc_y + rawdata->acc_z*rawdata->acc_z);
    
    acc_normal = sqrtf(rawdata->acc_x * rawdata->acc_x + rawdata->acc_y * rawdata->acc_y + rawdata->acc_z * rawdata->acc_z);
    accl_data[0] = rawdata->acc_x / acc_normal;
    accl_data[1] = rawdata->acc_y / acc_normal;
    accl_data[2] = rawdata->acc_z / acc_normal;

    rawdata->pitch = -atan2f(rawdata->acc_x, rawdata->acc_z) * RAD_TO_DEG;
    
    acc_normal = sqrtf(accl_data[0] * accl_data[0] + accl_data[1] * accl_data[1] + accl_data[2] * accl_data[2]);
    
    rawdata->roll = asinf((accl_data[1]/acc_normal)) * RAD_TO_DEG ;
}

/**
 * @brief       配置中断函数
 * @param       int_map：中断模式
 * @param       enable：使能
 * @retval      mp_const_none：初始化成功
 */
void qma6100p_step_int_config(int int_map, int enable)
{
    uint8_t    reg_16 = 0;
    uint8_t    reg_19 = 0;
    uint8_t    reg_1b = 0;

    qma6100p_register_read(0x16, &reg_16, 1);
    qma6100p_register_read(0x19, &reg_19, 1);
    qma6100p_register_read(0x1b, &reg_1b, 1);
    
    if (enable)
    {
        reg_16 |= 0x08;
        reg_19 |= 0x08;
        reg_1b |= 0x08;
        qma6100p_register_write_byte(0x16, reg_16);
        
        if(int_map == QMA6100P_MAP_INT1)
        {
            qma6100p_register_write_byte(0x19, reg_19);
        }
        else if(int_map == QMA6100P_MAP_INT2)
        {
            qma6100p_register_write_byte(0x1b, reg_1b);
        }
    }
    else
    {
        reg_16 &= (~0x08);
        reg_19 &= (~0x08);
        reg_1b &= (~0x08);

        qma6100p_register_write_byte(0x16, reg_16);
        qma6100p_register_write_byte(0x19, reg_19);
        qma6100p_register_write_byte(0x1b, reg_1b);
    }
}

/**
 * @brief       初始化qma6100p
 * @param       无
 * @retval      0, 成功;
                1, 失败;
*/
uint8_t qma6100p_comfig(void)
{
    static uint8_t id_data[2];
    qma6100p_register_read(QMA6100P_REG_CHIP_ID, id_data, 1);    /* 读取设备ID，正常是0x90 */
    
    /* qma6100p的初始化序列，请看手册“6.3 Initial sequence”章节 */
    qma6100p_register_write_byte(QMA6100P_REG_RESET, QMA6100P_RESET);
    vTaskDelay(5);
    qma6100p_register_write_byte(QMA6100P_REG_RESET, QMA6100P_RESET_END);
    vTaskDelay(10);
    
    qma6100p_register_read(QMA6100P_REG_CHIP_ID, id_data, 1);    /* 读取设备ID，正常是0x90 */
    
    qma6100p_register_write_byte(0x11, 0x80);
    qma6100p_register_write_byte(0x11, 0x84);
    qma6100p_register_write_byte(0x4a, 0x20);
    qma6100p_register_write_byte(0x56, 0x01);
    qma6100p_register_write_byte(0x5f, 0x80);
    vTaskDelay(1);
    qma6100p_register_write_byte(0x5f, 0x00);
    vTaskDelay(10);

    qma6100p_register_write_byte(QMA6100P_REG_RANGE, QMA6100P_RANGE_8G);
    qma6100p_register_write_byte(QMA6100P_REG_BW_ODR, QMA6100P_BW_100);
    qma6100p_register_write_byte(QMA6100P_REG_POWER_MANAGE, QMA6100P_MCLK_51_2K | 0x80);

    qma6100p_register_write_byte(0x21, 0x03);    /* default 0x1c, step latch mode */
    
    qma6100p_step_int_config(QMA6100P_MAP_INT1, 1);
    
    if (id_data[0] == 0x90)
    {
        ESP_LOGI(qma6100p_tag, "qma6100p success!!!");
        return 0;                                  /* qma6100p正常 */
    }
    else
    {
        ESP_LOGE(qma6100p_tag, "qma6100p fail!!!");
        return 1;                               /* qma6100p失败 */
    }
} 

/**
 * @brief       qma6100p初始化
 * @param       无
 * @retval      无
 */
esp_err_t qma6100p_init(void)
{
    /* 未调用myiic_init初始化IIC */
    if (bus_handle == NULL)
    {
        ESP_ERROR_CHECK(myiic_init());
    }

    i2c_device_config_t qmi8658_i2c_dev_conf = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,  /* 从机地址长度 */
        .scl_speed_hz    = IIC_SPEED_CLK,       /* 传输速率 */
        .device_address  = QMA6100P_ADDR,        /* 从机7位的地址 */
    };
    /* I2C总线上添加qmi8658设备 */
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &qmi8658_i2c_dev_conf, &qma6100p_handle));

    
    while (qma6100p_comfig())   /* 检测不到qma6100p */
    {
        ESP_LOGE(qma6100p_tag, "qma6100p init fail!!!");
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    return 0;
}

static qma6100p_rawdata_t last_filtered_value = {0};
/**
  * @brief 一阶低通滤波
  * @param qma6100p_rawdata_t *rawdata: qma6100p数据结构体
  * @param float alpha: 滤波系数, 范围0~1, 值越小,滤波越平滑;值越大, 响应越快
  * @param qma6100p_rawdata_t *filtered_data: 滤波后的数据结构体
  * @retval None
  */
void qma6100p_low_pass_filter(qma6100p_rawdata_t *rawdata, float alpha, qma6100p_rawdata_t *filtered_data)
{
    filtered_data->pitch = alpha * rawdata->pitch + (1 - alpha) * last_filtered_value.pitch;
    filtered_data->roll  = alpha * rawdata->roll  + (1 - alpha) * last_filtered_value.roll;
    last_filtered_value = *filtered_data;
}

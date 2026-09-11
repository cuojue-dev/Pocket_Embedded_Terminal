/*
 * mpu6500.c
 *
 *  Created on: Sep 7, 2026
 *      Author: cuojue
 */


#include "mpu6500.h"
#include "i2c.h"



#define MPU6500_ADDR (0x68 << 1)
#define MPU6500_WHO_AM_I 0x75
#define MPU6500_PWR_MGMT_1 0x6B
#define MPU6500_ACCEL_XOUT_H 0x3B

// HAL I2C 的 7-bit 从机地址参数需左移一位，MPU6500 原始地址为 0x68
static uint8_t s_pwr = 1;


HAL_StatusTypeDef MPU6500_Read_Who_Am_I(uint8_t *who_am_i)
{
    if(who_am_i == NULL)
    {
        return HAL_ERROR;
    }

    return HAL_I2C_Mem_Read(&hi2c1, MPU6500_ADDR, MPU6500_WHO_AM_I,
        I2C_MEMADD_SIZE_8BIT, who_am_i, 1, 100);
}

HAL_StatusTypeDef MPU6500_Init(void)
{
	// PWR_MGMT_1 写 1，使用 PLL 时钟源并退出默认睡眠状态
	return HAL_I2C_Mem_Write(&hi2c1, MPU6500_ADDR, MPU6500_PWR_MGMT_1,
							 I2C_MEMADD_SIZE_8BIT, &s_pwr, 1, 100);
}

HAL_StatusTypeDef MPU6500_Read_Raw(int16_t *acc_x, int16_t *acc_y, int16_t *acc_z,
								   int16_t *gyro_x, int16_t *gyro_y, int16_t *gyro_z)
{
	HAL_StatusTypeDef result;
	uint8_t register_buffer[14];

	// 从 ACCEL_XOUT_H 连续读取 14 Byte：加速度 6、温度 2、陀螺仪 6
	result = HAL_I2C_Mem_Read(&hi2c1, MPU6500_ADDR, MPU6500_ACCEL_XOUT_H,
							  I2C_MEMADD_SIZE_8BIT, register_buffer, sizeof(register_buffer), 100);

	if(result != HAL_OK)
	{
		return result;
	}

	*acc_x = (int16_t)(((uint16_t)register_buffer[0] << 8) | register_buffer[1]);
	*acc_y = (int16_t)(((uint16_t)register_buffer[2] << 8) | register_buffer[3]);
	*acc_z = (int16_t)(((uint16_t)register_buffer[4] << 8) | register_buffer[5]);

	*gyro_x = (int16_t)(((uint16_t)register_buffer[8] << 8) | register_buffer[9]);
	*gyro_y = (int16_t)(((uint16_t)register_buffer[10] << 8) | register_buffer[11]);
	*gyro_z = (int16_t)(((uint16_t)register_buffer[12] << 8) | register_buffer[13]);


	return result;
}

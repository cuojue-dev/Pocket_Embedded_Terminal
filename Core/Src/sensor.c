/*
 * sensor.c
 *
 *  Created on: Sep 7, 2026
 *      Author: cuojue
 */


#include "sensor.h"
#include "mpu6500.h"
#include <math.h>

#define RAD_TO_DEG 57.29578f
#define GYRO_CALIBRATION_SAMPLES 200
#define GYRO_SENSITIVITY 131.0f
#define SENSOR_SAMPLE_PERIOD_S  0.01f
#define COMPLEMENTARY_ALPHA     0.98f

// Sensor_Task 周期调用本模块；这里不依赖 FreeRTOS，只处理 MPU 原始数据和姿态估计
static sensor_state_t s_sensor_state;

static float s_gyro_bias_x;
static float s_gyro_bias_y;
static float s_gyro_bias_z;
static uint8_t s_filter_initialized;


HAL_StatusTypeDef Sensor_Init(void)
{
    HAL_StatusTypeDef result;
    int32_t sum_x = 0;
    int32_t sum_y = 0;
    int32_t sum_z = 0;

    int16_t acc_x;
    int16_t acc_y;
    int16_t acc_z;
    int16_t gyro_x;
    int16_t gyro_y;
    int16_t gyro_z;

    result = MPU6500_Init();

    if(result != HAL_OK)
    {
        return result;
    }

    HAL_Delay(100);

	// 上电静止时取平均值，作为陀螺仪零偏
	for(uint16_t i = 0;i < GYRO_CALIBRATION_SAMPLES;i++)
    {
    	result = MPU6500_Read_Raw(&acc_x, &acc_y, &acc_z, &gyro_x, &gyro_y, &gyro_z);

    	if(result != HAL_OK)
    	{
    		return result;
    	}

    	sum_x += gyro_x;
    	sum_y += gyro_y;
    	sum_z += gyro_z;

    	HAL_Delay(5);
    }

    s_gyro_bias_x = (float)sum_x / GYRO_CALIBRATION_SAMPLES;
    s_gyro_bias_y = (float)sum_y / GYRO_CALIBRATION_SAMPLES;
    s_gyro_bias_z = (float)sum_z / GYRO_CALIBRATION_SAMPLES;

    s_filter_initialized = 0;

    return HAL_OK;
}

HAL_StatusTypeDef Sensor_Update(void)
{
	HAL_StatusTypeDef result;
	float roll_acc_deg;
	float pitch_acc_deg;

	result = MPU6500_Read_Raw(&s_sensor_state.acc_x, &s_sensor_state.acc_y, &s_sensor_state.acc_z,
							  &s_sensor_state.gyro_x, &s_sensor_state.gyro_y, &s_sensor_state.gyro_z);

	if(result != HAL_OK)
	{
		s_sensor_state.error_count++;
		return result;
	}

	s_sensor_state.sample_count++;

	roll_acc_deg = atan2f((float)s_sensor_state.acc_y,
									 (float)s_sensor_state.acc_z) * RAD_TO_DEG;

	pitch_acc_deg = atan2f(-(float)s_sensor_state.acc_x,
									  sqrtf((float)s_sensor_state.acc_y * s_sensor_state.acc_y +
											(float)s_sensor_state.acc_z * s_sensor_state.acc_z)) * RAD_TO_DEG;

	s_sensor_state.gyro_x_dps = ((float)s_sensor_state.gyro_x - s_gyro_bias_x) / GYRO_SENSITIVITY;
	s_sensor_state.gyro_y_dps = ((float)s_sensor_state.gyro_y - s_gyro_bias_y) / GYRO_SENSITIVITY;
	s_sensor_state.gyro_z_dps = ((float)s_sensor_state.gyro_z - s_gyro_bias_z) / GYRO_SENSITIVITY;

	/* 首帧直接使用加速度角作为初值，避免陀螺积分从 0 度突然跳变 */
	if(!s_filter_initialized)
	{
		s_sensor_state.roll_deg = roll_acc_deg;
		s_sensor_state.pitch_deg = pitch_acc_deg;

		s_filter_initialized = 1;
	}
	else
	{
		// 互补滤波：陀螺积分负责短期变化，重力方向负责长期校正漂移
		s_sensor_state.roll_deg = COMPLEMENTARY_ALPHA *
								  (s_sensor_state.roll_deg +
								  s_sensor_state.gyro_x_dps *
								  SENSOR_SAMPLE_PERIOD_S) +
								  (1.0f - COMPLEMENTARY_ALPHA) *
								  roll_acc_deg;

		s_sensor_state.pitch_deg = COMPLEMENTARY_ALPHA *
								  (s_sensor_state.pitch_deg +
								  s_sensor_state.gyro_y_dps *
								  SENSOR_SAMPLE_PERIOD_S) +
								  (1.0f - COMPLEMENTARY_ALPHA) *
								  pitch_acc_deg;
	}

	return result;
}

void Sensor_Get_State(sensor_state_t *state)
{
	if(state == NULL)
	{
		return;
	}

	*state = s_sensor_state;
}

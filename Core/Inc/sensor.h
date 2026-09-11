/*
 * sensor.h
 *
 *  Created on: Sep 7, 2026
 *      Author: cuojue
 */

#ifndef INC_SENSOR_H_
#define INC_SENSOR_H_


#include "stm32f1xx_hal.h"


typedef struct
{
	int16_t acc_x;
	int16_t acc_y;
	int16_t acc_z;

	int16_t gyro_x;
	int16_t gyro_y;
	int16_t gyro_z;

	uint32_t sample_count;
	uint32_t error_count;

	float roll_deg;
	float pitch_deg;

	float gyro_x_dps;
	float gyro_y_dps;
	float gyro_z_dps;
}sensor_state_t;

typedef struct
{
    uint32_t timestamp_ms;

    int16_t acc_x;
    int16_t acc_y;
    int16_t acc_z;

    int16_t gyro_x;
    int16_t gyro_y;
    int16_t gyro_z;

    int16_t roll_x100;
    int16_t pitch_x100;
} motion_sample_t;

HAL_StatusTypeDef Sensor_Init(void);
HAL_StatusTypeDef Sensor_Update(void);
void Sensor_Get_State(sensor_state_t *state);

#endif /* INC_SENSOR_H_ */

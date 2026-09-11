/*
 * mpu6500.h
 *
 *  Created on: Sep 7, 2026
 *      Author: cuojue
 */

#ifndef INC_MPU6500_H_
#define INC_MPU6500_H_


#include "main.h"


HAL_StatusTypeDef MPU6500_Read_Who_Am_I(uint8_t *who_am_i);
HAL_StatusTypeDef MPU6500_Init(void);
HAL_StatusTypeDef MPU6500_Read_Raw(int16_t *acc_x, int16_t *acc_y, int16_t *acc_z,
								   int16_t *gyro_x, int16_t *gyro_y, int16_t *gyro_z);


#endif /* INC_MPU6500_H_ */

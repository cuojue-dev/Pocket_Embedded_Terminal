/*
 * key.c
 *
 *  Created on: Sep 5, 2026
 *      Author: cuojue
 */


#include "key.h"
#include "gpio.h"

// 开发板按键有效电平不完全相同：KEY_UP 为高有效，KEY0/KEY1 为低有效
ui_action_t Key_Read_Action(void)
{
	// 同时按下多个实体按键时按此顺序只产生一个 UI 动作
	if(HAL_GPIO_ReadPin(KEY_UP_GPIO_Port, KEY_UP_Pin) == GPIO_PIN_SET)
	{
		return UI_ACTION_UP;
	}
	else if (HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == GPIO_PIN_RESET)
	{
		return UI_ACTION_DOWN;
	}
	else if (HAL_GPIO_ReadPin(KEY0_GPIO_Port, KEY0_Pin) == GPIO_PIN_RESET)
	{
		return UI_ACTION_ENTER;
	}

	return UI_ACTION_NONE;
}

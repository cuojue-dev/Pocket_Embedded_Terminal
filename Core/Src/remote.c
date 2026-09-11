/*
 * remote.c
 *
 *  Created on: Sep 9, 2026
 *      Author: cuojue
 */


#include "remote.h"
#include "tim.h"

// 以下状态由 TIM4 输入捕获中断写入，任务上下文只通过 Pop 接收完整命令
static volatile uint16_t s_last_high_us;
static volatile uint8_t s_waiting_falling_edge;

static volatile uint8_t s_receiving_frame;
static volatile uint8_t s_bit_count;
static volatile uint32_t s_frame_data;
static volatile uint32_t s_last_frame_data;

static volatile uint8_t s_last_command;
static volatile uint8_t s_pending_command;

static void Remote_Process_High_Pulse(uint16_t high_us)
{
	uint8_t bit_value;
	uint8_t address;
	uint8_t address_inverse;
	uint8_t command;
	uint8_t command_inverse;

	// NEC 的 4.5 ms 高电平标志一帧开始，随后会收到 32 个数据高脉冲
	if(high_us >4200 && high_us <4700) // 4500us引导码
	{
		s_receiving_frame = 1;
		s_bit_count = 0;
		s_frame_data = 0;
		return;
	}

	if(high_us > 2000U && high_us < 2500U)
	{
	    if(s_receiving_frame == 0 && s_last_command != 0U)
	    {
	        s_pending_command = s_last_command;
	    }

	    return;
	}

	if(s_receiving_frame == 0)
	{
		return;
	}


	else if(high_us > 300 && high_us < 800) // 560us 左右表明 bit 是0
	{
		bit_value = 0;
	}
	else if(high_us > 1400 && high_us < 1800) // 1680us左右说明bit是1
	{
		bit_value = 1;
	}
	else
	{
		s_receiving_frame = 0;
		s_bit_count = 0;
		s_frame_data = 0;

		return;
	}

	if(bit_value)
	{
		// NEC 按低位在前传输，第 bit_count 个脉冲写入对应位
		s_frame_data |= (1UL << s_bit_count);
	}

	s_bit_count++;

	if(s_bit_count == 32)
	{
		s_last_frame_data = s_frame_data;
		s_receiving_frame = 0;

		address = (uint8_t)(s_frame_data);
		address_inverse = (uint8_t)(s_frame_data >> 8);
		command = (uint8_t)(s_frame_data >> 16);
		command_inverse = (uint8_t)(s_frame_data >> 24);

		if(address == (uint8_t)~address_inverse &&
		   command == (uint8_t)~command_inverse)
		{
			/* 这一帧可靠 */
			s_last_command = command;
			s_pending_command = command;
		}
	}
}

void Remote_Init(void)
{
	s_last_high_us = 0;
	s_waiting_falling_edge = 0;
	s_receiving_frame = 0;
	s_bit_count = 0;
	s_frame_data = 0;
	s_last_frame_data = 0;
	s_last_command = 0;
	s_pending_command = 0;

	HAL_TIM_IC_Start_IT(&htim4, TIM_CHANNEL_4);

	__HAL_TIM_ENABLE_IT(&htim4, TIM_IT_UPDATE);
}

uint16_t Remote_Get_Last_high_Us(void)
{
	return s_last_high_us;
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
	uint16_t capture_value;

	if(htim->Instance != TIM4 || htim->Channel != HAL_TIM_ACTIVE_CHANNEL_4)
	{
		return;
	}

	if(s_waiting_falling_edge == 0)
	{
		// 上升沿作为高电平起点，清零 CNT 后改捕获下降沿测量脉宽
		__HAL_TIM_SET_COUNTER(&htim4, 0);

		__HAL_TIM_SET_CAPTUREPOLARITY(&htim4, TIM_CHANNEL_4, TIM_INPUTCHANNELPOLARITY_FALLING);

		s_waiting_falling_edge = 1;
	}
	else
	{
		capture_value = HAL_TIM_ReadCapturedValue(&htim4, TIM_CHANNEL_4);

		s_last_high_us = capture_value;

		Remote_Process_High_Pulse(capture_value);

		__HAL_TIM_SET_CAPTUREPOLARITY(&htim4, TIM_CHANNEL_4, TIM_INPUTCHANNELPOLARITY_RISING);

		s_waiting_falling_edge = 0;
	}
}

void Remote_Handle_Timeout(void)
{
	// TIM4 更新中断说明一对边沿未在 10 ms 内完成，丢弃残缺 NEC 帧并恢复测上升沿
	if(s_waiting_falling_edge != 0 ||
	   s_receiving_frame != 0)
	{
		__HAL_TIM_SET_CAPTUREPOLARITY(&htim4, TIM_CHANNEL_4, TIM_INPUTCHANNELPOLARITY_RISING);

		s_waiting_falling_edge = 0;
		s_receiving_frame = 0;
		s_bit_count = 0;
		s_frame_data = 0;
	}
}

uint32_t Remote_Get_Last_Frame_Data(void)
{
	return s_last_frame_data;
}

uint8_t Remote_Get_Last_Command(void)
{
	return s_last_command;
}

uint8_t Remote_Pop_Command(void)
{
	uint8_t command;
	uint32_t primask;

	// 读出并清零必须原子完成，避免 ISR 恰好写入新命令而被覆盖
	primask = __get_PRIMASK();
	__disable_irq();

	command = s_pending_command;
	s_pending_command = 0;

	if(primask == 0)
	{
		__enable_irq();
	}

	return command;
}

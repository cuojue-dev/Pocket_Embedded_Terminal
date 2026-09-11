/*
 * stopwatch.c
 *
 *  Created on: Sep 5, 2026
 *      Author: cuojue
 */


#include "stopwatch.h"
#include "cmsis_os.h"

// 暂停前的累计时间与当前运行段起点分开保存，避免暂停期间继续计时
static uint32_t s_elapsed_ms;
static uint32_t s_start_tick;
static uint8_t s_running;


void Stopwatch_Init(void)
{
	s_elapsed_ms = 0;
	s_start_tick = 0;
	s_running = 0;
}

void Stopwatch_Start(void)
{
	if(!s_running)
	{
		s_start_tick = osKernelGetTickCount();
		s_running = 1;
	}
}

void Stopwatch_Pause(void)
{
	if(s_running)
	{
		// 暂停时把当前运行段结算到累计值，之后读取不再依赖 s_start_tick
		s_elapsed_ms += osKernelGetTickCount() - s_start_tick;
		s_running = 0;
	}
}

void Stopwatch_Reset(void)
{
	s_elapsed_ms = 0;

	if(s_running)
	{
		s_start_tick = osKernelGetTickCount();
	}
}

uint32_t Stopwatch_Get_Elapsed_Ms(void)
{
	if(s_running)
	{
		return s_elapsed_ms + osKernelGetTickCount() - s_start_tick;
	}

	return s_elapsed_ms;
}

uint8_t Stopwatch_Is_Running(void)
{
	return s_running;
}

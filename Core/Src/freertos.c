/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
/* 应用层模块：UI 负责交互，Sensor 产生样本，Logger 消费样本并写入 SD 卡 */
#include "ui.h"
#include "key.h"
#include "stopwatch.h"
#include "sensor.h"
#include "logger.h"
#include "storage.h"
#include "remote.h"
#include "game.h"
#include "settings.h"
#include "rtc.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for ui_task */
osThreadId_t ui_taskHandle;
const osThreadAttr_t ui_task_attributes = {
  .name = "ui_task",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for sensor_task */
osThreadId_t sensor_taskHandle;
const osThreadAttr_t sensor_task_attributes = {
  .name = "sensor_task",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for logger_task */
osThreadId_t logger_taskHandle;
const osThreadAttr_t logger_task_attributes = {
  .name = "logger_task",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};
/* Definitions for motion_queue */
osMessageQueueId_t motion_queueHandle;
const osMessageQueueAttr_t motion_queue_attributes = {
  .name = "motion_queue"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void UI_Task(void *argument);
void Sensor_Task(void *argument);
void Logger_Task(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /*
   * Sensor_Task 以 100 Hz 产生 motion_sample_t，Logger_Task 异步消费。
   * 队列承接短时间 SD 卡写入造成的延迟，避免 Sensor_Task 被文件系统阻塞。
   */
  /* Create the queue(s) */
  /* creation of motion_queue */
  motion_queueHandle = osMessageQueueNew (32, 20, &motion_queue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /*
   * 三个任务的职责和优先级：
   * UI_Task     ：人机输入、页面绘制和周期更新
   * Sensor_Task ：固定 10 ms 采样
   * Logger_Task ：AboveNormal，及时清空样本队列并独占 FatFs 写入
   */
  /* Create the thread(s) */
  /* creation of ui_task */
  ui_taskHandle = osThreadNew(UI_Task, NULL, &ui_task_attributes);

  /* creation of sensor_task */
  sensor_taskHandle = osThreadNew(Sensor_Task, NULL, &sensor_task_attributes);

  /* creation of logger_task */
  logger_taskHandle = osThreadNew(Logger_Task, NULL, &logger_task_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_UI_Task */
/**
  * @brief  Function implementing the ui_task thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_UI_Task */
void UI_Task(void *argument)
{
  /* USER CODE BEGIN UI_Task */

	uint32_t last_clock_tick = 0;
	uint32_t next_repeat_tick = 0;
	ui_action_t held_action = UI_ACTION_NONE;

	uint8_t remote_command;

	uint32_t last_game_tick = 0;
	uint32_t last_rtc_service_tick = 0;

	/* UI 状态只由 UI_Task 修改，其他任务不直接操作页面或显示器 */
	ui_state_t state = {
			.page = UI_PAGE_LAUNCHER,
			.selected_index = 0,
			.first_visible = 0
	};

	ui_state_t previous_state;

	/* 设置为 RAM 内 V1 状态，上电恢复默认速度 */
	Settings_Init();

	UI_Draw(&state);

	/* Infinite loop */
	for(;;)
	{
		ui_action_t current_action;
		ui_action_t action_to_handle = UI_ACTION_NONE;

		/*
		 * 输入优先级：先取红外 ISR 已解码的单次命令；
		 * 没有遥控命令时才扫描实体按键，避免同一轮处理两个动作。
		 */
		remote_command = Remote_Pop_Command();

		if(remote_command != 0)
		{
		    held_action = UI_ACTION_NONE;

		    switch(remote_command)
		    {
		        case 0x46:
		            action_to_handle = UI_ACTION_UP;
		            break;

		        case 0x15:
		            action_to_handle = UI_ACTION_DOWN;
		            break;

		        case 0x40:
		            action_to_handle = UI_ACTION_ENTER;
		            break;

		        case 0x44:
		        	action_to_handle = UI_ACTION_LEFT;
		        	break;

		        case 0x43:
		        	action_to_handle = UI_ACTION_RIGHT;
		        	break;

		        case 0x45:
		        	action_to_handle = UI_ACTION_BACK;
		        	break;

		        default:
		            action_to_handle = UI_ACTION_NONE;
		            break;
		    }
		}
		else
		{
			/* 实体按键：首次按下延时 20 ms 确认，方向键支持长按重复，ENTER 不重复 */
			current_action = Key_Read_Action();

			if(current_action == UI_ACTION_NONE)
			{
				held_action = UI_ACTION_NONE;
			}
			else if(held_action == UI_ACTION_NONE)
			{
				osDelay(20);

				if(Key_Read_Action() == current_action)
				{
					held_action = current_action;
					next_repeat_tick = osKernelGetTickCount() + 400;
					action_to_handle = current_action;
				}
			}
			else if(current_action != held_action)
			{
				held_action = UI_ACTION_NONE;
			}
			else if(current_action != UI_ACTION_ENTER &&
					(int32_t)(osKernelGetTickCount() - next_repeat_tick) >= 0)
			{
				next_repeat_tick = osKernelGetTickCount() + 150;
				action_to_handle = current_action;
			}

		}


		/*
		 * UI_Handle_Action 只修改 state 或应用模块状态；
		 * 返回 1 表示状态已变化，随后按页面和操作类型选择全量或局部刷新。
		 */
		if(action_to_handle != UI_ACTION_NONE)
		{
			previous_state = state;

			if(UI_Handle_Action(&state, action_to_handle) != 0)
			{
				UI_Draw_State_Change(&previous_state, &state, action_to_handle);
			}

			last_clock_tick = osKernelGetTickCount();

			if(previous_state.page != UI_PAGE_GAME && state.page == UI_PAGE_GAME)
			{
				last_game_tick = osKernelGetTickCount();
			}
		}

		/*
		 * STM32F1 的日期由 HAL 软件维护，必须定期读取以推进跨天日期。
		 * 该维护独立于 Clock 页面，避免在其他页面跨天后复位丢失新日期。
		 */
		if(osKernelGetTickCount() - last_rtc_service_tick >= 1000)
		{
			RTC_Service_Date();
			last_rtc_service_tick = osKernelGetTickCount();
		}

		/* 当前页面的周期更新。Game 使用独立节拍，不能被时钟/按键刷新重置。 */
		if(state.page == UI_PAGE_CLOCK &&
		   osKernelGetTickCount() - last_clock_tick >= 1000)
		{
			UI_Update_Clock();
			last_clock_tick = osKernelGetTickCount();
		}
		else if(state.page == UI_PAGE_STOPWATCH &&
				Stopwatch_Is_Running() &&
				osKernelGetTickCount() - last_clock_tick >= 1000)
		{
			UI_Update_Stopwatch();
			last_clock_tick = osKernelGetTickCount();
		}
		else if(state.page == UI_PAGE_MOTION &&
				osKernelGetTickCount() - last_clock_tick >= 250)
		{
			UI_Update_Motion();
			last_clock_tick = osKernelGetTickCount();
		}
		else if(state.page == UI_PAGE_GAME &&
				osKernelGetTickCount() - last_game_tick >=
				Settings_Get_Game_Interval_Ms())
		{
			if(Game_Update() != 0)
			{
				UI_Update_Game();
			}

			last_game_tick = osKernelGetTickCount();
		}

		osDelay(1);
	}

  /* USER CODE END UI_Task */
}

/* USER CODE BEGIN Header_Sensor_Task */
/**
* @brief Function implementing the sensor_task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Sensor_Task */
void Sensor_Task(void *argument)
{
  /* USER CODE BEGIN Sensor_Task */
    HAL_StatusTypeDef result;
    uint32_t next_tick;
    sensor_state_t state;
    motion_sample_t sample;
    osStatus_t queue_result;

    next_tick = osKernelGetTickCount();

    /* 传感器未连好或初始化失败时不退出任务，每 100 ms 重试一次 */
    for(;;)
    {
        result = Sensor_Init();

        if(result == HAL_OK)
        {
            break;
        }

        osDelay(100);
    }

    /*
     * 使用绝对节拍保持 10 ms 周期，避免本轮 I2C 耗时累积到下一轮。
     * 该任务只采集和投递样本，不直接调用 FatFs。
     */
    for(;;)
    {
    	next_tick += 10;

        result = Sensor_Update();

        if(result == HAL_OK)
        {
        	Sensor_Get_State(&state);

        	sample.timestamp_ms = osKernelGetTickCount();

        	sample.acc_x = state.acc_x;
        	sample.acc_y = state.acc_y;
        	sample.acc_z = state.acc_z;

        	sample.gyro_x = state.gyro_x;
        	sample.gyro_y = state.gyro_y;
        	sample.gyro_z = state.gyro_z;

        	sample.roll_x100 = (int16_t)(state.roll_deg * 100.0f);
        	sample.pitch_x100 = (int16_t)(state.pitch_deg * 100.0f);

        	/* timeout=0：队列满时宁可记录丢样，不阻塞下一次传感器采样 */
        	queue_result = osMessageQueuePut(motion_queueHandle, &sample, 0, 0);

        	if(queue_result != osOK)
        	{
        		Logger_Notify_Sample_Dropped();
        	}
        }

        osDelayUntil(next_tick);
    }
  /* USER CODE END Sensor_Task */
}

/* USER CODE BEGIN Header_Logger_Task */
/**
* @brief Function implementing the logger_task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Logger_Task */
void Logger_Task(void *argument)
{
  /* USER CODE BEGIN Logger_Task */
	motion_sample_t sample;
	osStatus_t result;

  /*
   * 先处理 UI 发来的开始/停止 Thread Flag，再最多等待 10 ms 取一条样本。
   * Logger 是唯一执行 FatFs 写入的任务。
   */
  /* Infinite loop */
	for(;;)
	{
		Logger_Handle_Request();

		result = osMessageQueueGet(motion_queueHandle, &sample, NULL, 10);

		if(result == osOK)
		{
			Logger_Process_Sample(&sample);
		}
	}
  /* USER CODE END Logger_Task */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */


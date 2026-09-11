/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    rtc.c
  * @brief   This file provides code for the configuration
  *          of the RTC instances.
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
#include "rtc.h"

/* USER CODE BEGIN 0 */
#define RTC_BKP_MARKER 0xA5AEU

// STM32F1 日期由 HAL 软件维护，缓存用于避免每秒重复写入备份寄存器
static RTC_DateTypeDef s_last_saved_date;
static uint8_t s_date_cache_valid;
/* USER CODE END 0 */

RTC_HandleTypeDef hrtc;

void RTC_Save_Date_To_Backup(const RTC_DateTypeDef *date)
{
  uint32_t saved_date;

  if(date == NULL)
  {
    return;
  }

  saved_date = ((uint16_t)date->Year << 9)
             | ((uint16_t)date->Month << 5)
             | date->Date;

	// DR1 为有效标记，DR2 打包年月日，DR3 单独保存星期
	HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR3, date->WeekDay);
  HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR2, saved_date);
  HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, RTC_BKP_MARKER);

  s_last_saved_date = *date;
  s_date_cache_valid = 1;
}

void RTC_Service_Date(void)
{
  RTC_DateTypeDef current_date;

	// GetDate 会依据 RTC 秒计数推进 DateToUpdate，不依赖 Clock 页面是否打开
	if(HAL_RTC_GetDate(&hrtc, &current_date, RTC_FORMAT_BIN) != HAL_OK)
  {
    return;
  }

  if(s_date_cache_valid != 0 &&
     current_date.Year == s_last_saved_date.Year &&
     current_date.Month == s_last_saved_date.Month &&
     current_date.Date == s_last_saved_date.Date &&
     current_date.WeekDay == s_last_saved_date.WeekDay)
  {
    return;
  }

  RTC_Save_Date_To_Backup(&current_date);
}

/* RTC init function */
void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef DateToUpdate = {0};

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.AsynchPrediv = RTC_AUTO_1_SECOND;
  hrtc.Init.OutPut = RTC_OUTPUTSOURCE_ALARM;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN Check_RTC_BKUP */
  if (HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR1) == RTC_BKP_MARKER)
  {
	  uint16_t saved_date;

	  saved_date = (uint16_t)HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR2);

	  hrtc.DateToUpdate.Year =
	      (uint8_t)(saved_date >> 9);

	  hrtc.DateToUpdate.Month =
	      (uint8_t)((saved_date >> 5) & 0x0F);

	  hrtc.DateToUpdate.Date =
	      (uint8_t)(saved_date & 0x1F);

	  hrtc.DateToUpdate.WeekDay =
	      (uint8_t)HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR3);

      return;
  }
  /* USER CODE END Check_RTC_BKUP */

  /** Initialize RTC and set the Time and Date
  */
  sTime.Hours = 15;
  sTime.Minutes = 13;
  sTime.Seconds = 40;

  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
  {
    Error_Handler();
  }
  DateToUpdate.WeekDay = RTC_WEEKDAY_FRIDAY;
  DateToUpdate.Month = RTC_MONTH_SEPTEMBER;
  DateToUpdate.Date = 11;
  DateToUpdate.Year = 26;

  if (HAL_RTC_SetDate(&hrtc, &DateToUpdate, RTC_FORMAT_BIN) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */
  RTC_Save_Date_To_Backup(&DateToUpdate);
  /* USER CODE END RTC_Init 2 */

}

void HAL_RTC_MspInit(RTC_HandleTypeDef* rtcHandle)
{

  if(rtcHandle->Instance==RTC)
  {
  /* USER CODE BEGIN RTC_MspInit 0 */

  /* USER CODE END RTC_MspInit 0 */
    HAL_PWR_EnableBkUpAccess();
    /* Enable BKP CLK enable for backup registers */
    __HAL_RCC_BKP_CLK_ENABLE();
    /* RTC clock enable */
    __HAL_RCC_RTC_ENABLE();
  /* USER CODE BEGIN RTC_MspInit 1 */

  /* USER CODE END RTC_MspInit 1 */
  }
}

void HAL_RTC_MspDeInit(RTC_HandleTypeDef* rtcHandle)
{

  if(rtcHandle->Instance==RTC)
  {
  /* USER CODE BEGIN RTC_MspDeInit 0 */

  /* USER CODE END RTC_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_RTC_DISABLE();
  /* USER CODE BEGIN RTC_MspDeInit 1 */

  /* USER CODE END RTC_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/*
 * logger.c
 *
 *  Created on: Sep 7, 2026
 *      Author: cuojue
 */


#include "logger.h"
#include <string.h>
#include <stdio.h>
#include "cmsis_os.h"
#include "storage.h"


extern osThreadId_t logger_taskHandle;

// Logger 是日志文件句柄的唯一拥有者，UI 不能在记录期间访问 FatFs
static logger_state_t s_logger_state;
static FIL s_log_file;
static char s_write_buffer[512];
static UINT s_buffer_used;
static char s_line_buffer[96];
static uint16_t s_buffer_sample_count;

static FRESULT Logger_Flush_Buffer(void)
{
	FRESULT result;
	UINT bytes_written;

	if(s_buffer_used == 0)
	{
		return FR_OK;
	}

	// 以 512 Byte 批量写卡，避免每条采样都触发一次 FatFs/SDIO 写入
	result = f_write(&s_log_file, s_write_buffer, s_buffer_used, &bytes_written);

	if(result != FR_OK || bytes_written != s_buffer_used)
	{
		/* 写失败后文件句柄在这里关闭并进入 ERROR，调用者不能再次关闭 */
		s_logger_state.last_result = (result != FR_OK) ? result:FR_INT_ERR;

	    f_close(&s_log_file);

	    s_buffer_used = 0;
	    s_buffer_sample_count = 0;

	    s_logger_state.status = LOGGER_ERROR;


	    return s_logger_state.last_result;
	}

	s_logger_state.written_sample_count += s_buffer_sample_count;

	s_buffer_used = 0;
	s_buffer_sample_count = 0;
	s_logger_state.last_result = FR_OK;

	return FR_OK;
}

void Logger_Process_Sample(const motion_sample_t *sample)
{
    FRESULT result;
    int line_length;

	if(sample == NULL)
	{
		return;
	}

	// Logger_Task 从 motion_queue 取到的每一条样本都会计数，未记录时只丢弃内容
	s_logger_state.received_sample_count++;

	if(s_logger_state.status != LOGGER_RECORDING)
	{
		return;
	}

    line_length = snprintf(
        s_line_buffer,
        sizeof(s_line_buffer),
        "%d,%d\r\n",
        sample->roll_x100,
        sample->pitch_x100
    );

    if(line_length < 0 || line_length >= sizeof(s_line_buffer))
    {
        s_logger_state.status = LOGGER_ERROR;
        s_logger_state.last_result = FR_INT_ERR;

        return;
    }

    if(s_buffer_used + line_length > sizeof(s_write_buffer))
    {
    	result  = Logger_Flush_Buffer();

    	if(result != FR_OK)
    	{
    		return;
    	}
    }

    memcpy(&s_write_buffer[s_buffer_used], s_line_buffer, line_length);

    s_buffer_used += line_length;
    s_buffer_sample_count++;

}

void Logger_Get_State(logger_state_t *state)
{
	if(state == NULL)
	{
		return;
	}

	*state = s_logger_state;
}

FRESULT Logger_Start(void)
{
	FRESULT result;
	UINT byte_written;

    static const char csv_header[] = "R100,P100\r\n";

    /* 启动顺序：挂载文件系统、确保 LOG 目录存在、创建并写入新 CSV 文件 */
    result = Storage_Init();

    if(result != FR_OK)
    {
        s_logger_state.status = LOGGER_ERROR;
        s_logger_state.last_result = result;

        return result;
    }

	result = f_mkdir("0:/LOG");

	if(result != FR_OK && result != FR_EXIST)
	{
		s_logger_state.status = LOGGER_ERROR;
		s_logger_state.last_result = result;

		return result;
	}

	/* FA_CREATE_ALWAYS 表示每次开始记录覆盖上一次的 MOTION01.CSV */
	result = f_open(&s_log_file, "0:/LOG/MOTION01.CSV", FA_WRITE | FA_CREATE_ALWAYS);

	if(result != FR_OK)
	{
		s_logger_state.status = LOGGER_ERROR;
		s_logger_state.last_result = result;

		return result;
	}

	result = f_write(&s_log_file, csv_header, sizeof(csv_header) - 1, &byte_written);

	if(result != FR_OK || byte_written != sizeof(csv_header) - 1)
	{
		f_close(&s_log_file);

		s_logger_state.status = LOGGER_ERROR;
		s_logger_state.last_result = (result != FR_OK) ? result:FR_INT_ERR;

		return s_logger_state.last_result;
	}

	s_logger_state.received_sample_count = 0;
	s_logger_state.written_sample_count = 0;
	s_logger_state.dropped_sample_count = 0;

	s_buffer_used = 0;
	s_buffer_sample_count = 0;

	s_logger_state.status = LOGGER_RECORDING;
	s_logger_state.last_result = result;

	return result;
}

FRESULT Logger_Stop(void)
{
	FRESULT result;

	if(s_logger_state.status != LOGGER_RECORDING)
	{
		return FR_OK;
	}

	result = Logger_Flush_Buffer();

	if(result != FR_OK)
	{
		return result;
	}

	result = f_close(&s_log_file);

	if(result != FR_OK)
	{
		s_logger_state.status = LOGGER_ERROR;
		s_logger_state.last_result = result;

		return result;
	}

	s_logger_state.status = LOGGER_IDLE;
	s_logger_state.last_result = FR_OK;

	return FR_OK;
}

void Logger_Request_Start(void)
{
	// UI_Task 只发请求；文件打开和写入始终在 Logger_Task 上下文完成
	osThreadFlagsSet(logger_taskHandle, LOGGER_FLAG_START);
}
void Logger_Request_Stop(void)
{
	osThreadFlagsSet(logger_taskHandle, LOGGER_FLAG_STOP);
}
void Logger_Handle_Request(void)
{
	uint32_t flags;

	flags = osThreadFlagsWait(LOGGER_FLAG_START | LOGGER_FLAG_STOP, osFlagsWaitAny, 0);

	if((int32_t)flags < 0)
	{
		return;
	}

	/* 同时收到 STOP 和 START 时优先停止，避免当前文件尚未关闭就重新打开 */
	if(flags & LOGGER_FLAG_STOP)
	{
		Logger_Stop();
	}
	else if(flags & LOGGER_FLAG_START)
	{
		Logger_Start();
	}
}
uint8_t Logger_Is_Recording(void)
{
	return s_logger_state.status == LOGGER_RECORDING;
}

void Logger_Notify_Sample_Dropped(void)
{
	s_logger_state.dropped_sample_count++;
}

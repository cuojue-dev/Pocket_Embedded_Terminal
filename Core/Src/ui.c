/*
 * ui.c
 *
 *  Created on: Sep 5, 2026
 *      Author: cuojue
 */


#include <string.h>
#include <stdio.h>

#include "st7735.h"
#include "ui.h"
#include "rtc.h"
#include "stopwatch.h"
#include "storage.h"
#include "fatfs.h"
#include "sensor.h"
#include "logger.h"
#include "FreeRTOS.h"
#include "task.h"
#include "remote.h"
#include "game.h"
#include "settings.h"


#define MENU_ITEM_COUNT  7U
#define VISIBLE_ROWS     6U

// UI_Task 独占 ui_state_t；以下缓存用于局部刷新，避免频繁全屏重画
static game_state_t s_last_game_state;
static RTC_TimeTypeDef s_last_clock_time;
static RTC_DateTypeDef s_last_clock_date;
static uint8_t s_clock_cache_valid;


static const char * const s_menu_items[MENU_ITEM_COUNT] = {
    "Clock",
    "Stopwatch",
    "Files",
    "Motion",
    "Game",
    "System",
    "Settings"
};

static void UI_Draw_Focus_Box(uint16_t x, uint16_t y,
							  uint16_t width, uint16_t height,
							  uint16_t color)
{
	uint16_t box_x = x;
	uint16_t box_y = y;
	uint16_t box_w = width;
	uint16_t box_h = height;

	ST7735_Fill_Rect(box_x, box_y, box_w, 2, color);
	ST7735_Fill_Rect(box_x, box_y + box_h - 2, box_w, 2, color);

	ST7735_Fill_Rect(box_x, box_y, 2, box_h, color);
	ST7735_Fill_Rect(box_x + box_w - 2, box_y, 2, box_h, color);
}

static void UI_Draw_Launcher_Focus(uint8_t selected_index,
								   uint8_t first_visible,
								   uint16_t color)
{
	uint8_t row;
	uint16_t text_y;
	uint16_t box_w;

	if(selected_index < first_visible ||
	   selected_index >= first_visible + VISIBLE_ROWS ||
	   selected_index >= MENU_ITEM_COUNT)
	{
		return;
	}

	row = selected_index - first_visible;
	text_y = 24 + row * 20;
	box_w = strlen(s_menu_items[selected_index]) * 8 + 10;

	UI_Draw_Focus_Box(15, text_y - 4, box_w, 24, color);
}

static void UI_Draw_Storage_Focus(uint8_t selected_index, uint16_t color)
{
	uint16_t text_y;
	uint16_t box_w;

	if(selected_index >= Storage_Get_File_Count())
	{
		return;
	}

	text_y = 32 + selected_index * 20;
	box_w = strlen(Storage_Get_File_Name(selected_index)) * 8 + 8;

	UI_Draw_Focus_Box(15, text_y - 4, box_w, 22, color);
}

void UI_Draw_Launcher(uint8_t selected_index, uint8_t first_visible)
{
	uint8_t row;
	uint8_t box_x = 15;
	uint8_t box_y;
	uint8_t box_w = 50;
	uint8_t box_h = 24;
	uint8_t text_y;
	uint8_t item_index;

	ST7735_Fill_Color(0x0000);
	ST7735_Draw_String(20, 0, "Project26", 0xFFFF, 0x0000);

	for(row = 0;row < VISIBLE_ROWS;row++)
	{
		item_index = first_visible + row;

		if(item_index >= MENU_ITEM_COUNT)
		{
			break;
		}

		text_y = 24 + row * 20;

		if(item_index == selected_index)
		{
			box_y = text_y -4;
			box_w = strlen(s_menu_items[item_index]) * 8 + 10;

			UI_Draw_Focus_Box(box_x, box_y, box_w, box_h, 0xFFFF);
		}

		ST7735_Draw_String(20, text_y, s_menu_items[item_index], 0xFFFF, 0x0000);
	}
}

void UI_Draw_Clock(void)
{
	ST7735_Fill_Color(0x0000);
	s_clock_cache_valid = 0;

	ST7735_Draw_String(20, 24, "Clock", 0xFFFF, 0x0000);
	ST7735_Draw_String(20, 128, "UP:Back", 0xFFFF, 0x0000);

	UI_Update_Clock();
}

void UI_Update_Clock(void)
{
	RTC_TimeTypeDef rtc_time;
	RTC_DateTypeDef rtc_date;
	uint8_t time_changed;
	uint8_t date_changed;

	char time_text[9] = "00:00:00";
	char date_text[11] = "2000-00-00";

	// STM32F1 先读 Time 再读 Date，HAL 才能按秒计数推进软件日历
	if(HAL_RTC_GetTime(&hrtc, &rtc_time, RTC_FORMAT_BIN) != HAL_OK)
	{
		return;
	}

    if (HAL_RTC_GetDate(&hrtc, &rtc_date, RTC_FORMAT_BIN) != HAL_OK)
    {
        return;
    }

	time_changed = !s_clock_cache_valid ||
				rtc_time.Hours != s_last_clock_time.Hours ||
				rtc_time.Minutes != s_last_clock_time.Minutes ||
				rtc_time.Seconds != s_last_clock_time.Seconds;

	date_changed = !s_clock_cache_valid ||
				rtc_date.Year != s_last_clock_date.Year ||
				rtc_date.Month != s_last_clock_date.Month ||
				rtc_date.Date != s_last_clock_date.Date ||
				rtc_date.WeekDay != s_last_clock_date.WeekDay;

	time_text[0] = rtc_time.Hours /10 + '0';
	time_text[1] = rtc_time.Hours % 10 + '0';
	time_text[3] = rtc_time.Minutes / 10 + '0';
	time_text[4] = rtc_time.Minutes % 10 + '0';
	time_text[6] = rtc_time.Seconds / 10 + '0';
	time_text[7] = rtc_time.Seconds % 10 + '0';

	date_text[2] = rtc_date.Year / 10 + '0';
	date_text[3] = rtc_date.Year % 10 + '0';
	date_text[5] = rtc_date.Month / 10 + '0';
	date_text[6] = rtc_date.Month % 10 + '0';
	date_text[8] = rtc_date.Date / 10 + '0';
	date_text[9] = rtc_date.Date % 10 + '0';

	if(time_changed)
	{
		ST7735_Fill_Rect(20, 56, 64, 16, 0x0000);
		ST7735_Draw_String(20, 56, time_text, 0xFFFF, 0x0000);
	}

	if(date_changed)
	{
		ST7735_Fill_Rect(20, 88, 88, 16, 0x0000);
		ST7735_Draw_String(20, 88, date_text, 0xFFFF, 0x0000);
	}

	s_last_clock_time = rtc_time;
	s_last_clock_date = rtc_date;
	s_clock_cache_valid = 1;
}

void UI_Draw_Stopwatch(void)
{
	ST7735_Fill_Color(0x0000);

	ST7735_Draw_String(20, 24, "Stopwatch", 0xFFFF, 0x0000);
	ST7735_Draw_String(20, 128, "UP:Back", 0xFFFF, 0x0000);

	UI_Update_Stopwatch();
}

void UI_Update_Stopwatch(void)
{
	char time_text[9] = "00:00:00";
	uint32_t total_seconds = Stopwatch_Get_Elapsed_Ms() / 1000;

	uint32_t hours = total_seconds / 3600;
	uint32_t minutes = (total_seconds % 3600) / 60;
	uint32_t seconds = total_seconds % 60;

	time_text[0] = hours /10 + '0';
	time_text[1] = hours % 10 + '0';
	time_text[3] = minutes / 600 + '0';
	time_text[4] = minutes / 60 + '0';
	time_text[6] = seconds / 10 + '0';
	time_text[7] = seconds % 10 + '0';

	ST7735_Fill_Rect(20, 56, 64, 16, 0x0000);
	ST7735_Draw_String(20, 56, time_text, 0xFFFF, 0x0000);
}

void UI_Draw_Storage(uint8_t selected_index)
{
	uint8_t row;
	uint8_t file_count;
	uint16_t text_y;
	uint16_t box_w;
	uint16_t box_y;
	uint16_t box_h;

	ST7735_Fill_Color(0x0000);

	ST7735_Draw_String(20, 8, "Files", 0xFFFF, 0x0000);

	UI_Update_Storage();

	file_count = Storage_Get_File_Count();

	for(row = 0;row < file_count; row++)
	{
		text_y = 32 + row * 20;
		box_y = text_y - 4;
		box_h = 22;
		box_w = strlen(Storage_Get_File_Name(row)) * 8 + 8;

		ST7735_Draw_String(20, text_y, Storage_Get_File_Name(row), 0xFFFF, 0x0000);

		if(row == selected_index)
		{
			UI_Draw_Focus_Box(15, box_y, box_w, box_h, 0xFFFF);
		}
	}

	ST7735_Draw_String(20, 128, "UP:Back", 0xFFFF, 0x0000);
}

void UI_Update_Storage(void)
{
	char result_text[20];
	FRESULT list_result;

	list_result = Storage_List_Current_Dir();

	if(list_result != FR_OK)
	{
	    snprintf(result_text, sizeof(result_text), "DIR:%u", (unsigned)list_result);

		ST7735_Draw_String(20, 112, result_text, 0xFFFF, 0x0000);

		return;
	}
}

void UI_Draw_File_View(void)
{
	const char *text;
	uint16_t x = 20;
	uint16_t y = 40;

	ST7735_Fill_Color(0x0000);
	ST7735_Draw_String(20, 8, "File", 0xFFFF, 0x0000);

	/* Storage 已按完整换行裁剪当前页，这里只负责把 CSV 字节流排到屏幕 */
	text = Storage_Read_File_Buf();

	while(*text != '\0' && y <= 104)
	{
		if(*text == '\r')
		{
			text++;
			continue;
		}

		if(*text == '\n')
		{
			x = 20;
			y += 16;
			text++;
			continue;
		}

		ST7735_Draw_Char(x, y, *text, 0xFFFF, 0x0000);
		x += 8;

		if(x > 116)
		{
			x = 20;
			y += 16;
		}

		text++;
	}

	ST7735_Draw_String(20, 128, "UP:Back", 0xFFFF, 0x0000);
}

void UI_Draw_Motion(void)
{
	ST7735_Fill_Color(0x0000);

	ST7735_Draw_String(20, 8, "Motion", 0xFFFF, 0x0000);
	ST7735_Draw_String(8, 136, "K0:Rec UP:Back", 0xFFFF, 0x0000);

	UI_Update_Motion();
}

void UI_Update_Motion(void)
{
	sensor_state_t state;
	char text[16];
	logger_state_t logger_state;

	Sensor_Get_State(&state);

	Logger_Get_State(&logger_state);

	if(logger_state.status == LOGGER_RECORDING)
	{
	    ST7735_Draw_String(20, 120, "REC:ON ", 0xFFFF, 0x0000);
	}
	else if(logger_state.status == LOGGER_ERROR)
	{
	    snprintf(text, sizeof(text), "ERR:%-3u", (unsigned)logger_state.last_result);
	    ST7735_Draw_String(12, 120, text, 0xFFFF, 0x0000);
	}
	else
	{
	    ST7735_Draw_String(12, 120, "REC:OFF", 0xFFFF, 0x0000);
	}

    snprintf(text, sizeof(text), "Roll :%4d", (int)state.roll_deg);
    ST7735_Draw_String(12, 32, text, 0xFFFF, 0x0000);

    snprintf(text, sizeof(text), "Pitch:%4d", (int)state.pitch_deg);
    ST7735_Draw_String(12, 48, text, 0xFFFF, 0x0000);

    snprintf(text, sizeof(text), "AX:%6d", state.acc_x);
    ST7735_Draw_String(20, 72, text, 0xFFFF, 0x0000);

//    snprintf(text, sizeof(text), "AY:%6d", state.acc_y);
//    ST7735_Draw_String(20, 88, text, 0xFFFF, 0x0000);

//    snprintf(text, sizeof(text), "AZ:%6d", state.acc_z);
//    ST7735_Draw_String(20, 104, text, 0xFFFF, 0x0000);

    snprintf(
        text,
        sizeof(text),
        "F:%08lX",
        (unsigned long)Remote_Get_Last_Command());

    ST7735_Draw_String(12, 88, text, 0xFFFF, 0x0000);

    snprintf(
        text,
        sizeof(text),
        "IR:%5u",
        (unsigned)Remote_Get_Last_high_Us());

    ST7735_Draw_String(20, 104, text, 0xFFFF, 0x0000);
}

void UI_Draw_Game(void)
{
	char text[16];
	game_state_t game_state;

	Game_Get_State(&game_state);

	ST7735_Fill_Color(0x0000);

	ST7735_Draw_String(36, 8, "DODGE", 0xFFFF, 0x0000);

	ST7735_Fill_Rect(4, 28, 120, 2, 0x7BEF);
	ST7735_Fill_Rect(4, 130, 120, 2, 0x7BEF);
	ST7735_Fill_Rect(4, 28, 2, 104, 0x7BEF);
	ST7735_Fill_Rect(122, 28, 2, 104, 0x7BEF);

	ST7735_Fill_Rect(
			game_state.player_x,
			game_state.player_y,
			GAME_PLAYER_SIZE,
			GAME_PLAYER_SIZE,
			0x07FF);
	ST7735_Fill_Rect(
			game_state.obstacle_x,
			game_state.obstacle_y,
			GAME_OBSTACLE_SIZE,
			GAME_OBSTACLE_SIZE,
			0xF800);
	ST7735_Fill_Rect(
	    game_state.right_obstacle_x,
	    game_state.right_obstacle_y,
	    GAME_OBSTACLE_SIZE,
	    GAME_OBSTACLE_SIZE,
	    0xFD20
	);

	s_last_game_state = game_state;

	ST7735_Draw_String(22, 136, "PWR:Exit", 0xFFFF, 0x0000);

	if(game_state.game_over != 0)
	{
	    ST7735_Draw_String(20, 68, "GAME OVER", 0xF800, 0x0000);
	    ST7735_Draw_String(20, 88, "PLAY:Again", 0xFFFF, 0x0000);
	}

	snprintf(text, sizeof(text), "S:%03u", (unsigned)game_state.score);
	ST7735_Draw_String_Small(84, 16, text, 0xFFFF, 0x0000);
}

void UI_Update_Game(void)
{
	char text[16];
	game_state_t game_state;

	Game_Get_State(&game_state);

	if(game_state.game_over != 0)
	{
	    UI_Draw_Game();
	    return;
	}

	ST7735_Fill_Rect(
			s_last_game_state.obstacle_x,
			s_last_game_state.obstacle_y,
			GAME_OBSTACLE_SIZE,
			GAME_OBSTACLE_SIZE,
			0x0000);

	ST7735_Fill_Rect(
			s_last_game_state.right_obstacle_x,
			s_last_game_state.right_obstacle_y,
			GAME_OBSTACLE_SIZE,
			GAME_OBSTACLE_SIZE,
			0x0000
	);

	ST7735_Fill_Rect(
			game_state.obstacle_x,
			game_state.obstacle_y,
			GAME_OBSTACLE_SIZE,
			GAME_OBSTACLE_SIZE,
			0xF800);

	ST7735_Fill_Rect(
			game_state.right_obstacle_x,
			game_state.right_obstacle_y,
			GAME_OBSTACLE_SIZE,
			GAME_OBSTACLE_SIZE,
			0xFD20
	);

	ST7735_Fill_Rect(
			game_state.player_x,
			game_state.player_y,
			GAME_PLAYER_SIZE,
			GAME_PLAYER_SIZE,
			0x07FF);

	if(game_state.score != s_last_game_state.score)
	{
	    ST7735_Fill_Rect(84, 16, 44, 8, 0x0000);

	    snprintf(
	        text,
	        sizeof(text),
	        "S:%03u",
	        (unsigned)game_state.score
	    );

	    ST7735_Draw_String_Small(84, 16, text, 0xFFFF, 0x0000);
	}

	s_last_game_state = game_state;
}

static void UI_Update_Game_Player(void)
{
	game_state_t game_state;

	Game_Get_State(&game_state);

	// 移动玩家时仅擦除旧玩家并重画必要障碍物，不触发整页刷新
	ST7735_Fill_Rect(
        s_last_game_state.player_x,
        s_last_game_state.player_y,
        GAME_PLAYER_SIZE,
        GAME_PLAYER_SIZE,
        0x0000
    );

    ST7735_Fill_Rect(
        s_last_game_state.obstacle_x,
        s_last_game_state.obstacle_y,
        GAME_OBSTACLE_SIZE,
        GAME_OBSTACLE_SIZE,
        0xF800
    );

    ST7735_Fill_Rect(
        game_state.player_x,
        game_state.player_y,
        GAME_PLAYER_SIZE,
        GAME_PLAYER_SIZE,
        0x07FF
    );

    s_last_game_state = game_state;
}

void UI_Draw_System(void)
{
	logger_state_t logger_state;

	Logger_Get_State(&logger_state);

	ST7735_Fill_Color(0x0000);

	ST7735_Draw_String(36, 8, "System", 0xFFFF, 0x0000);
	ST7735_Draw_String(12, 32, "MCU:F103ZET6", 0xFFFF, 0x0000);
	ST7735_Draw_String(12, 48, "LCD:ST7735S", 0xFFFF, 0x0000);
	ST7735_Draw_String(12, 64, "IMU:MPU6500", 0xFFFF, 0x0000);
	ST7735_Draw_String(12, 80, "IR:NEC PB9", 0xFFFF, 0x0000);

	if(logger_state.status == LOGGER_RECORDING)
	{
		ST7735_Draw_String(12, 104, "LOG:REC", 0x07E0, 0x0000);
	}
	else if(logger_state.status == LOGGER_ERROR)
	{
		ST7735_Draw_String(12, 104, "LOG:ERR", 0xF800, 0x0000);
	}
	else
	{
		ST7735_Draw_String(12, 104, "LOG:IDLE", 0xFFFF, 0x0000);
	}

	ST7735_Draw_String(20, 136, "UP:Back", 0xFFFF, 0x0000);
}

void UI_Draw_Settings(void)
{
	ST7735_Fill_Color(0x0000);

	ST7735_Draw_String(28, 8, "Settings", 0xFFFF, 0x0000);
	ST7735_Draw_String(12, 40, "Game Speed", 0xFFFF, 0x0000);
	ST7735_Draw_String(12, 64,
			Settings_Get_Game_Speed_Name(), 0x07FF, 0x0000);
	ST7735_Draw_String(8, 112, "DOWN:Change", 0xFFFF, 0x0000);
	ST7735_Draw_String(8, 128, "ENT:Default", 0xFFFF, 0x0000);
    ST7735_Draw_String(20, 144, "UP:Back", 0xFFFF, 0x0000);
}

void UI_Update_Settings(void)
{
	ST7735_Fill_Rect(12, 64, 80, 16, 0x0000);
	ST7735_Draw_String(12, 64,
			Settings_Get_Game_Speed_Name(), 0x07FF, 0x0000);
}


uint8_t UI_Handle_Action(ui_state_t *state, ui_action_t action)
{
	/* 页面状态机：本函数只改变状态或调用对应服务，绘制由 UI_Draw_State_Change 统一处理 */
	if(state->page == UI_PAGE_LAUNCHER)
	{
		if(action == UI_ACTION_UP)
		{
			if(state->selected_index > 0)
			{
				state->selected_index--;

				if(state->selected_index < state->first_visible)
				{
					state->first_visible = state->selected_index;
				}
			}
			else
			{
				return 0;
			}
		}
		else if(action == UI_ACTION_DOWN)
		{
			if(state->selected_index < 6)
			{
				state->selected_index++;

				if(state->selected_index >= state->first_visible + 6)
				{
					state->first_visible = state->selected_index - 5;
				}
			}
			else
			{
				return 0;
			}
		}
		else if(action == UI_ACTION_ENTER)
		{
			if(state->selected_index == 0)
			{
				state->page = UI_PAGE_CLOCK;
			}
			else if(state->selected_index == 1)
			{
				state->page = UI_PAGE_STOPWATCH;
			}
			else if(state->selected_index == 2)
			{
				logger_state_t logger_state;
				FRESULT result;

				Logger_Get_State(&logger_state);

				/* FatFs 未启用重入，记录期间拒绝 Files 与 Logger 并发访问 SD 卡 */
				if(logger_state.status != LOGGER_IDLE)
				{
				    return 0;
				}

				vTaskSuspendAll();
				result = Storage_Open_Root();
				(void)xTaskResumeAll();

			    if(result == FR_OK)
			    {
			        state->page = UI_PAGE_FILES;
			        state->file_selected_index = 0;
			        return 1;
			    }

			    return 0;
			}
			else if(state->selected_index == 3)
			{
				state->page = UI_PAGE_MOTION;
			}
			else if(state->selected_index == 4)
			{
				Game_Init();
				state->page = UI_PAGE_GAME;
			}
			else if(state->selected_index == 5)
			{
				state->page = UI_PAGE_SYSTEM;
			}
			else if(state->selected_index == 6)
			{
				state->page = UI_PAGE_SETTINGS;
			}
			else
			{
				return 0;
			}
		}

		return 1;
	}
	else if(state->page == UI_PAGE_CLOCK)
	{
		if(action == UI_ACTION_UP)
		{
			state->page = UI_PAGE_LAUNCHER;
			return 1;
		}
		else
		{
			return 0;
		}
	}
	else if(state->page == UI_PAGE_STOPWATCH)
	{
		if(action == UI_ACTION_UP)
		{
			state->page = UI_PAGE_LAUNCHER;
			return 1;
		}
		else if(action == UI_ACTION_DOWN)
		{
			Stopwatch_Reset();
			return 1;
		}
		else if(action == UI_ACTION_ENTER)
		{
			if(!Stopwatch_Is_Running())
			{
				Stopwatch_Start();
			}
			else
			{
				Stopwatch_Pause();
			}

			return 1;
		}
	}
	else if(state->page == UI_PAGE_FILES)
	{
		uint8_t file_count = Storage_Get_File_Count();
		FRESULT enter_result;

		if(action == UI_ACTION_UP)
		{
			FRESULT result;
			if(state->file_selected_index > 0)
			{
				state->file_selected_index--;
				return 1;
			}
			else if(state->file_selected_index == 0)
			{
				if(Storage_Is_Root())
				{
					state->page = UI_PAGE_LAUNCHER;
					return 1;
				}
			}


			vTaskSuspendAll();
			result = Storage_Go_Back();
			(void)xTaskResumeAll();

			if(result == FR_OK)
			{
				state->file_selected_index = state->file_parent_index;
				return 1;
			}

			return 0;
		}
		else if(action == UI_ACTION_DOWN)
		{
			if(file_count == 0)
			{
				return 0;
			}

			if(state->file_selected_index + 1 < file_count)
			{
				state->file_selected_index++;
				return 1;
			}

			return 0;
		}
		else if(action == UI_ACTION_ENTER)
		{
			if(file_count == 0)
			{
				return 0;
			}

			/* 目录进入与文件读取都可能触发轮询 SDIO，期间暂时禁止任务切换 */
			if(Storage_Get_File_Attr(state->file_selected_index) & AM_DIR)
			{
				vTaskSuspendAll();

				enter_result = Storage_Enter_Selected_Directory(state->file_selected_index);

				(void)xTaskResumeAll();

				if(enter_result == FR_OK)
			    {
			    	state->file_parent_index = state->file_selected_index;
			        state->file_selected_index = 0;
			        return 1;
			    }

				ST7735_Fill_Rect(20, 112, 80, 16, 0x0000);

				{
				    char error_text[16];

				    snprintf(error_text, sizeof(error_text), "ENTER:%u", (unsigned)enter_result);

				    ST7735_Draw_String(20, 112, error_text, 0xFFFF, 0x0000);
				}

			    return 0;
			}
			else
			{
				FRESULT read_result;
				UINT page_bytes = 0;

				vTaskSuspendAll();
				read_result = Storage_Read_Selected_File_At(state->file_selected_index, 0, &page_bytes);
				(void)xTaskResumeAll();

				if(read_result == FR_OK && page_bytes > 0)
				{
					state->file_view_offset = 0;
					state->file_view_page_bytes = (uint16_t)page_bytes;
					state->page = UI_PAGE_FILE_VIEW;
					return 1;
				}
			}
		}

		return 0;
	}
	else if(state->page == UI_PAGE_FILE_VIEW)
	{
		if(action == UI_ACTION_UP)
		{
			state->page = UI_PAGE_FILES;
			return 1;
		}
		else if(action == UI_ACTION_DOWN)
		{
			uint32_t next_offset;
			UINT next_page_bytes = 0;
			FRESULT result;

			/* 当前页实际显示的 Byte 数决定下一页文件偏移，保证不会重复或跳行 */
			next_offset = state->file_view_offset + state->file_view_page_bytes;

			vTaskSuspendAll();
			result = Storage_Read_Selected_File_At(
					state->file_selected_index, next_offset, &next_page_bytes);
			(void)xTaskResumeAll();

		    if(result != FR_OK || next_page_bytes == 0)
		    {
		        return 0;
		    }

		    state->file_view_offset = next_offset;
		    state->file_view_page_bytes = (uint16_t)next_page_bytes;

			return 1;
		}
		else if(action == UI_ACTION_ENTER)
		{
			UINT next_page_bytes = 0;
			FRESULT result;

			vTaskSuspendAll();
			result = Storage_Read_Selected_File_At(
					state->file_selected_index, 0, &next_page_bytes);
			(void)xTaskResumeAll();

		    if(result != FR_OK || next_page_bytes == 0)
		    {
		        return 0;
		    }

		    state->file_view_offset = 0;
		    state->file_view_page_bytes = next_page_bytes;

			return 1;
		}

		return 0;
	}
	else if(state->page == UI_PAGE_MOTION)
	{
		if(action == UI_ACTION_UP)
		{
			state->page = UI_PAGE_LAUNCHER;
			return 1;
		}
		else if(action == UI_ACTION_ENTER)
		{
			logger_state_t logger_state;

			Logger_Get_State(&logger_state);

			if(logger_state.status == LOGGER_IDLE ||
			   logger_state.status == LOGGER_ERROR)
			{
			    Logger_Request_Start();
			    return 1;
			}
			else if(logger_state.status == LOGGER_RECORDING)
			{
			    Logger_Request_Stop();
			    return 1;
			}

			return 0;
		}

		return 0;
	}
	else if(state->page == UI_PAGE_SYSTEM)
	{
		if(action == UI_ACTION_UP || action == UI_ACTION_BACK)
		{
			state->page = UI_PAGE_LAUNCHER;
			return 1;
		}

		return 0;
	}
	else if(state->page == UI_PAGE_SETTINGS)
	{
		if(action == UI_ACTION_UP || action == UI_ACTION_BACK)
		{
			state->page = UI_PAGE_LAUNCHER;
			return 1;
		}
		else if(action == UI_ACTION_DOWN)
		{
			Settings_Next_Game_Speed();
			return 1;
		}
		else if(action == UI_ACTION_ENTER)
		{
			Settings_Reset_Game_Speed();
			return 1;
		}

		return 0;
	}
	else if(state->page == UI_PAGE_GAME)
	{
		if(action == UI_ACTION_BACK)
		{
			state->page = UI_PAGE_LAUNCHER;
			return 1;
		}
		else if(action == UI_ACTION_UP)
		{
			return Game_Move_Up();
		}
		else if(action == UI_ACTION_DOWN)
		{
			return Game_Move_Down();
		}
		else if(action == UI_ACTION_LEFT)
		{
			return Game_Move_Left();
		}
		else if(action == UI_ACTION_RIGHT)
		{
			return Game_Move_Right();
		}
		else if(action == UI_ACTION_ENTER)
		{
			Game_Init();
			return 1;
		}

		return 0;
	}

	return 0;
}

void UI_Draw(const ui_state_t *state)
{
	switch(state->page)
	{
		case UI_PAGE_LAUNCHER:
			UI_Draw_Launcher(state->selected_index, state->first_visible);
			break;

		case UI_PAGE_CLOCK:
			UI_Draw_Clock();
			break;

		case UI_PAGE_STOPWATCH:
			UI_Draw_Stopwatch();
			break;

		case UI_PAGE_FILES:
			UI_Draw_Storage(state->file_selected_index);
			break;

		case UI_PAGE_FILE_VIEW:
			UI_Draw_File_View();
			break;

		case UI_PAGE_MOTION:
			UI_Draw_Motion();
			break;

		case UI_PAGE_GAME:
			UI_Draw_Game();
			break;

		case UI_PAGE_SYSTEM:
			UI_Draw_System();
			break;

		case UI_PAGE_SETTINGS:
			UI_Draw_Settings();
			break;
	}
}

void UI_Draw_State_Change(const ui_state_t *previous_state,
						  const ui_state_t *current_state,
						  ui_action_t action)
{
	// 换页才全量绘制；同页操作优先走下面的局部刷新分支
	if(previous_state->page != current_state->page)
	{
		UI_Draw(current_state);
		return;
	}

	if(current_state->page == UI_PAGE_LAUNCHER &&
	   (action == UI_ACTION_UP || action == UI_ACTION_DOWN) &&
	   previous_state->first_visible == current_state->first_visible)
	{
		UI_Draw_Launcher_Focus(previous_state->selected_index,
				previous_state->first_visible, 0x0000);
		UI_Draw_Launcher_Focus(current_state->selected_index,
				current_state->first_visible, 0xFFFF);
		return;
	}

	if(current_state->page == UI_PAGE_FILES &&
	   (action == UI_ACTION_DOWN ||
	   (action == UI_ACTION_UP && previous_state->file_selected_index > 0)))
	{
		UI_Draw_Storage_Focus(previous_state->file_selected_index, 0x0000);
		UI_Draw_Storage_Focus(current_state->file_selected_index, 0xFFFF);
		return;
	}

	if(current_state->page == UI_PAGE_STOPWATCH &&
	   (action == UI_ACTION_DOWN || action == UI_ACTION_ENTER))
	{
		UI_Update_Stopwatch();
		return;
	}

	if(current_state->page == UI_PAGE_MOTION &&
		action == UI_ACTION_ENTER)
	{
		UI_Update_Motion();
		return;
	}

	if(current_state->page == UI_PAGE_GAME &&
	   previous_state->page == UI_PAGE_GAME &&
	   (action == UI_ACTION_UP ||
	    action == UI_ACTION_DOWN ||
	    action == UI_ACTION_LEFT ||
	    action == UI_ACTION_RIGHT))
	{
	    UI_Update_Game_Player();
	    return;
	}

	if(current_state->page == UI_PAGE_SETTINGS &&
	   previous_state->page == UI_PAGE_SETTINGS &&
	   (action == UI_ACTION_DOWN || action == UI_ACTION_ENTER))
	{
		UI_Update_Settings();
		return;
	}
	UI_Draw(current_state);
}



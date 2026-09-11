/*
 * ui.h
 *
 *  Created on: Sep 5, 2026
 *      Author: cuojue
 */

#ifndef INC_UI_H_
#define INC_UI_H_


#include <stdint.h>


typedef enum
{
	UI_PAGE_LAUNCHER = 0,
	UI_PAGE_CLOCK,
	UI_PAGE_STOPWATCH,
	UI_PAGE_FILES,
	UI_PAGE_FILE_VIEW,
	UI_PAGE_MOTION,
	UI_PAGE_GAME,
	UI_PAGE_SYSTEM,
	UI_PAGE_SETTINGS
}ui_page_t;

typedef enum
{
	UI_ACTION_NONE = 0,
	UI_ACTION_UP,
	UI_ACTION_DOWN,
	UI_ACTION_ENTER,
	UI_ACTION_LEFT,
	UI_ACTION_RIGHT,
	UI_ACTION_BACK
}ui_action_t;

typedef struct
{
	ui_page_t page;
	uint8_t selected_index;
	uint8_t first_visible;

	uint8_t file_selected_index; // files 页面选中项
	uint8_t file_parent_index; //一级目录，后续升级为返回栈

	uint32_t file_view_offset; // 当前页从文件第几个 Byte 开始
	uint16_t file_view_page_bytes; // 当前页实际显示了多少 Byte
}ui_state_t;

void UI_Draw_Launcher(uint8_t selected_index, uint8_t first_visible);
void UI_Draw_Clock(void);
void UI_Update_Clock(void);
void UI_Draw_Stopwatch(void);
void UI_Update_Stopwatch(void);
void UI_Draw_Storage(uint8_t selected_index);
void UI_Update_Storage(void);
void UI_Draw_File_View(void);
void UI_Draw_Motion(void);
void UI_Update_Motion(void);
void UI_Draw_Game(void);
void UI_Update_Game(void);
void UI_Draw_System(void);
void UI_Draw_Settings(void);
void UI_Update_Settings(void);
uint8_t UI_Handle_Action(ui_state_t *state, ui_action_t action);
void UI_Draw(const ui_state_t *state);
void UI_Draw_State_Change(const ui_state_t *previous_state,
						  const ui_state_t *current_state,
						  ui_action_t action);


#endif /* INC_UI_H_ */

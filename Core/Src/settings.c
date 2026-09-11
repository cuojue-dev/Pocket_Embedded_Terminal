/*
 * settings.c
 */

#include "settings.h"

// 当前 V1 设置仅由 UI_Task 读写，因此无需额外同步机制
static settings_game_speed_t s_game_speed;

void Settings_Init(void)
{
    s_game_speed = SETTINGS_SPEED_NORMAL;
}

void Settings_Next_Game_Speed(void)
{
	// 设置页只提供循环切换，Fast 的下一档回到 Slow
    s_game_speed++;

    if(s_game_speed > SETTINGS_SPEED_FAST)
    {
        s_game_speed = SETTINGS_SPEED_SLOW;
    }
}

void Settings_Reset_Game_Speed(void)
{
    s_game_speed = SETTINGS_SPEED_NORMAL;
}

uint32_t Settings_Get_Game_Interval_Ms(void)
{
	// 间隔越短，Game_Update 调用越频繁，障碍物移动越快
    switch(s_game_speed)
    {
        case SETTINGS_SPEED_SLOW:
            return 180U;

        case SETTINGS_SPEED_FAST:
            return 60U;

        case SETTINGS_SPEED_NORMAL:
        default:
            return 100U;
    }
}

const char *Settings_Get_Game_Speed_Name(void)
{
    switch(s_game_speed)
    {
        case SETTINGS_SPEED_SLOW:
            return "Slow";

        case SETTINGS_SPEED_FAST:
            return "Fast";

        case SETTINGS_SPEED_NORMAL:
        default:
            return "Normal";
    }
}

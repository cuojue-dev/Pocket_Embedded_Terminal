/*
 * settings.h
 */

#ifndef INC_SETTINGS_H_
#define INC_SETTINGS_H_

#include <stdint.h>

typedef enum
{
    SETTINGS_SPEED_SLOW = 0,
    SETTINGS_SPEED_NORMAL,
    SETTINGS_SPEED_FAST
} settings_game_speed_t;

void Settings_Init(void);
void Settings_Next_Game_Speed(void);
void Settings_Reset_Game_Speed(void);
uint32_t Settings_Get_Game_Interval_Ms(void);
const char *Settings_Get_Game_Speed_Name(void);

#endif /* INC_SETTINGS_H_ */

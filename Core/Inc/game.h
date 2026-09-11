/*
 * game.h
 *
 *  Created on: Sep 10, 2026
 *      Author: cuojue
 */

#ifndef INC_GAME_H_
#define INC_GAME_H_


#include <stdint.h>


#define GAME_PLAYER_SIZE 12U
#define GAME_OBSTACLE_SIZE 10U

typedef struct
{
	uint16_t player_x;
	uint16_t player_y;

	uint16_t obstacle_x;
	uint16_t obstacle_y;
	uint16_t right_obstacle_x;
	uint16_t right_obstacle_y;

	uint8_t game_over;

	uint16_t score;
}game_state_t;

void Game_Init(void);

uint8_t Game_Move_Up(void);
uint8_t Game_Move_Down(void);
uint8_t Game_Move_Left(void);
uint8_t Game_Move_Right(void);
uint8_t Game_Update(void);

void Game_Get_State(game_state_t *state);


#endif /* INC_GAME_H_ */

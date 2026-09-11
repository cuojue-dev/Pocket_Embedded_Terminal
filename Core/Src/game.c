/*
 * game.c
 *
 *  Created on: Sep 10, 2026
 *      Author: cuojue
 */


#include "game.h"
#include <stddef.h>

// 游戏逻辑层只维护游戏状态，不直接操作 ST7735 显示

#define GAME_LEFT 6U
#define GAME_TOP 30U
#define GAME_RIGHT   122U   // 右边界，采用右开区间
#define GAME_BOTTOM  130U   // 下边界，采用下开区间

#define GAME_MOVE_STEP 8U
#define GAME_OBSTACLE_STEP 4U

#define GAME_LANE_COUNT ((GAME_RIGHT - GAME_LEFT - GAME_OBSTACLE_SIZE) \
						  / GAME_MOVE_STEP + 1)
#define GAME_LANE_RIGHT_COUNT ((GAME_BOTTOM - GAME_TOP - GAME_OBSTACLE_SIZE) \
						  / GAME_MOVE_STEP + 1)

static game_state_t s_game_state;
// 固定种子的线性同余伪随机序列，用于选择障碍物出生跑道
static uint32_t s_random_state = 0x13579BDFUL;

static uint32_t Game_Random(void)
{
	s_random_state = s_random_state * 1664525UL + 1013904223UL;

	return s_random_state;
}

static void Game_Respawn_Top_Obstacle(void)
{
	uint32_t lane;

	lane = (Game_Random() >> 16) % GAME_LANE_COUNT;

	s_game_state.obstacle_x = GAME_LEFT + lane * GAME_MOVE_STEP;

	s_game_state.obstacle_y = GAME_TOP;
}

static void Game_Respawn_Right_Obstacle(void)
{
    uint32_t lane;

    lane = (Game_Random() >> 16) % GAME_LANE_RIGHT_COUNT;

    s_game_state.right_obstacle_x = GAME_RIGHT - GAME_OBSTACLE_SIZE;

    s_game_state.right_obstacle_y = GAME_TOP + lane * GAME_MOVE_STEP;
}

static uint8_t Game_Rect_Collision(uint16_t obstacle_x, uint16_t obstacle_y)
{
    // 两个矩形在 X、Y 方向都重叠，才算发生碰撞
    return
        s_game_state.player_x < obstacle_x + GAME_OBSTACLE_SIZE &&
        s_game_state.player_x + GAME_PLAYER_SIZE > obstacle_x &&
        s_game_state.player_y < obstacle_y + GAME_OBSTACLE_SIZE &&
        s_game_state.player_y + GAME_PLAYER_SIZE > obstacle_y;
}

void Game_Init(void)
{
	/* 每局从固定玩家位置开始，两类障碍物各自随机选择出生跑道 */
	s_game_state.player_x = 54;
	s_game_state.player_y = 102;

	Game_Respawn_Top_Obstacle();
	Game_Respawn_Right_Obstacle();

	s_game_state.game_over = 0;
	s_game_state.score = 0;
}

uint8_t Game_Move_Up(void)
{
	if(s_game_state.game_over != 0)
	{
	    return 0;
	}

	if(s_game_state.player_y >= GAME_TOP + GAME_MOVE_STEP)
	{
		s_game_state.player_y -= GAME_MOVE_STEP;
		return 1;
	}
	else
	{
		return 0;
	}
}
uint8_t Game_Move_Down(void)
{
	if(s_game_state.game_over != 0)
	{
	    return 0;
	}

	if(s_game_state.player_y + GAME_PLAYER_SIZE +
		       GAME_MOVE_STEP <= GAME_BOTTOM)
	{
		s_game_state.player_y += GAME_MOVE_STEP;
		return 1;
	}
	else
	{
		return 0;
	}
}
uint8_t Game_Move_Left(void)
{
	if(s_game_state.game_over != 0)
	{
	    return 0;
	}

	if(s_game_state.player_x >= GAME_LEFT + GAME_MOVE_STEP)
	{
		s_game_state.player_x -= GAME_MOVE_STEP;
		return 1;
	}
	else
	{
		return 0;
	}
}
uint8_t Game_Move_Right(void)
{
	if(s_game_state.game_over != 0)
	{
	    return 0;
	}

	if(s_game_state.player_x + GAME_PLAYER_SIZE +
		       GAME_MOVE_STEP <= GAME_RIGHT)
	{
		s_game_state.player_x += GAME_MOVE_STEP;
		return 1;
	}
	else
	{
		return 0;
	}
}

uint8_t Game_Update(void)
{
	if(s_game_state.game_over != 0)
	{
		return 0;
	}

	/* 顶部障碍物向下移动，到底后重生并计一分 */
	if(s_game_state.obstacle_y +
			GAME_OBSTACLE_SIZE +
			GAME_OBSTACLE_STEP <= GAME_BOTTOM)
	{
		s_game_state.obstacle_y += GAME_OBSTACLE_STEP;
	}
	else
	{
		Game_Respawn_Top_Obstacle();
		s_game_state.score++;
	}

	/* 右侧障碍物向左移动，到左边后重生并计一分 */
	if(s_game_state.right_obstacle_x >= GAME_LEFT + GAME_OBSTACLE_STEP)
	{
	    s_game_state.right_obstacle_x -= GAME_OBSTACLE_STEP;
	}
	else
	{
	    Game_Respawn_Right_Obstacle();
	    s_game_state.score++;
	}


	/* 两个障碍物任意一个碰到玩家，当前局立即结束 */
	if(Game_Rect_Collision(s_game_state.obstacle_x, s_game_state.obstacle_y) ||
	   Game_Rect_Collision( s_game_state.right_obstacle_x, s_game_state.right_obstacle_y))
	{
	    s_game_state.game_over = 1;
	}

	return 1;
}

void Game_Get_State(game_state_t *state)
{
	if(state == NULL)
	{
		return;
	}

	*state = s_game_state;
}

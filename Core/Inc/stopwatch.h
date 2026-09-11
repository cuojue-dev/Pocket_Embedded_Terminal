/*
 * stopwatch.h
 *
 *  Created on: Sep 5, 2026
 *      Author: cuojue
 */

#ifndef INC_STOPWATCH_H_
#define INC_STOPWATCH_H_


#include <stdint.h>

void Stopwatch_Init(void);
void Stopwatch_Start(void);
void Stopwatch_Pause(void);
void Stopwatch_Reset(void);
uint32_t Stopwatch_Get_Elapsed_Ms(void);
uint8_t Stopwatch_Is_Running(void);


#endif /* INC_STOPWATCH_H_ */

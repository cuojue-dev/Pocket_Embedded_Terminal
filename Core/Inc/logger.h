/*
 * logger.h
 *
 *  Created on: Sep 7, 2026
 *      Author: cuojue
 */

#ifndef INC_LOGGER_H_
#define INC_LOGGER_H_


#include <stdint.h>
#include "sensor.h"
#include "ff.h"

#define LOGGER_FLAG_START (1U << 0)
#define LOGGER_FLAG_STOP (1U << 1)

typedef enum
{
	LOGGER_IDLE = 0,
	LOGGER_RECORDING,
	LOGGER_ERROR
}logger_status_t;

typedef struct
{
	logger_status_t status;
	uint32_t received_sample_count;
	uint32_t written_sample_count;
	uint32_t dropped_sample_count;
	FRESULT last_result;
}logger_state_t;

void Logger_Process_Sample(const motion_sample_t *sample);
void Logger_Get_State(logger_state_t *state);
FRESULT Logger_Start(void);
FRESULT Logger_Stop(void);
void Logger_Request_Start(void);
void Logger_Request_Stop(void);
void Logger_Handle_Request(void);
uint8_t Logger_Is_Recording(void);
void Logger_Notify_Sample_Dropped(void);

#endif /* INC_LOGGER_H_ */

/*
 * remote.h
 *
 *  Created on: Sep 9, 2026
 *      Author: cuojue
 */

#ifndef INC_REMOTE_H_
#define INC_REMOTE_H_


#include <stdint.h>

void Remote_Init(void);
uint16_t Remote_Get_Last_high_Us(void);
void Remote_Handle_Timeout(void);
uint32_t Remote_Get_Last_Frame_Data(void);
uint8_t Remote_Get_Last_Command(void);
uint8_t Remote_Pop_Command(void);


#endif /* INC_REMOTE_H_ */

/*
 * storage.h
 *
 *  Created on: Sep 6, 2026
 *      Author: cuojue
 */

#ifndef INC_STORAGE_H_
#define INC_STORAGE_H_


#include "ff.h"

#define STORAGE_FILE_PAGE_SIZE 49U


FRESULT Storage_Init(void);
FRESULT Storage_Write_Read_Test(void);
FRESULT Storage_List_Dir(const char *path);
FRESULT Storage_Enter_Selected_Directory(uint8_t index);
FRESULT Storage_Go_Back(void);
uint8_t Storage_Is_Root(void);
uint8_t Storage_Get_File_Count(void);
const char *Storage_Get_File_Name(uint8_t index);
BYTE Storage_Get_File_Attr(uint8_t index);
uint32_t Storage_Get_Sd_Error(void);
FRESULT Storage_List_Current_Dir(void);
FRESULT Storage_Open_Root(void);
FRESULT Storage_Read_File(const char *path);
const char *Storage_Read_File_Buf(void);
FRESULT Storage_Read_Selected_File(uint8_t index);
FRESULT Storage_Read_Selected_File_At(uint8_t index, uint32_t offset, UINT *read_bytes);

#endif /* INC_STORAGE_H_ */

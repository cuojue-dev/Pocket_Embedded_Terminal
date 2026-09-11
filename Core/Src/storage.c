/*
 * storage.c
 *
 *  Created on: Sep 6, 2026
 *      Author: cuojue
 */


#include "storage.h"
#include "fatfs.h"
#include "sdio.h"

#include <string.h>
#include <stdio.h>


#define STORAGE_MAX_ITMES 6
#define STORAGE_NAME_LEN  13

// 当前目录及其条目缓存是 Files 页的唯一数据源
// 调用者必须保证 Logger 未占用 FatFs，并在轮询 SDIO 时避免任务切换
static uint8_t s_file_count;
static char s_file_names[STORAGE_MAX_ITMES][STORAGE_NAME_LEN];
static BYTE s_file_attributes[STORAGE_MAX_ITMES];
static char s_current_path[64] = "0:/";
static char s_read_file_rx_buffer[STORAGE_FILE_PAGE_SIZE + 1];

FRESULT Storage_Init(void)
{
	return f_mount(&SDFatFS, SDPath, 1);
}

FRESULT Storage_Write_Read_Test(void)
{
	FRESULT result;
	const char write_data[] = "ABCD";
	UINT bytes_written = 0;
	char read_data[sizeof(write_data)] = {0};
	UINT bytes_read = 0;
	int compare_result;


	result = f_open(&SDFile, "0:/test.txt", FA_CREATE_ALWAYS | FA_WRITE);

	if(result != FR_OK)
	{
		return result;
	}

	result = f_write(&SDFile, write_data, sizeof(write_data) - 1, &bytes_written);

	if(result != FR_OK)
	{
		f_close(&SDFile);
		return result;
	}

	if(bytes_written != sizeof(write_data) - 1)
	{
		f_close(&SDFile);
		return FR_INT_ERR;
	}

	result = f_close(&SDFile);

	if(result != FR_OK)
	{
		return result;
	}

	result = f_open(&SDFile, "0:/test.txt", FA_READ);

	if(result != FR_OK)
	{
		return result;
	}

	result = f_read(&SDFile, read_data, sizeof(read_data) - 1, &bytes_read);

	if(result != FR_OK)
	{
		f_close(&SDFile);
		return FR_INT_ERR;
	}

	if(bytes_read != sizeof(read_data) - 1)
	{
		f_close(&SDFile);
		return result;
	}

	compare_result = memcmp(read_data, write_data, sizeof(write_data) - 1);

	if(compare_result != 0)
	{
	    f_close(&SDFile);
	    return FR_INT_ERR;
	}

	return f_close(&SDFile);
}

FRESULT Storage_List_Dir(const char *path)
{
	FRESULT result;
	DIR dir;
	FILINFO fno;

	/* 每次列目录都覆盖 UI 使用的条目缓存，当前 V1 最多展示 6 项 */
	s_file_count = 0;

	result = f_opendir(&dir, path);

	if(result != FR_OK)
	{
		return result;
	}

	while(s_file_count < STORAGE_MAX_ITMES)
	{
		result = f_readdir(&dir, &fno);

		if(result != FR_OK || fno.fname[0] == '\0')
		{
			break;
		}

		strncpy(s_file_names[s_file_count], fno.fname, STORAGE_NAME_LEN - 1);

		s_file_names[s_file_count][STORAGE_NAME_LEN - 1] = '\0';
		s_file_attributes[s_file_count] = fno.fattrib;
		s_file_count++;
	}

	f_closedir(&dir);

	return result;
}

FRESULT Storage_Enter_Selected_Directory(uint8_t index)
{
    char next_path[64];
    FRESULT result;
    int path_length;

    if(index >= s_file_count)
    {
    	return FR_INVALID_OBJECT;
    }

    if(!(s_file_attributes[index] & AM_DIR))
    {
    	return FR_INVALID_OBJECT;
    }

    if(strcmp(s_current_path, "0:/") == 0)
    {
    	path_length = snprintf(next_path, sizeof(next_path), "%s%s",
    			s_current_path, Storage_Get_File_Name(index));
    }
    else
    {
    	path_length = snprintf(next_path, sizeof(next_path), "%s/%s",
    			s_current_path, s_file_names[index]);
    }


	if(path_length < 0 || path_length >= (int)sizeof(next_path))
	{
	    return FR_INVALID_NAME;
	}

	/* 先验证目标目录能够列出，成功后才提交 s_current_path */
	result = Storage_List_Dir(next_path);

    if(result == FR_OK)
    {
    	strcpy(s_current_path, next_path);
    }

    return result;
}

FRESULT Storage_Go_Back(void)
{
	char *last_slash;
	FRESULT result;
	char parent_path[64];

	if(Storage_Is_Root())
	{
		return FR_INVALID_OBJECT;
	}

	// 先在局部副本中计算父目录，列目录失败时不能破坏当前浏览路径
	strcpy(parent_path, s_current_path);

	last_slash = strrchr(parent_path, '/');

	if(last_slash == NULL)
	{
	    return FR_INVALID_NAME;
	}

	if(last_slash == &parent_path[2])
	{
	    parent_path[3] = '\0';
	}
	else
	{
	    *last_slash = '\0';
	}

	result = Storage_List_Dir(parent_path);

	if(result == FR_OK)
	{
	    strcpy(s_current_path, parent_path);
	}

	return result;
}

uint8_t Storage_Is_Root(void)
{
	return strcmp(s_current_path, "0:/") == 0;
}

uint8_t Storage_Get_File_Count(void)
{
	return s_file_count;
}

const char *Storage_Get_File_Name(uint8_t index)
{
	return s_file_names[index];
}

BYTE Storage_Get_File_Attr(uint8_t index)
{
	return s_file_attributes[index];
}
uint32_t Storage_Get_Sd_Error(void)
{
    return HAL_SD_GetError(&hsd);
}

FRESULT Storage_List_Current_Dir(void)
{
	return Storage_List_Dir(s_current_path);
}

FRESULT Storage_Open_Root(void)
{
	FRESULT mount_result;

	/* Files 页入口总是重新挂载并回到根目录，避免保留上次浏览的路径 */
	mount_result = Storage_Init();

	if (mount_result != FR_OK)
	{
		return mount_result;
	}

    strcpy(s_current_path, "0:/");

    return Storage_List_Dir(s_current_path);
}

FRESULT Storage_Read_File(const char *path)
{
	FRESULT result;
	UINT read_bytes = 0;

	s_read_file_rx_buffer[0] = '\0';

	result = f_open(&SDFile, path, FA_READ);

	if(result != FR_OK)
	{
		return result;
	}

	result = f_read(&SDFile, s_read_file_rx_buffer,
			sizeof(s_read_file_rx_buffer) - 1, &read_bytes);

	if(result != FR_OK)
	{
		f_close(&SDFile);
		return result;
	}

	s_read_file_rx_buffer[read_bytes] = '\0';

	return f_close(&SDFile);
}

const char *Storage_Read_File_Buf(void)
{
	return s_read_file_rx_buffer;
}

FRESULT Storage_Read_Selected_File(uint8_t index)
{
	UINT read_bytes;

	return Storage_Read_Selected_File_At(index, 0, &read_bytes);
}

FRESULT Storage_Read_Selected_File_At(uint8_t index, uint32_t offset, UINT *read_bytes)
{
	FRESULT result;
	char next_path[64];
	int path_length;
	UINT raw_read_bytes;
	UINT page_bytes;

	if(read_bytes == NULL)
	{
		return FR_INVALID_PARAMETER;
	}

	if(index >= s_file_count)
	{
		return FR_INVALID_OBJECT;
	}

	if(s_file_attributes[index] & AM_DIR)
	{
		return FR_INVALID_OBJECT;
	}

	if(strcmp(s_current_path, "0:/") == 0)
	{
		path_length = snprintf(next_path, sizeof(next_path), "%s%s",
				s_current_path, s_file_names[index]);
	}
	else
	{
		path_length = snprintf(next_path, sizeof(next_path), "%s/%s",
				s_current_path, s_file_names[index]);
	}

	if(path_length < 0 || path_length >= (int)sizeof(next_path))
	{
		return FR_INVALID_NAME;
	}

	result = f_open(&SDFile, next_path, FA_READ);

	if(result != FR_OK)
	{
		return result;
	}

	// offset 是文件起点起的 Byte 偏移，用于 Files 页的向后翻页
	result = f_lseek(&SDFile, offset);

	if(result != FR_OK)
	{
		f_close(&SDFile);
		return result;
	}

	result = f_read(&SDFile, s_read_file_rx_buffer, STORAGE_FILE_PAGE_SIZE, &raw_read_bytes);

	if(result != FR_OK)
	{
		f_close(&SDFile);
		return result;
	}

	page_bytes = raw_read_bytes;

	if(raw_read_bytes == STORAGE_FILE_PAGE_SIZE)
	{
		// 满页时退回最后一个换行符，避免显示或下一页从半行 CSV 开始
		for(UINT i = raw_read_bytes;i > 0;i--)
		{
			if(s_read_file_rx_buffer[i - 1] == '\n')
			{
				page_bytes = i;
				break;
			}
		}
	}

	s_read_file_rx_buffer[page_bytes] = '\0';
	*read_bytes = page_bytes;

	return 	f_close(&SDFile);
}

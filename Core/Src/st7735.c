/*
 * st7735.c
 *
 *  Created on: Sep 5, 2026
 *      Author: cuojue
 */


#include "st7735.h"
#include "spi.h"
#include "font8x8_basic.h"

// 本模块负责 ST7735 的 SPI 传输和像素绘制；UI 模块只调用图形接口
static uint8_t pixel_format = 0x05;

void ST7735_Init(void)
{
	ST7735_Set_Backlight(0);
	ST7735_Reset();
	ST7735_Write_Command(0x11); // Sleep Out

	HAL_Delay(120);

	ST7735_Write_Command(0x3A); // 选择像素格式
	ST7735_Write_Data(&pixel_format, 1); //RGB565
	ST7735_Write_Command(0x29); //Display On

	ST7735_Set_Backlight(1);
}

void ST7735_Reset(void)
{
	HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);

	HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_RESET);

	HAL_Delay(1);

	HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_SET);

	HAL_Delay(120);
}

void ST7735_Set_Backlight(uint8_t enable)
{
	HAL_GPIO_WritePin(LCD_BL_GPIO_Port, LCD_BL_Pin, enable);
}

void ST7735_Write_Command(uint8_t command)
{
	HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);

	HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_RESET);

	HAL_SPI_Transmit(&hspi1, &command, 1, 100);

	HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
}

void ST7735_Write_Data(const uint8_t *data, uint16_t size)
{
	HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);

	HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET);

	HAL_SPI_Transmit(&hspi1, data, size, 100);

	HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
}

void ST7735_Set_Address_Window(uint16_t x0, uint16_t y0,
							   uint16_t x1, uint16_t y1)
{
	uint8_t x0_H = (uint8_t)(x0 >> 8);
	uint8_t x0_L = (uint8_t)x0;
	uint8_t x1_H = (uint8_t)(x1 >> 8);
	uint8_t x1_L = (uint8_t)x1;
	uint8_t y0_H = (uint8_t)(y0 >> 8);
	uint8_t y0_L = (uint8_t)y0;
	uint8_t y1_H = (uint8_t)(y1 >> 8);
	uint8_t y1_L = (uint8_t)y1;

	// 0x2A/0x2B 设定写入窗口，0x2C 后的像素按窗口从左到右、从上到下填充
	ST7735_Write_Command(0x2A); // 0x2A：设置 X 范围，x0 到 x1

	ST7735_Write_Data(&x0_H, 1);
	ST7735_Write_Data(&x0_L, 1);
	ST7735_Write_Data(&x1_H, 1);
	ST7735_Write_Data(&x1_L, 1);

	ST7735_Write_Command(0x2B); // 0x2B：设置 Y 范围，y0 到 y1

	ST7735_Write_Data(&y0_H, 1);
	ST7735_Write_Data(&y0_L, 1);
	ST7735_Write_Data(&y1_H, 1);
	ST7735_Write_Data(&y1_L, 1);

	ST7735_Write_Command(0x2C); //0x2C：从这个窗口的左上角开始连续写像素
}

void ST7735_Fill_Color(uint16_t color)
{
	uint8_t color_data[2];

	color_data[0] = (uint8_t)(color >> 8);
	color_data[1] = (uint8_t)color;

	ST7735_Set_Address_Window(0, 0, 127, 159);

	for(int i = 0; i < 128;i++)
	{
		for(int j = 0;j < 160;j++)
		{
			ST7735_Write_Data(color_data, 2);
		}
	}
}

void ST7735_Draw_Point(uint16_t x, uint16_t y, uint16_t color)
{
	uint8_t color_data[2];

	color_data[0] = (uint8_t)(color >> 8);
	color_data[1] = (uint8_t)color;

	// 越界就 return
	if(x > 127 || x < 0 || y > 159 || y < 0)
	{
		return;
	}

	ST7735_Set_Address_Window(x, y, x, y);

	ST7735_Write_Data(color_data, 2);
}

void ST7735_Fill_Rect(uint16_t x, uint16_t y,
					  uint16_t width, uint16_t height,
					  uint16_t color)
{
	uint8_t color_data[2];

	color_data[0] = (uint8_t)(color >> 8);
	color_data[1] = (uint8_t)color;

	// 越界或 width / height 为 0 就 return
	if(x > 127 || x < 0 || y > 159 || y < 0 ||
	   width == 0 || height == 0)
	{
		return;
	}

	// 若矩形超出右边或下边，裁剪 width / height
	if (width > 128U - x)
	{
	    width = 128U - x;
	}

	if (height > 160U - y)
	{
	    height = 160U - y;
	}

	/* 设置一次地址窗口后连续写入像素，比逐点设置窗口快得多 */
	ST7735_Set_Address_Window(x, y, x + width -1, y + height -1);

	// 循环 width × height 次
	for(int i = 0;i < width;i++)
	{
		for(int j = 0;j < height;j++)
		{
			ST7735_Write_Data(color_data, 2);
		}
	}
}

void ST7735_Draw_Char(uint16_t x, uint16_t y, char ch,
					  uint16_t foreground, uint16_t background)
{
	const uint8_t *glyph;
	uint8_t row;
	uint8_t col;
	uint8_t repeat;
	uint16_t color;
	uint8_t row_data;
	uint16_t index = 0;
	uint8_t pixel_buffer[8 * 16 * 2];

	if(ch > 0x7F || ch < 0x20 || x > 120 || y > 144)
	{
		return;
	}

	glyph = ST7735_FONT8X8_BASIC[ch - 0x20];

	ST7735_Set_Address_Window(x, y, x + 7, y + 15);

	// 字模本身是 8x8；每行重复两次，形成 UI 使用的 8x16 大字
	for(row = 0;row < 8;row++)
	{
		row_data = glyph[row];

		for(repeat = 0;repeat < 2;repeat++)
		{
			for(col = 0; col < 8;col++)
			{
				if ((row_data >> col) & 1)
				{
				    color = foreground;
				}
				else
				{
				    color = background;
				}

				pixel_buffer[index++] = (uint8_t)(color >> 8);
				pixel_buffer[index++] = (uint8_t)color;
			}
		}
	}

	ST7735_Write_Data(pixel_buffer, sizeof(pixel_buffer));
}

void ST7735_Draw_String(uint16_t x, uint16_t y, const char *text,
						uint16_t foreground, uint16_t background)
{
	while(*text != '\0')
	{
		ST7735_Draw_Char(x, y, *text, foreground, background);

		x += 8;
		text++;
	}
}

void ST7735_Draw_Char_Small(uint16_t x, uint16_t y, char ch,
                            uint16_t foreground,
                            uint16_t background)
{
	const uint8_t *glyph;
	uint8_t row;
	uint8_t col;
	uint16_t color;
	uint8_t row_data;
	uint16_t index = 0;
	uint8_t pixel_buffer[8 * 8 * 2];

	if(ch > 0x7F || ch < 0x20 || x > 120 || y > 152)
	{
		return;
	}

	glyph = ST7735_FONT8X8_BASIC[ch - 0x20];

	ST7735_Set_Address_Window(x, y, x + 7, y + 7);

	// 小字直接按原始 8x8 字模输出，适合分数等状态信息
	for(row = 0;row < 8;row++)
	{
		row_data = glyph[row];

		for(col = 0; col < 8;col++)
		{
            color = ((row_data >> col) & 1U) ? foreground : background;

			pixel_buffer[index++] = (uint8_t)(color >> 8);
			pixel_buffer[index++] = (uint8_t)color;
		}
	}

	ST7735_Write_Data(pixel_buffer, sizeof(pixel_buffer));
}

void ST7735_Draw_String_Small(uint16_t x, uint16_t y,
                              const char *text,
                              uint16_t foreground,
                              uint16_t background)
{
	while(*text != '\0')
	{
		ST7735_Draw_Char_Small(x, y, *text, foreground, background);

		x += 8;
		text++;
	}
}

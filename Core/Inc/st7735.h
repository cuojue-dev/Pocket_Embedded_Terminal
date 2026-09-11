/*
 * st7735.h
 *
 *  Created on: Sep 5, 2026
 *      Author: cuojue
 */

#ifndef INC_ST7735_H_
#define INC_ST7735_H_


#include <stdint.h>


void ST7735_Init(void);
void ST7735_Reset(void);
void ST7735_Set_Backlight(uint8_t enable);
void ST7735_Write_Command(uint8_t command);
void ST7735_Write_Data(const uint8_t *data, uint16_t size);
void ST7735_Set_Address_Window(uint16_t x0, uint16_t y0,
							   uint16_t x1, uint16_t y1);

void ST7735_Fill_Color(uint16_t color);
void ST7735_Draw_Point(uint16_t x, uint16_t y, uint16_t color);
void ST7735_Fill_Rect(uint16_t x, uint16_t y,
					  uint16_t width, uint16_t height,
					  uint16_t color);
void ST7735_Draw_Char(uint16_t x, uint16_t y, char ch,
					  uint16_t foreground, uint16_t background);
void ST7735_Draw_String(uint16_t x, uint16_t y, const char *text,
						uint16_t foreground, uint16_t background);
void ST7735_Draw_Char_Small(uint16_t x, uint16_t y, char ch,
                            uint16_t foreground,
                            uint16_t background);

void ST7735_Draw_String_Small(uint16_t x, uint16_t y,
                              const char *text,
                              uint16_t foreground,
                              uint16_t background);


#endif /* INC_ST7735_H_ */

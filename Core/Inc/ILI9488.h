#ifndef INC_ILI9488_H
#define INC_ILI9488_H

#include <stdint.h>
#include <fonts.h>

#include "stm32f4xx_hal.h"

#define ILI9488_WIDTH  320
#define ILI9488_HEIGHT 480

#define	RED			0xF800
#define	GREEN		0x07E0
#define	BLUE		0x001F
#define YELLOW		0xFFE0
#define MAGENTA		0xF81F
#define ORANGE		0xFD00
#define CYAN		0x07FF
#define	D_RED 		0xC000
#define	D_GREEN		0x0600
#define	D_BLUE		0x0018
#define D_YELLOW	0xC600
#define D_MAGENTA	0xC018
#define D_ORANGE	0xC300
#define D_CYAN		0x0618
#define	DD_RED		0x8000
#define	DD_GREEN	0x0400
#define DD_BLUE		0x0010
#define DD_YELLOW	0x8400
#define DD_MAGENTA	0x8020
#define DD_ORANGE	0x8200
#define DD_CYAN		0x0410
#define WHITE		0xFFFF
#define D_WHITE		0xC618
#define DD_WHITE	0x8410
#define DDD_WHITE	0x4208
#define DDDD_WHITE	0x2104
#define	BLACK		0x0000
#define color565(r, g, b) ((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | (((b) & 0xF8) >> 3))

typedef enum
{
    ILI9488_Orientation_0, ILI9488_Orientation_90, ILI9488_Orientation_180, ILI9488_Orientation_270
} ILI9488_Orientation_e;

typedef struct
{
    SPI_HandleTypeDef *spi;
    GPIO_TypeDef *dc_port;
    uint16_t dc_pin;
    GPIO_TypeDef *rst_port;
    uint16_t rst_pin;
} ILI9488_Config_t;

void ILI9488_Init(ILI9488_Config_t config, ILI9488_Orientation_e orientation);
void ILI9488_Orientation(ILI9488_Orientation_e orientation);

void ILI9488_FillArea(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void ILI9488_Pixel(uint16_t x, uint16_t y, uint16_t color);
void ILI9488_FillScreen(uint16_t bgcolor);
void ILI9488_DrawBorder(int16_t x, int16_t y, int16_t w, int16_t h, int16_t t, uint16_t color);
void ILI9488_DrawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
void ILI9488_FillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);

void ILI9488_WChar(uint16_t x, uint16_t y, char ch, sFONT font, uint8_t size, uint16_t color, uint16_t bgcolor);
void ILI9488_WString(uint16_t x, uint16_t y, const char *str, sFONT font, uint8_t size, uint16_t color,
                     uint16_t bgcolor);
void ILI9488_CString(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, const char *str, sFONT font, uint8_t size,
                     uint16_t color, uint16_t bgcolor);
void ILI9488_DrawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t *data, uint32_t size);
#endif /* INC_ILI9488_H */

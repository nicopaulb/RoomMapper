#include <stdbool.h>
#include <string.h>

#include "ILI9488.h"
#include "stm32f4xx_hal.h"

#define BUFFER_SIZE 2048

#define ILI9488_DMA_CUTOFF 	20

#define ILI9488_SLEEP_OUT			0x11
#define ILI9488_DISPLAY_ON			0x29
#define ILI9488_PIXEL_FORMAT    	0x3A
#define ILI9488_RGB_INTERFACE   	0xB0
#define ILI9488_MEMWR				0x2C
#define ILI9488_COLUMN_ADDR			0x2A
#define ILI9488_PAGE_ADDR			0x2B
#define ILI9488_MADCTL				0x36
#define ILI9488_MADCTL_0 			0X88
#define ILI9488_MADCTL_90 			0xE8
#define ILI9488_MADCTL_180 			0x48
#define ILI9488_MADCTL_270 			0x28
#define ILI9488_POWER0				0xC0
#define ILI9488_POWER1				0xC1
#define ILI9488_POWER2				0xC2
#define ILI9488_POWER3				0xC5

#define _swap_int16_t(a, b)  { int16_t t = a; a = b; b = t; }

ILI9488_Orientation_e orientation_cur;
int16_t ili9488_width = 0;
int16_t ili9488_height = 0;

static ILI9488_Config_t config;
static uint8_t dispBuffer1[BUFFER_SIZE];
static uint8_t dispBuffer2[BUFFER_SIZE];
static uint8_t *dispBuffer = dispBuffer1;

static void ILI9488_Transmit(uint8_t *data, uint16_t dataSize, bool command);
static void ILI9488_WriteCommand(uint8_t cmd);
static void ILI9488_WriteData(uint8_t *data, size_t size);
static void ILI9488_Reset();

void ILI9488_Init(ILI9488_Config_t conf, ILI9488_Orientation_e orientation)
{
	uint8_t data[4];

	config = conf;

	ILI9488_Reset();

	ILI9488_WriteCommand(ILI9488_PIXEL_FORMAT);
	data[0] = 0x66;		// RGB666
	ILI9488_WriteData(data, 1);

	ILI9488_WriteCommand(ILI9488_RGB_INTERFACE);
	data[0] = 0x80;
	ILI9488_WriteData(data, 1);

	ILI9488_WriteCommand(ILI9488_SLEEP_OUT);
	HAL_Delay(120);
	ILI9488_WriteCommand(ILI9488_DISPLAY_ON);

	ILI9488_Orientation(orientation);
}

void ILI9488_Orientation(ILI9488_Orientation_e orientation)
{
	static uint8_t data[1];
	switch (orientation)
	{
	case ILI9488_Orientation_0:
		data[0] = ILI9488_MADCTL_0;
		ili9488_height = ILI9488_HEIGHT;
		ili9488_width = ILI9488_WIDTH;
		break;
	case ILI9488_Orientation_90:
		data[0] = ILI9488_MADCTL_90;
		ili9488_height = ILI9488_WIDTH;
		ili9488_width = ILI9488_HEIGHT;
		break;
	case ILI9488_Orientation_180:
		data[0] = ILI9488_MADCTL_180;
		ili9488_height = ILI9488_HEIGHT;
		ili9488_width = ILI9488_WIDTH;
		break;
	case ILI9488_Orientation_270:
		data[0] = ILI9488_MADCTL_270;
		ili9488_height = ILI9488_WIDTH;
		ili9488_width = ILI9488_HEIGHT;
		break;
	}
	ILI9488_WriteCommand(ILI9488_MADCTL);
	ILI9488_WriteData(data, 1);
	orientation_cur = orientation;
}

void ILI9488_SetAddressWindow(uint16_t x1, uint16_t y1, uint16_t x2,
		uint16_t y2)
{
	uint8_t data[4];

	data[0] = (x1 & 0xFF00) >> 8;
	data[1] = (x1 & 0xFF);
	data[2] = (x2 & 0xFF00) >> 8;
	data[3] = (x2 & 0xFF);
	ILI9488_WriteCommand(ILI9488_COLUMN_ADDR);
	ILI9488_WriteData(data, 4);

	data[0] = (y1 & 0xFF00) >> 8;
	data[1] = (y1 & 0xFF);
	data[2] = (y2 & 0xFF00) >> 8;
	data[3] = (y2 & 0xFF);
	ILI9488_WriteCommand(ILI9488_PAGE_ADDR);
	ILI9488_WriteData(data, 4);
	ILI9488_WriteCommand(ILI9488_MEMWR);
}

void ILI9488_FillArea(uint16_t x1, uint16_t y1, uint16_t w, uint16_t h,
		uint16_t color)
{
	uint16_t times = 0;
	uint32_t totalDataSize = 0;
	uint32_t dataSize = 0;
	uint16_t x2 = 0;
	uint16_t y2 = 0;
	uint8_t red = (color & 0xF800) >> 8;
	uint8_t green = (color & 0x07E0) >> 3;
	uint8_t blue = (color & 0x001F) << 3;
	uint8_t *buf = dispBuffer;

	if ((x1 >= ili9488_width) || (y1 >= ili9488_height) || (w == 0) || (h == 0))
		return;

	x2 = x1 + w - 1;
	if (x2 > ili9488_width)
	{
		x2 = ili9488_width;
	}

	y2 = y1 + h - 1;
	if (y2 > ili9488_height)
	{
		y2 = ili9488_height;
	}

	totalDataSize = (((y2 - y1 + 1) * (x2 - x1 + 1)) * 3);
	dataSize = (
			totalDataSize < (BUFFER_SIZE - 3) ?
					totalDataSize : (BUFFER_SIZE - 3));

	while ((buf - dispBuffer) <= dataSize)
	{
		*(buf++) = red;
		*(buf++) = green;
		*(buf++) = blue;
	}
	dataSize = buf - dispBuffer;

	ILI9488_SetAddressWindow(x1, y1, x2, y2);

	times = (totalDataSize / dataSize);
	for (uint16_t i = 0; i < times; i++)
	{
		ILI9488_WriteData(dispBuffer, dataSize);
	}
	ILI9488_WriteData(dispBuffer, (totalDataSize - (times * dataSize)));

	dispBuffer = (dispBuffer == dispBuffer1 ? dispBuffer2 : dispBuffer1);
}

void ILI9488_Pixel(uint16_t x, uint16_t y, uint16_t color)
{
	if ((x >= ili9488_width) || (y >= ili9488_height))
		return;
	ILI9488_FillArea(x, y, 1, 1, color);
}

void ILI9488_DrawBorder(int16_t x, int16_t y, int16_t w, int16_t h, int16_t t,
		uint16_t color)
{
	ILI9488_FillArea(x, y, w, t, color);
	ILI9488_FillArea(x, y + h - t, w, t, color);
	ILI9488_FillArea(x, y, t, h, color);
	ILI9488_FillArea(x + w - t, y, t, h, color);
}

void ILI9488_FillScreen(uint16_t bgColor)
{
	ILI9488_FillArea(0, 0, ili9488_width, ili9488_height, bgColor);
}

void ILI9488_DrawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color)
{
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;

	ILI9488_Pixel(x0, y0 + r, color);
	ILI9488_Pixel(x0, y0 - r, color);
	ILI9488_Pixel(x0 + r, y0, color);
	ILI9488_Pixel(x0 - r, y0, color);

	while (x < y)
	{
		if (f >= 0)
		{
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;

		ILI9488_Pixel(x0 + x, y0 + y, color);
		ILI9488_Pixel(x0 - x, y0 + y, color);
		ILI9488_Pixel(x0 + x, y0 - y, color);
		ILI9488_Pixel(x0 - x, y0 - y, color);
		ILI9488_Pixel(x0 + y, y0 + x, color);
		ILI9488_Pixel(x0 - y, y0 + x, color);
		ILI9488_Pixel(x0 + y, y0 - x, color);
		ILI9488_Pixel(x0 - y, y0 - x, color);
	}
}

void ILI9488_FillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color)
{
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;

	ILI9488_FillArea(x0, y0 - r, 1, 2 * r, color);

	while (x < y)
	{
		if (f >= 0)
		{
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;

		if (3 & 0x1)
		{
			ILI9488_FillArea(x0 + x, y0 - y, 1, 2 * y + 1, color);
			ILI9488_FillArea(x0 + y, y0 - x, 1, 2 * x + 1, color);
		}
		if (3 & 0x2)
		{
			ILI9488_FillArea(x0 - x, y0 - y, 1, 2 * y + 1, color);
			ILI9488_FillArea(x0 - y, y0 - x, 1, 2 * x + 1, color);
		}
	}
}

void ILI9488_DrawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
		uint8_t *data, uint32_t size)
{
	ILI9488_SetAddressWindow(x, y, w + x - 1, h + y - 1);
	ILI9488_WriteData(data, size);
}

/***********************
 * @brief	display one character on the display
 * @param 	x,y: top left corner of the character to be printed
 * 			ch, font, color, bgcolor: as per parameter name
 * 			size: (1 or 2) single or double wided printing
 **********************/
void ILI9488_WChar(uint16_t x, uint16_t y, char ch, sFONT font, uint8_t size,
		uint16_t color, uint16_t bgcolor)
{
	uint32_t i, b, bytes, j, bufSize, mask;

	const uint8_t *pos;
	uint8_t wsize = font.Width;

	if (size == 2)
		wsize <<= 1;
	bufSize = 0;
	bytes = font.Height * font.Size;
	pos = font.table + (ch - 32) * bytes;
	switch (font.Size)
	{
	case 3:
		mask = 0x800000;
		break;
	case 2:
		mask = 0x8000;
		break;
	default:
		mask = 0x80;
	}

	uint8_t Rcol = (color & 0xF800) >> 8;
	uint8_t Gcol = (color & 0x07E0) >> 3;
	uint8_t Bcol = (color & 0x001F) << 3;
	uint8_t Rbak = (bgcolor & 0xF800) >> 8;
	uint8_t Gbak = (bgcolor & 0x07E0) >> 3;
	uint8_t Bbak = (bgcolor & 0x001F) << 3;

	for (i = 0; i < (bytes); i += font.Size)
	{
		b = 0;
		switch (font.Size)
		{
		case 3:
			b = pos[i] << 16 | pos[i + 1] << 8 | pos[i + 2];
			break;
		case 2:
			b = pos[i] << 8 | pos[i + 1];
			break;
		default:
			b = pos[i];
		}

		for (j = 0; j < font.Width; j++)
		{
			if ((b << j) & mask)
			{
				dispBuffer[bufSize++] = Rcol;
				dispBuffer[bufSize++] = Gcol;
				dispBuffer[bufSize++] = Bcol;

				if (size == 2)
				{
					dispBuffer[bufSize++] = Rcol;
					dispBuffer[bufSize++] = Gcol;
					dispBuffer[bufSize++] = Bcol;
				}
			}
			else
			{
				dispBuffer[bufSize++] = Rbak;
				dispBuffer[bufSize++] = Gbak;
				dispBuffer[bufSize++] = Bbak;
				if (size == 2)
				{
					dispBuffer[bufSize++] = Rbak;
					dispBuffer[bufSize++] = Gbak;
					dispBuffer[bufSize++] = Bbak;
				}
			}
		}
	}

	ILI9488_SetAddressWindow(x, y, x + wsize - 1, y + font.Height - 1);
	ILI9488_WriteData(dispBuffer, bufSize);
	dispBuffer = (dispBuffer == dispBuffer1 ? dispBuffer2 : dispBuffer1); // swapping buffer

}

/************************
 * @brief	print a string on display starting from a defined position
 * @params	x, y	top left area-to-print corner
 * 			str		string to print
 * 			font	to bu used
 * 			size	1 (normal), 2 (double width)
 * 			color	font color
 * 			bgcolor	background color
 ************************/
void ILI9488_WString(uint16_t x, uint16_t y, const char *str, sFONT font,
		uint8_t size, uint16_t color, uint16_t bgcolor)
{
	uint16_t delta = font.Width;
	if (size > 1)
		delta <<= 1;

	while (*str)
	{
		/*
		 *  these rows split oversize string in more screen lines
		 if(x + font.Width >= _width) {
		 x = 0;
		 y += font.Height;
		 if(y + font.Height >= _height) {
		 break;
		 }
		 if(*str == ' ') {
		 // skip spaces in the beginning of the new line
		 str++;
		 continue;
		 }
		 }
		 */
		ILI9488_WChar(x, y, *str, font, size, color, bgcolor);
		x += delta;
		str++;
	}
}

void ILI9488_CString(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
		const char *str, sFONT font, uint8_t size, uint16_t color,
		uint16_t bgcolor)
{
	uint16_t x, y;
	uint16_t wsize = font.Width;
	if (size > 1)
		wsize <<= 1;
	if ((strlen(str) * wsize) > (x1 - x0 + 1))
		x = x0;
	else
		x = (x1 + x0 + 1 - strlen(str) * wsize) >> 1;
	if (font.Height > (y1 - y0 + 1))
		y = y0;
	else
		y = (y1 + y0 + 1 - font.Height) >> 1;

	if (x > x0)
	{
		ILI9488_FillArea(x0, y0, x - x0, y1 - y0 + 1, bgcolor);
	}
	else
		x = x0; // fixing here mistake could be due to roundings: x lower than x0.
	if (x1 > (strlen(str) * wsize + x0))
		ILI9488_FillArea(x1 - x + x0 - 1, y0, x - x0 + 1, y1 - y0 + 1, bgcolor);

	if (y > y0)
	{
		ILI9488_FillArea(x0, y0, x1 - x0 + 1, y - y0, bgcolor);
	}
	else
		y = y0; //same comment as above
	if (y1 >= (font.Height + y0))
		ILI9488_FillArea(x0, y1 - y + y0, x1 - x0 + 1, y - y0 + 1, bgcolor);

	ILI9488_WString(x, y, str, font, size, color, bgcolor);
}

static void ILI9488_Transmit(uint8_t *data, uint16_t dataSize, bool command)
{

	while (HAL_SPI_GetState(config.spi) != HAL_SPI_STATE_READY)
	{
	};

	HAL_GPIO_WritePin(config.dc_port, config.dc_pin,
			command ? GPIO_PIN_RESET : GPIO_PIN_SET);

	if (dataSize < ILI9488_DMA_CUTOFF)
	{
		HAL_SPI_Transmit(config.spi, data, dataSize, HAL_MAX_DELAY);
	}
	else
	{
		HAL_SPI_Transmit_DMA(config.spi, data, dataSize);
	}
}

static void ILI9488_WriteCommand(uint8_t cmd)
{
	ILI9488_Transmit(&cmd, sizeof(cmd), true);
}

static void ILI9488_WriteData(uint8_t *data, size_t size)
{
	ILI9488_Transmit(data, size, false);
}

static void ILI9488_Reset()
{
	HAL_GPIO_WritePin(config.rst_port, config.rst_pin, GPIO_PIN_RESET);
	HAL_Delay(10);
	HAL_GPIO_WritePin(config.rst_port, config.rst_pin, GPIO_PIN_SET);
	HAL_Delay(150);
}


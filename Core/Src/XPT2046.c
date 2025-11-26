#include <stdint.h>
#include <stdbool.h>

#include "XPT2046.h"
#include "ILI9488.h"
#include "stm32f4xx_hal.h"

#define X_AXIS		0xD0
#define Y_AXIS		0x90
#define Z_AXIS		0xB0

#define X_THRESHOLD		0x0500
#define Z_THRESHOLD		0x0500

#define AX -0.0104f
#define BX 328.4327f
#define AY 0.0159f
#define BY -20.9882f

extern ILI9488_Orientation_e orientation_cur;
static XPT2046_Config_t config;
static volatile bool touched = false;

static uint16_t XPT2046_PollAxis(uint8_t axis);

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	if (GPIO_Pin == config.int_pin) {
		if (NVIC_GetEnableIRQ(config.int_irq)) {
			touched = true;
		}
	}
}

void XPT2046_Init(XPT2046_Config_t conf) {
	config = conf;
}

bool XPT2046_PollTouch() {
	const uint8_t pollingLevel = 4;
	uint8_t k;
	uint32_t touch;

	touch = 0;
	for (k = 0; k < (1 << pollingLevel); k++)
		touch += XPT2046_PollAxis(Z_AXIS);
	touch >>= pollingLevel;

	return (touch > Z_THRESHOLD);
}

bool XPT2046_GetTouchPosition(uint16_t *x, uint16_t *y) {

	const uint8_t pollingLevel = 4;
	uint8_t k;
	uint32_t touchx, touchy, touch;

	touch = 0;
	for (k = 0; k < (1 << pollingLevel); k++)
		touch += XPT2046_PollAxis(Z_AXIS);
	touch >>= pollingLevel;
	if (touch <= Z_THRESHOLD) {
		return false;
	}

	touch = 0;
	for (k = 0; k < (1 << pollingLevel); k++)
		touch += XPT2046_PollAxis(X_AXIS);
	touch >>= pollingLevel;
	if (touch <= X_THRESHOLD) {
		return false;
	}
	touchx = (AX * touch + BX);

	touch = 0;
	for (k = 0; k < (1 << pollingLevel); k++)
		touch += XPT2046_PollAxis(Y_AXIS);
	touch >>= pollingLevel;

	touchy = (AY * touch + BY);

	switch (orientation_cur) {
	case ILI9488_Orientation_0:
		*x = touchx;
		*y = touchy;
		break;
	case ILI9488_Orientation_90:
		*x = touchy;
		*y = (ILI9488_WIDTH - touchx);
		break;
	case ILI9488_Orientation_180:
		*x = (ILI9488_WIDTH - touchx);
		*y = (ILI9488_HEIGHT - touchy);
		break;
	case ILI9488_Orientation_270:
		*x = (ILI9488_HEIGHT - touchy);
		*y = touchx;
		break;
	}

	return true;
}

bool XPT2046_WaitForTouch(uint16_t timeout) {
	uint32_t start;

	start = HAL_GetTick();
	while (!touched) {
		if ((timeout != 0) && ((HAL_GetTick() - start) > timeout)) {
			return false;
		}
	};

	touched = false;
	return true;
}

bool Touch_WaitForUntouch(uint16_t timeout) {
	uint16_t start;

	start = HAL_GetTick();
	while (1) {
		if ((timeout != 0) && ((HAL_GetTick() - start) > timeout)) {
			return false;
		}

		if (XPT2046_PollAxis(Z_AXIS) <= Z_THRESHOLD
				|| XPT2046_PollAxis(X_AXIS) <= X_THRESHOLD) {
			return true;
		}
	}
}

bool XPT2046_In_XY_area(uint16_t x1, uint16_t y1, uint16_t width, uint16_t height) {
	uint16_t x, y;

	if (!XPT2046_GetTouchPosition(&x, &y)) {
		return false;
	}

	return x >= x1 && x < x1 + width && y >= y1 && y < y1 + height;
}

bool XPT2046_GotATouch(void) {
	bool result = touched;
	touched = false;
	return result;
}

static uint16_t XPT2046_PollAxis(uint8_t axis) {
	uint8_t poll[2] = { 0, 0 };
	uint32_t poll16;

	HAL_NVIC_DisableIRQ(config.int_irq);
	HAL_NVIC_ClearPendingIRQ(config.int_irq);

	HAL_SPI_Transmit(config.spi, &axis, 1, 10);
	if (HAL_SPI_Receive(config.spi, poll, 2, 10) == HAL_OK) {
		poll16 = (poll[0] << 8) + poll[1];
	} else {
		poll16 = 0;
	}

	HAL_NVIC_EnableIRQ(config.int_irq);
	return poll16;
}


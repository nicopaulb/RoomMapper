#ifndef INC_XPT2046_H
#define INC_XPT2046_H

#include <stdbool.h>

#include "stm32f4xx_hal.h"

typedef struct
{
    SPI_HandleTypeDef *spi;
    uint16_t int_pin;
    IRQn_Type int_irq;
} XPT2046_Config_t;

void XPT2046_Init(XPT2046_Config_t config);
bool XPT2046_In_XY_area(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
bool XPT2046_GotATouch(void);
bool Touch_WaitForUntouch(uint16_t timeout);
bool XPT2046_WaitForTouch(uint16_t timeout);
bool XPT2046_PollTouch(void);
bool XPT2046_GetTouchPosition(uint16_t *x, uint16_t *y);

#endif /* INC_XPT2046_H */


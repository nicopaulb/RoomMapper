#include <stdint.h>
#include "stm32f4xx_hal.h"

uint8_t *buf;

HAL_StatusTypeDef HAL_UARTEx_ReceiveToIdle_DMA(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size)
{
    buf = pData;
    return HAL_OK;
}

HAL_StatusTypeDef HAL_UART_Transmit_DMA(UART_HandleTypeDef *huart, const uint8_t *pData, uint16_t Size)
{
    return HAL_OK;
}

HAL_StatusTypeDef HAL_UART_Abort(UART_HandleTypeDef *huart)
{
    return HAL_OK;
}

uint32_t HAL_GetTick(void)
{
    return 1234;
}

void HAL_Delay(uint32_t Delay)
{
    return;
}

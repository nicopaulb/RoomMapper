#ifndef __STM32F4XX_HAL_H
#define __STM32F4XX_HAL_H

#include <stdint.h>

typedef enum
{
    HAL_OK = 0x00U,
    HAL_ERROR = 0x01U,
    HAL_BUSY = 0x02U,
    HAL_TIMEOUT = 0x03U
} HAL_StatusTypeDef;

#define DMA_IT_HT 0
#define __HAL_DMA_DISABLE_IT(hdma, IT) ;

typedef struct
{
    uint32_t Instance;
} UART_HandleTypeDef;

HAL_StatusTypeDef HAL_UARTEx_ReceiveToIdle_DMA(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size);
HAL_StatusTypeDef HAL_UART_Transmit_DMA(UART_HandleTypeDef *huart, const uint8_t *pData, uint16_t Size);
HAL_StatusTypeDef HAL_UART_Abort(UART_HandleTypeDef *huart);
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t head);
uint32_t HAL_GetTick(void);
void HAL_Delay(uint32_t Delay);

extern uint8_t *buf;

#endif /* __STM32F4XX_HAL_H */

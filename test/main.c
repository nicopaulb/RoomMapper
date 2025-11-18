#include <stdio.h>
#include "rplidar.h"
#include "stm32f4xx_hal.h"

UART_HandleTypeDef huart1;

extern uint8_t *buf;

int main(int, char **)
{
    printf("Hello rplidar!\n");

    RPLIDAR_Init(&huart1);

    // Send info descriptor
    buf[0] = 0xA5;
    buf[1] = 0x5A;
    buf[2] = 0x14;
    buf[3] = 0x00;
    buf[4] = 0x00;
    buf[5] = 0x00;
    buf[6] = 0x04;

    // Set sub-model value in info request
    buf[7] = 0xF0;

    HAL_UARTEx_RxEventCallback(&huart1, 7 /*descriptor size*/ + 20 /*info size*/);
}

void RPLIDAR_OnDeviceInfo(rplidar_info_t *info)
{
    printf("Submodel : %hhu", info->model_sub);
}
/*
 * rplidar.h
 *
 *  Created on: Nov 17, 2025
 *      Author: nicob
 */

#ifndef INC_RPLIDAR_H_
#define INC_RPLIDAR_H_

#include <stdint.h>
#include "stm32f4xx_hal.h"

typedef struct __attribute__((packed))
{
    uint8_t model_major : 4;
    uint8_t model_sub : 4;
    uint8_t fw_minor;
    uint8_t fw_major;
    uint8_t hardware;
    uint8_t serial_nb[16];
} rplidar_info_t;

typedef struct __attribute__((packed))
{
    uint8_t status;
    uint16_t error_code;
} rplidar_health_t;

typedef struct __attribute__((packed))
{
    uint16_t tstandart;
    uint16_t texpress;
} rplidar_samplerate_t;

typedef struct __attribute__((packed))
{
    uint8_t start : 2;
    uint8_t quality : 6;
    uint16_t check : 1;
    uint16_t angle : 15;
    uint16_t distance;
} rplidar_measurement_t;

HAL_StatusTypeDef RPLIDAR_Init(UART_HandleTypeDef *huart);
void RPLIDAR_StartScan(void);
void RPLIDAR_Stop(void);
void RPLIDAR_RequestHealth(void);
void RPLIDAR_RequestDeviceInfo(void);
void RPLIDAR_OnSingleMeasurement(rplidar_measurement_t *measurement);
void RPLIDAR_OnDeviceInfo(rplidar_info_t *info);
void RPLIDAR_OnHealth(rplidar_health_t *health);
void RPLIDAR_OnSampleRate(rplidar_samplerate_t *samplerate);
void RPLIDAR_OnConfiguration(uint32_t type, uint8_t *payload, uint16_t payload_size);

#endif /* INC_RPLIDAR_H_ */

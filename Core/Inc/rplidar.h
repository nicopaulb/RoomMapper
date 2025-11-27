/*
 * rplidar.h
 *
 *  Created on: Nov 17, 2025
 *      Author: Nicolas BESNARD
 */

#ifndef INC_RPLIDAR_H_
#define INC_RPLIDAR_H_

#include <stdbool.h>
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
    uint32_t type;
    uint8_t payload_size;
    uint8_t *payload;
} rplidar_configuration_t;

typedef struct __attribute__((packed))
{
    uint8_t start : 2;
    uint8_t quality : 6;
    uint16_t check : 1;
    uint16_t angle : 15;
    uint16_t distance;
} rplidar_measurement_t;

typedef struct __attribute__((packed))
{
    uint8_t checksum1 : 4;
    uint8_t sync1 : 4;
    uint8_t checksum2 : 4;
    uint8_t sync2 : 4;
    uint16_t angle : 15;
    uint16_t start : 1;
    uint16_t distance[40];
} rplidar_dense_measurements_t;

bool RPLIDAR_Init(UART_HandleTypeDef *huart);
bool RPLIDAR_StartScan(void);
bool RPLIDAR_StartScanExpress(void);
bool RPLIDAR_StopScan(void);
bool RPLIDAR_Reset(void);
bool RPLIDAR_RequestHealth(rplidar_health_t *health, uint32_t timeout);
bool RPLIDAR_RequestDeviceInfo(rplidar_info_t *info, uint32_t timeout);
bool RPLIDAR_RequestSampleRate(rplidar_samplerate_t *samplerate, uint32_t timeout);
void RPLIDAR_OnSingleMeasurement(rplidar_measurement_t *measurement);
void RPLIDAR_OnDenseMeasurements(rplidar_dense_measurements_t *measurement);
void RPLIDAR_OnDeviceInfo(rplidar_info_t *info);
void RPLIDAR_OnHealth(rplidar_health_t *health);
void RPLIDAR_OnSampleRate(rplidar_samplerate_t *samplerate);
void RPLIDAR_OnConfiguration(rplidar_configuration_t *config);

#endif /* INC_RPLIDAR_H_ */

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
    uint8_t model_sub :4;
    uint8_t model_major :4;
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
    uint8_t start :2;
    uint8_t quality :6;
    uint16_t check :1;
    uint16_t angle :15;
    uint16_t distance;
} rplidar_measurement_t;

typedef struct __attribute__((packed))
{
    uint8_t checksum1 :4;
    uint8_t sync1 :4;
    uint8_t checksum2 :4;
    uint8_t sync2 :4;
    uint16_t angle :15;
    uint16_t start :1;
    uint16_t distance[40];
} rplidar_dense_measurements_t;

/**
 * @brief Initialize communication with the RPLIDAR device.
 * @param huart Pointer to the UART handle.
 * @return True if initialization is successful, false otherwise.
 *
 * This function sets up the necessary UART configurations for communication with the RPLIDAR and check its health status.
 * If needed, it resets the RPLIDAR and can takes up to 1000 ms before returning.
 */
bool RPLIDAR_Init(UART_HandleTypeDef *huart);

/**
 * @brief Start scanning in blocking or non-blocking mode.
 * @param measurement User buffer where the measurements will be stored.
 * @param count Number of measurements to take.
 * @param timeout Timeout in milliseconds.
 * @return In blocking mode, return true if all measurements have been acquired else, in non-blocking mode,
 * return true if scanning has started.
 *
 * If a measurements buffer is provided, the function will return after the specified number of measurements
 * have been acquired or after the given timeout.
 * If a NULL pointer is provided, the function will return immediately and the `RPLIDAR_OnSingleMeasurement`
 * callback will be called for each new measurement.
 */
bool RPLIDAR_StartScan(rplidar_measurement_t measurement[], uint32_t count, uint32_t timeout);

/**
 * @brief Start express scanning in blocking or non-blocking mode.
 * @param measurements User buffer where the measurements will be stored.
 * @param count Number of measurements to take.
 * @param timeout Timeout in milliseconds.
 * @return In blocking mode, return true if all measurements have been acquired else, in non-blocking mode,
 * return true if scanning has started.
 *
 * If a measurements buffer is provided, the function will return after the specified number of measurements
 * have been acquired or after the given timeout.
 * If a NULL pointer is provided, the function will return immediately and the `RPLIDAR_OnDenseMeasurements`
 * callback will be called for each new measurement.
 */
bool RPLIDAR_StartScanExpress(rplidar_dense_measurements_t *measurements, uint32_t count, uint32_t timeout);

/**
 * @brief Stop scanning.
 * @return True if scanning is stopped successfully, false otherwise.
 *
 * This function stops the scanning process of the RPLIDAR device.
 */
bool RPLIDAR_StopScan(void);

/**
 * @brief Reset the RPLIDAR device.
 * @return True if the reset is successful, false otherwise.
 *
 * This function resets the RPLIDAR device.
 */
bool RPLIDAR_Reset(void);

/**
 * @brief Set the motor speed.
 * @param rpm Real-time motor speed.
 * @return True if the command is successful, false otherwise.
 *
 * This function set the speed of the RPLIDAR motor.
 */
bool RPLIDAR_SetMotorSpeed(uint16_t rpm);

/**
 * @brief Request the health status in blocking or non-blocking mode.
 * @param health User buffer where the `rplidar_health_t` response will be stored.
 * @param timeout Timeout in milliseconds.
 * @return In blocking mode, return true if a response have been received else, in non-blocking mode,
 * return true if request has been sent.
 *
 * If a user buffer is provided, the function will return after the response has been received or
 * after the given timeout.
 * If a NULL pointer is provided, the function will return immediately and the `RPLIDAR_OnHealth`
 * callback will be called with the response.
 */
bool RPLIDAR_RequestHealth(rplidar_health_t *health, uint32_t timeout);

/**
 * @brief Request device information in blocking or non-blocking mode.
 * @param info User buffer where the `rplidar_info_t` response will be stored.
 * @param timeout Timeout in milliseconds.
 * @return In blocking mode, return true if a response have been received else, in non-blocking mode,
 * return true if request has been sent.
 *
 * If a user buffer is provided, the function will return after the response has been received or
 * after the given timeout.
 * If a NULL pointer is provided, the function will return immediately and the `RPLIDAR_OnDeviceInfo`
 * callback will be called with the response.
 */
bool RPLIDAR_RequestDeviceInfo(rplidar_info_t *info, uint32_t timeout);

/**
 * @brief Request current configured sample rate in blocking or non-blocking mode.
 * @param samplerate User buffer where the `rplidar_samplerate_t` response will be stored.
 * @param timeout Timeout in milliseconds.
 * @return In blocking mode, return true if a response have been received else, in non-blocking mode,
 * return true if request has been sent.
 *
 * If a user buffer is provided, the function will return after the response has been received or
 * after the given timeout.
 * If a NULL pointer is provided, the function will return immediately and the `RPLIDAR_OnSampleRate`
 * callback will be called with the response.
 */
bool RPLIDAR_RequestSampleRate(rplidar_samplerate_t *samplerate, uint32_t timeout);

/**
 * @brief Request configuration in blocking or non-blocking mode.
 * @param type Configuration option to read.
 * @param payload Configuration payload to send.
 * @param payload_size Size of the payload.
 * @param config User buffer where the `rplidar_configuration_t` response will be stored.
 * @param timeout Timeout in milliseconds.
 * @return In blocking mode, return true if a response have been received else, in non-blocking mode,
 * return true if request has been sent.
 *
 * If a user buffer is provided, the function will return after the response has been received or
 * after the given timeout.
 * If a NULL pointer is provided, the function will return immediately and the `RPLIDAR_OnConfiguration`
 * callback will be called with the response.
 */
bool RPLIDAR_RequestConfiguration(uint32_t type, uint8_t *payload, uint16_t payload_size,
                                  rplidar_configuration_t *config, uint32_t timeout);

/**
 * @brief Callback called when a legacy measurement is received.
 * @param measurement Measurement made by the RPLIDAR.
 *
 * Measurement distance field should be divided by 4.0 to get the real distance in mm.
 * Measurement angle field should be divided by 64.0 to get the real angle in °.
 */
void RPLIDAR_OnSingleMeasurement(rplidar_measurement_t *measurement);
/**
 * @brief Callback called when a dense measurement is received.
 * @param measurement Measurement made by the RPLIDAR.
 *
 * Measurement distance field should be divided by 4.0 to get the real distance in mm.
 * Measurement angle field should be divided by 64.0 to get the real angle in °.
 */
void RPLIDAR_OnDenseMeasurements(rplidar_dense_measurements_t *measurement);
void RPLIDAR_OnDeviceInfo(rplidar_info_t *info);
void RPLIDAR_OnHealth(rplidar_health_t *health);
void RPLIDAR_OnSampleRate(rplidar_samplerate_t *samplerate);
void RPLIDAR_OnConfiguration(rplidar_configuration_t *config);

#endif /* INC_RPLIDAR_H_ */

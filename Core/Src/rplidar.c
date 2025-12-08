/*
 * rplidar.c
 *
 *  Created on: Nov 17, 2025
 *      Author: Nicolas BESNARD
 */

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "stm32f4xx_hal.h"
#include "rplidar.h"

#define START_FLAG 0xA5

#define REQ_SCAN 0x20
#define REQ_STOP 0x25
#define REQ_RESET 0x40
#define REQ_INFO 0x50
#define REQ_HEALTH 0x52
#define REQ_SAMPLERATE 0x59
#define REQ_SCAN_EXPR 0x82
#define REQ_CONF 0x84
#define REQ_MOTOR 0xA8

#define RESP_MODE_SINGLE 0x00
#define RESP_MODE_MULTI 0x01
#define RESP_FLAG 0x5A
#define RESP_INFO 0x04
#define RESP_HEALTH 0x06
#define RESP_SAMPLERATE 0x15
#define RESP_CONF 0x20
#define RESP_SCAN 0x81
#define RESP_SCAN_EXPR 0x85

#define BUFFER_RX_SIZE 4096
#define BUFFER_RESP_SIZE 128
#define BUFFER_DESC_SIZE 7

#define REQ_CONF_PAYLOAD_MAX 16

typedef enum parser_state
{
    PARSER_DESCRIPTOR, PARSER_RESPONSE_SINGLE, PARSER_RESPONSE_MULTI, PARSER_ERROR
} parser_state_t;

typedef enum response_type
{
    RESPONSE_UNKNOWN,
    RESPONSE_INFO,
    RESPONSE_HEALTH,
    RESPONSE_SAMPLERATE,
    RESPONSE_CONF,
    RESPONSE_SCAN,
    RESPONSE_SCAN_EXPRESS
} response_type_t;

static UART_HandleTypeDef *rpl_uart;
static uint8_t rpl_rx_buf[BUFFER_RX_SIZE] __attribute__((aligned(4))); // 32-bits aligned for DMA
static uint16_t rpl_rx_tail = 0;
static uint8_t rpl_resp_buf[BUFFER_RESP_SIZE];
static uint8_t rpl_resp_len = 0;
static response_type_t rpl_resp_type = RESPONSE_UNKNOWN;
static parser_state_t rpl_parser_state = PARSER_DESCRIPTOR;
static parser_state_t rpl_last_complete_resp = RESPONSE_UNKNOWN;
static uint8_t *rpl_usr_buf = NULL;
static uint32_t rpl_usr_buf_idx = 0;
static uint32_t rpl_multiresp_remaining = 0;

static void _ParseRX(uint8_t *data, uint16_t len);
static parser_state_t _ParseDescriptor(uint8_t *buf);
static response_type_t _ParseRspType(uint8_t type);
static bool _ParseResponse(uint8_t *response, uint16_t size);
static bool _SendRequest(uint8_t *data, uint16_t size, bool resp);
static uint8_t _ComputeChecksum(uint8_t *data, uint16_t size);
static void _ResetParser(void);
static bool _WaitForResponse(response_type_t type, uint32_t timeout);
static bool _WaitForMultiResponse(response_type_t type, uint32_t timeout);

bool RPLIDAR_Init(UART_HandleTypeDef *huart)
{
    rplidar_health_t health;

    if (!huart)
    {
        return false;
    }

    rpl_uart = huart;

    if (!RPLIDAR_RequestHealth(&health, 1000) || health.status != 0)
    {
        // Reset to clear errors
        return RPLIDAR_Reset();
    }
    else
    {
        return true;
    }
}

bool RPLIDAR_StartScan(rplidar_measurement_t measurement[], uint32_t count, uint32_t timeout)
{
    uint8_t packet[2] = {START_FLAG, REQ_SCAN};

    // Use buffer provided by user to store response
    // If null then use internal buffer and callback
    rpl_usr_buf = (uint8_t*) measurement;
    rpl_multiresp_remaining = count;

    if (!_SendRequest(packet, sizeof(packet), true))
    {
        return false;
    }

    // If user didn't provide a buffer, return immediatly and the callback will be called
    return rpl_usr_buf != NULL ? _WaitForMultiResponse(RESPONSE_SCAN, timeout) : true;
}

bool RPLIDAR_StartScanExpress(rplidar_dense_measurements_t *measurements, uint32_t count, uint32_t timeout)
{
    uint8_t packet[9] = {START_FLAG, REQ_SCAN_EXPR, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x22};
    packet[8] = _ComputeChecksum(packet, 8);

    // Use buffer provided by user to store response
    // If null then use internal buffer and callback
    rpl_usr_buf = (uint8_t*) measurements;
    rpl_multiresp_remaining = count;

    if (!_SendRequest(packet, sizeof(packet), true))
    {
        return false;
    }

    // If user didn't provide a buffer, return immediatly and the callback will be called
    return rpl_usr_buf != NULL ? _WaitForMultiResponse(RESPONSE_SCAN_EXPRESS, timeout) : true;
}

bool RPLIDAR_StopScan(void)
{
    uint8_t packet[2] = {START_FLAG, REQ_STOP};
    return _SendRequest(packet, sizeof(packet), false);
}

bool RPLIDAR_Reset(void)
{
    uint8_t packet[2] = {START_FLAG, REQ_RESET};

    _ResetParser();

    if (_SendRequest(packet, sizeof(packet), false))
    {
        HAL_Delay(1000);
        return true;
    }
    return false;
}

bool RPLIDAR_SetMotorSpeed(uint16_t rpm)
{
    uint8_t packet[6];
    packet[0] = START_FLAG;
    packet[1] = REQ_MOTOR;
    packet[2] = 0x02;
    packet[3] = (rpm >> 8) & 0xFF;
    packet[4] = rpm & 0xFF;
    packet[5] = _ComputeChecksum(packet, 5);

    return _SendRequest(packet, sizeof(packet), false);
}

bool RPLIDAR_RequestDeviceInfo(rplidar_info_t *info, uint32_t timeout)
{
    uint8_t packet[2] = {START_FLAG, REQ_INFO};

    // Use buffer provided by user to store response
    // If null then use internal buffer and callback
    rpl_usr_buf = (uint8_t*) info;

    if (!_SendRequest(packet, sizeof(packet), true))
    {
        return false;
    }

    // If user didn't provide a buffer, return immediatly and the callback will be called
    return rpl_usr_buf != NULL ? _WaitForResponse(RESPONSE_INFO, timeout) : true;
}

bool RPLIDAR_RequestHealth(rplidar_health_t *health, uint32_t timeout)
{
    uint8_t packet[2] = {START_FLAG, REQ_HEALTH};

    // Use buffer provided by user to store response
    // If null then use internal buffer and callback
    rpl_usr_buf = (uint8_t*) health;

    if (!_SendRequest(packet, sizeof(packet), true))
    {
        return false;
    }

    // If user didn't provide a buffer, return immediatly and the callback will be called
    return rpl_usr_buf != NULL ? _WaitForResponse(RESPONSE_HEALTH, timeout) : true;
}

bool RPLIDAR_RequestSampleRate(rplidar_samplerate_t *samplerate, uint32_t timeout)
{
    uint8_t packet[2] = {START_FLAG, REQ_SAMPLERATE};

    // Use buffer provided by user to store response
    // If null then use internal buffer and callback
    rpl_usr_buf = (uint8_t*) samplerate;

    if (!_SendRequest(packet, sizeof(packet), true))
    {
        return false;
    }

    // If user didn't provide a buffer, return immediatly and the callback will be called
    return rpl_usr_buf != NULL ? _WaitForResponse(RESPONSE_SAMPLERATE, timeout) : true;
}

bool RPLIDAR_RequestConfiguration(uint32_t type, uint8_t *payload, uint16_t payload_size,
                                  rplidar_configuration_t *config, uint32_t timeout)
{
    uint8_t packet[REQ_CONF_PAYLOAD_MAX + 8];
    if (payload_size > REQ_CONF_PAYLOAD_MAX)
    {
        return false;
    }

    packet[0] = START_FLAG;
    packet[1] = REQ_CONF;
    packet[2] = payload_size + 1;
    packet[3] = (type >> 24) & 0xFF;
    packet[4] = (type >> 16) & 0xFF;
    packet[5] = (type >> 8) & 0xFF;
    packet[6] = type & 0xFF;
    memcpy(&packet[7], payload, payload_size);
    packet[payload_size + 7] = _ComputeChecksum(packet, payload_size + 7);

    // Use buffer provided by user to store response
    // If null then use internal buffer and callback
    rpl_usr_buf = (uint8_t*) config;

    if (!_SendRequest(packet, sizeof(packet), true))
    {
        return false;
    }

    // If user didn't provide a buffer, return immediatly and the callback will be called
    return rpl_usr_buf != NULL ? _WaitForResponse(RESPONSE_CONF, timeout) : true;
}

__attribute__((weak)) void RPLIDAR_OnDeviceInfo(rplidar_info_t *info)
{
    return;
}

__attribute__((weak)) void RPLIDAR_OnHealth(rplidar_health_t *health)
{
    return;
}

__attribute__((weak)) void RPLIDAR_OnSampleRate(rplidar_samplerate_t *samplerate)
{
    return;
}

__attribute__((weak)) void RPLIDAR_OnConfiguration(rplidar_configuration_t *config)
{
    return;
}

__attribute__((weak)) void RPLIDAR_OnSingleMeasurement(rplidar_measurement_t *measurement)
{
    return;
}

__attribute__((weak)) void RPLIDAR_OnDenseMeasurements(rplidar_dense_measurements_t *measurement)
{
    return;
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t head)
{
    if (huart->Instance == rpl_uart->Instance)
    {
        if (head > rpl_rx_tail)
        {
            _ParseRX(&rpl_rx_buf[rpl_rx_tail], head - rpl_rx_tail);
        }
        else
        {
            // Buffer wrapped around
            _ParseRX(&rpl_rx_buf[rpl_rx_tail], BUFFER_RX_SIZE - rpl_rx_tail);
            _ParseRX(&rpl_rx_buf[0], head);
        }
        rpl_rx_tail = head;
    }
}

static void _ParseRX(uint8_t *data, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++)
    {
        switch (rpl_parser_state)
        {
            case PARSER_DESCRIPTOR:
                {
                    static uint8_t rpl_desc_buf[BUFFER_DESC_SIZE];
                    static uint16_t rpl_desc_idx = 0;
                    rpl_desc_buf[rpl_desc_idx++] = data[i];
                    if (rpl_desc_idx == BUFFER_DESC_SIZE)
                    {
                        // Entire descriptor (7 bytes) received
                        rpl_parser_state = _ParseDescriptor(rpl_desc_buf);
                        rpl_desc_idx = 0;
                    }
                }
                break;
            case PARSER_RESPONSE_SINGLE:
            case PARSER_RESPONSE_MULTI:
                {
                    static uint16_t rpl_resp_idx = 0;
                    rpl_resp_buf[rpl_resp_idx++] = data[i];
                    if (rpl_resp_idx == rpl_resp_len)
                    {
                        // Entire response received
                        if (_ParseResponse(rpl_resp_buf, rpl_resp_len))
                        {
                            // Parsing complete
                            if (rpl_parser_state == PARSER_RESPONSE_SINGLE)
                            {
                                // Single response was expected, ready for new descriptor
                                rpl_parser_state = PARSER_DESCRIPTOR;
                            }
                            else
                            {
                                // Multi response expected, ready for next responses
                            }
                        }
                        else
                        {
                            rpl_parser_state = PARSER_ERROR;
                        }
                        // Reset response buffer
                        rpl_resp_idx = 0;
                    }
                }
                break;

                break;
            case PARSER_ERROR:
                // TODO Handle parsing error. Maybe reset RPLIDAR ?
                return;
        }
    }
}

static bool _ParseResponse(uint8_t *response, uint16_t size)
{
    switch (rpl_resp_type)
    {
        case RESPONSE_INFO:
            if (size == sizeof(rplidar_info_t))
            {
                rplidar_info_t *info = (rplidar_info_t*) response;
                if (rpl_usr_buf != NULL)
                {
                    // Use user buffer
                    memcpy(rpl_usr_buf, info, sizeof(rplidar_info_t));
                    rpl_last_complete_resp = RESPONSE_INFO;
                }
                else
                {
                    // Use callback
                    RPLIDAR_OnDeviceInfo(info);
                }
                return true;
            }
            break;
        case RESPONSE_HEALTH:
            if (size == sizeof(rplidar_health_t))
            {
                rplidar_health_t *health = (rplidar_health_t*) response;
                if (rpl_usr_buf != NULL)
                {
                    // Use user buffer
                    memcpy(rpl_usr_buf, health, sizeof(rplidar_health_t));
                    rpl_last_complete_resp = RESPONSE_HEALTH;
                }
                else
                {
                    // Use callback
                    RPLIDAR_OnHealth(health);
                }
                return true;
            }
            break;
        case RESPONSE_SAMPLERATE:
            if (size == sizeof(rplidar_samplerate_t))
            {
                rplidar_samplerate_t *samplerate = (rplidar_samplerate_t*) response;
                if (rpl_usr_buf != NULL)
                {
                    // Use user buffer
                    memcpy(rpl_usr_buf, samplerate, sizeof(rplidar_samplerate_t));
                    rpl_last_complete_resp = RESPONSE_SAMPLERATE;
                }
                else
                {
                    // Use callback
                    RPLIDAR_OnSampleRate(samplerate);
                }
                return true;
            }
            break;
        case RESPONSE_CONF:
            {
                rplidar_configuration_t config = {.type = response[3] << 24 | response[2] << 16 | response[1] << 8
                        | response[0], .payload_size = size - 4, .payload = response + 4, };
                if (rpl_usr_buf != NULL)
                {
                    // Use user buffer
                    memcpy(rpl_usr_buf, &config, sizeof(rplidar_configuration_t));
                    // Copy payload to end of user buffer
                    memcpy(((rplidar_configuration_t*) rpl_usr_buf)->payload, response + 4, size - 4);
                    rpl_last_complete_resp = RESPONSE_SAMPLERATE;
                }
                else
                {
                    // Use callback
                    RPLIDAR_OnConfiguration(&config);
                }
                return true;
            }
        case RESPONSE_SCAN:
            if (size == sizeof(rplidar_measurement_t))
            {
                rplidar_measurement_t *measurement = (rplidar_measurement_t*) response;
                if (measurement->check != 1)
                {
                    return false;
                }

                if (rpl_usr_buf != NULL)
                {
                    // Use user buffer
                    static uint32_t rpl_usr_buf_idx = 0;
                    if (rpl_multiresp_remaining > 0)
                    {
                        // Do not copy more than the needed number of measurements to prevent overflowing the user buffer
                        memcpy(rpl_usr_buf + (sizeof(rplidar_measurement_t) * rpl_usr_buf_idx++), measurement,
                               sizeof(rplidar_measurement_t));
                        rpl_multiresp_remaining--;
                        rpl_last_complete_resp = RESPONSE_SCAN;
                    }
                }
                else
                {
                    // Use callback
                    RPLIDAR_OnSingleMeasurement(measurement);
                }
                return true;
            }
            break;
        case RESPONSE_SCAN_EXPRESS:
            if (size == sizeof(rplidar_dense_measurements_t))
            {
                rplidar_dense_measurements_t *measurements = (rplidar_dense_measurements_t*) response;
                if (rpl_usr_buf != NULL)
                {
                    // Use user buffer
                    if (rpl_multiresp_remaining > 0)
                    {
                        // Do not copy more than the needed number of measurements to prevent overflowing the user buffer
                        memcpy(rpl_usr_buf + (sizeof(rplidar_dense_measurements_t) * rpl_usr_buf_idx++), measurements,
                               sizeof(rplidar_dense_measurements_t));
                        rpl_multiresp_remaining--;
                        rpl_last_complete_resp = RESPONSE_SCAN_EXPRESS;
                    }
                }
                else
                {
                    // Use callback
                    RPLIDAR_OnDenseMeasurements(measurements);
                }
                return true;
            }
            break;

        default:
            break;
    }

    return false;
}

static response_type_t _ParseRspType(uint8_t type)
{
    switch (type)
    {
        case RESP_INFO:
            return RESPONSE_INFO;
        case RESP_HEALTH:
            return RESPONSE_HEALTH;
        case RESP_SAMPLERATE:
            return RESPONSE_SAMPLERATE;
        case RESP_CONF:
            return RESPONSE_CONF;
        case RESP_SCAN:
            return RESPONSE_SCAN;
        case RESP_SCAN_EXPR:
            return RESPONSE_SCAN_EXPRESS;
        default:
            return RESPONSE_UNKNOWN;
    }
}

static parser_state_t _ParseDescriptor(uint8_t *buf)
{
    typedef struct __attribute__((packed))
    {
        uint8_t flags[2];
        uint32_t len :30;
        uint32_t mode :2;
        uint8_t type;
    } descriptor_t;

    descriptor_t *descriptor = (descriptor_t*) buf;

    if (descriptor->flags[0] != START_FLAG || descriptor->flags[1] != RESP_FLAG)
    {
        return PARSER_ERROR;
    }

    rpl_resp_len = descriptor->len;
    rpl_resp_type = _ParseRspType(descriptor->type);

    switch (descriptor->mode)
    {
        case RESP_MODE_SINGLE:
            return PARSER_RESPONSE_SINGLE;
        case RESP_MODE_MULTI:
            return PARSER_RESPONSE_MULTI;
        default:
            return PARSER_ERROR;
    }
}

static bool _SendRequest(uint8_t *data, uint16_t size, bool resp)
{
    _ResetParser();

    if (resp)
    {
        // Expect a response, so enable receive DMA
        if (HAL_UARTEx_ReceiveToIdle_DMA(rpl_uart, rpl_rx_buf, BUFFER_RX_SIZE) != HAL_OK)
        {
            return false;
        }
    }

    return (HAL_UART_Transmit_DMA(rpl_uart, data, size) == HAL_OK);
}

static uint8_t _ComputeChecksum(uint8_t *data, uint16_t size)
{
    uint8_t checksum = 0;
    for (uint16_t i = 0; i < size; i++)
    {
        checksum += data[i];
    }
    return checksum;
}

static void _ResetParser(void)
{
    HAL_UART_Abort(rpl_uart);
    rpl_parser_state = PARSER_DESCRIPTOR;
    rpl_rx_tail = 0;
    rpl_resp_len = 0;
    rpl_resp_type = RESPONSE_UNKNOWN;
    rpl_last_complete_resp = RESPONSE_UNKNOWN;
    rpl_usr_buf_idx = 0;
}

static bool _WaitForResponse(response_type_t type, uint32_t timeout)
{
    uint32_t start = HAL_GetTick();
    while (rpl_resp_type != type)
    {
        if ((timeout != 0) && ((HAL_GetTick() - start) > timeout))
        {
            return false;
        }
    }

    return true;
}

static bool _WaitForMultiResponse(response_type_t type, uint32_t timeout)
{
    uint32_t start = HAL_GetTick();
    while (rpl_resp_type != type || rpl_multiresp_remaining > 0)
    {
        if ((timeout != 0) && ((HAL_GetTick() - start) > timeout))
        {
            return false;
        }
    }

    return true;
}

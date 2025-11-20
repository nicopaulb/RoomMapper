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
#define REQ_RATE 0x59
#define REQ_SCAN_EXPR 0x82
#define REQ_CONF 0x84

#define RESP_MODE_SINGLE 0x00
#define RESP_MODE_MULTI 0x01
#define RESP_FLAG 0x5A
#define RESP_INFO 0x04
#define RESP_HEALTH 0x06
#define RESP_RATE 0x15
#define RESP_CONF 0x20
#define RESP_SCAN 0x81
#define RESP_SCAN_EXPR 0x85

#define BUFFER_RX_SIZE 2048
#define BUFFER_RESP_SIZE 128
#define BUFFER_DESC_SIZE 7

#define REQ_CONF_PAYLOAD_MAX 16

typedef enum parser_state
{
	PARSER_DESCRIPTOR,
	PARSER_RESPONSE_SINGLE,
	PARSER_RESPONSE_MULTI,
	PARSER_ERROR
} parser_state_t;

typedef enum response_type
{
	RESPONSE_UNKNOWN,
	RESPONSE_INFO,
	RESPONSE_HEALTH,
	RESPONSE_RATE,
	RESPONSE_CONF,
	RESPONSE_SCAN,
	RESPONSE_SCAN_EXPRESS
} response_type_t;

static UART_HandleTypeDef *rpl_huart;

static uint8_t rpl_rx_buf[BUFFER_RX_SIZE];
static uint8_t rpl_resp_len = 0;
static response_type_t rpl_resp_type = RESPONSE_UNKNOWN;
static parser_state_t rpl_parser_state = PARSER_DESCRIPTOR;

static void _ParseRX(uint8_t *data, uint16_t len);
static parser_state_t _ParseDescriptor(uint8_t *buf);
static response_type_t _ParseRspType(uint8_t type);
static bool _ParseResponse(uint8_t *response, uint16_t size);
static bool _SendRequest(uint8_t cmd);
static uint8_t _ComputeChecksum(uint8_t *data, uint16_t size);

bool RPLIDAR_Init(UART_HandleTypeDef *huart)
{
	if (!huart)
	{
		return false;
	}

	rpl_huart = huart;

	if (HAL_UARTEx_ReceiveToIdle_DMA(rpl_huart, rpl_rx_buf, BUFFER_RX_SIZE) != HAL_OK)
	{
		return false;
	}

	// Disable half transfer IT
	__HAL_DMA_DISABLE_IT(rpl_huart->hdmarx, DMA_IT_HT);

	return true;
}

bool RPLIDAR_StartScan(void)
{
	return _SendRequest(REQ_SCAN);
}

bool RPLIDAR_StartScanExpress(void)
{
	uint8_t packet[9] = {START_FLAG, REQ_SCAN_EXPR, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x22};
	packet[8] = _ComputeChecksum(packet, 8);
	return (HAL_UART_Transmit_IT(rpl_huart, packet, sizeof(packet)) == HAL_OK);
}

bool RPLIDAR_Stop(void)
{
	return _SendRequest(REQ_STOP);
}

bool RPLIDAR_RequestDeviceInfo(void)
{
	return _SendRequest(REQ_INFO);
}

bool RPLIDAR_RequestHealth(void)
{
	return _SendRequest(REQ_HEALTH);
}

bool RPLIDAR_RequestConfiguration(uint32_t type, uint8_t *payload, uint16_t payload_size)
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
	return (HAL_UART_Transmit_IT(rpl_huart, packet, payload_size + 8) == HAL_OK);
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

__attribute__((weak)) void RPLIDAR_OnConfiguration(uint32_t type, uint8_t *payload, uint16_t payload_size)
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
	static uint16_t rpl_rx_tail = 0;
	if (huart->Instance == rpl_huart->Instance)
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
			static uint8_t rpl_resp_buf[BUFFER_RESP_SIZE];
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
			rplidar_info_t *info = (rplidar_info_t *)response;
			RPLIDAR_OnDeviceInfo(info);
			return true;
		}
		break;
	case RESPONSE_HEALTH:
		if (size == sizeof(rplidar_health_t))
		{
			rplidar_health_t *health = (rplidar_health_t *)response;
			RPLIDAR_OnHealth(health);
			return true;
		}
		break;
	case RESPONSE_RATE:
		if (size == sizeof(rplidar_samplerate_t))
		{
			rplidar_samplerate_t *samplerate = (rplidar_samplerate_t *)response;
			RPLIDAR_OnSampleRate(samplerate);
			return true;
		}
		break;
	case RESPONSE_CONF:
	{
		uint32_t type = response[3] << 24 | response[2] << 16 | response[1] << 8 | response[0];
		RPLIDAR_OnConfiguration(type, response + 4, size - 4);
		return true;
	}
	case RESPONSE_SCAN:
		if (size == sizeof(rplidar_measurement_t))
		{
			rplidar_measurement_t *measurement = (rplidar_measurement_t *)response;
			RPLIDAR_OnSingleMeasurement(measurement);
			return true;
		}
		break;
	case RESPONSE_SCAN_EXPRESS:
		if (size == sizeof(rplidar_dense_measurements_t))
		{
			rplidar_dense_measurements_t *measurement = (rplidar_dense_measurements_t *)response;
			RPLIDAR_OnDenseMeasurements(measurement);
			return true;
		}
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
	case RESP_RATE:
		return RESPONSE_RATE;
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
		uint32_t len : 30;
		uint32_t mode : 2;
		uint8_t type;
	} descriptor_t;

	descriptor_t *descriptor = (descriptor_t *)buf;

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

static bool _SendRequest(uint8_t cmd)
{
	uint8_t packet[2] = {START_FLAG, cmd};
	return (HAL_UART_Transmit_IT(rpl_huart, packet, sizeof(packet)) == HAL_OK);
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
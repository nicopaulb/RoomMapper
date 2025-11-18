/*
 * rplidar.c
 *
 *  Created on: Nov 17, 2025
 *      Author: nicob
 */

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
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

#define BUFFER_RX_SIZE 2048
#define BUFFER_RESP_SIZE 128
#define BUFFER_DESC_SIZE 7

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
	RESPONSE_SCAN
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
static void _SendRequest(uint8_t cmd);

HAL_StatusTypeDef RPLIDAR_Init(UART_HandleTypeDef *huart)
{
	if (!huart)
	{
		return HAL_ERROR;
	}

	rpl_huart = huart;

	if (HAL_UARTEx_ReceiveToIdle_DMA(rpl_huart, rpl_rx_buf, BUFFER_RX_SIZE) != HAL_OK)
	{
		return HAL_ERROR;
	}

	// Disable half transfer IT
	__HAL_DMA_DISABLE_IT(rpl_huart->hdmarx, DMA_IT_HT);

	return HAL_OK;
}

void RPLIDAR_StartScan(void)
{
	_SendRequest(REQ_SCAN);
}

void RPLIDAR_Stop(void)
{
	_SendRequest(REQ_STOP);
}

void RPLIDAR_RequestDeviceInfo(void)
{
	_SendRequest(REQ_INFO);
}

void RPLIDAR_RequestHealth(void)
{
	_SendRequest(REQ_HEALTH);
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
		printf("Response parsed\n");
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
		uint32_t type = response[0] << 24 | response[1] << 16 | response[2] << 8 | response[3];
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
	}

	printf("failed %u\n", rpl_resp_type);
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

static void _SendRequest(uint8_t cmd)
{
	uint8_t packet[2] = {START_FLAG, cmd};
	HAL_UART_Transmit_IT(rpl_huart, packet, sizeof(packet));
}

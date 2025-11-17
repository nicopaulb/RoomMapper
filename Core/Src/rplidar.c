/*
 * rplidar.c
 *
 *  Created on: Nov 17, 2025
 *      Author: nicob
 */

#include "rplidar.h"
#include "string.h"
#include "stm32f4xx_hal.h"

#define START_FLAG 0xA5

// Request flag
#define REQUEST_SCAN 0x20
#define REQUEST_STOP 0x25
#define REQUEST_RESET 0x40
#define REQUEST_INFO 0x50
#define REQUEST_HEALTH 0x52
#define REQUEST_RATE 0x59
#define REQUEST_SCAN_EXPR 0x82
#define REQUEST_CONF 0x84

static uint8_t rpl_rx_buf[2048];
static uint8_t rpl_response_buf[128];
static UART_HandleTypeDef *rpl_huart;

static void _ParseResponse(uint8_t *data, uint16_t len);
static void _ParseCabins(uint8_t *frame);
static void _SendRequest(uint8_t cmd);

HAL_StatusTypeDef RPLIDAR_Init(UART_HandleTypeDef *huart, rplidar_sample_cb_t cb, void *user_ctx) {
	if (!huart) {
		return HAL_ERROR;
	}
	rpl_huart = huart;

	if (HAL_UARTEx_ReceiveToIdle_DMA(rpl_huart, rpl_rx_buf, sizeof(rpl_rx_buf)) != HAL_OK) {
		return HAL_ERROR;
	}

	// Disable half transfer IT
	__HAL_DMA_DISABLE_IT(rpl_huart->hdmarx, DMA_IT_HT);

	return HAL_OK;
}

void RPLIDAR_StartScan(void) {
	RPLIDAR_SendCommand(REQUEST_SCAN);
}

void RPLIDAR_Stop(void) {
	RPLIDAR_SendCommand(REQUEST_STOP);
}

void RPLIDAR_RequestDeviceInfo(void) {
	RPLIDAR_SendCommand(REQUEST_INFO);
}

void RPLIDAR_RequestHealth(void) {
	RPLIDAR_SendCommand(REQUEST_HEALTH);
}

__weak void RPLIDAR_OnMeasurement(float angle_deg, float distance_mm) {
	return;
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size) {
	if (huart->Instance == rpl_huart->Instance) {
		// Copy response and restart DMA
		memcpy(rpl_response_buf, rplidar_rx_buf, size);
		HAL_UARTEx_ReceiveToIdle_DMA(rpl_huart, rpl_rx_buf, sizeof(rpl_rx_buf));

		_ParseResponse(rpl_response_buf, size);
	}
}

static void _SendRequest(uint8_t cmd) {
	uint8_t frame[2] = { START_FLAG, cmd };
	HAL_UART_Transmit_IT(rplidar_huart, frame, sizeof(frame));
}

static void _ParseResponse(uint8_t *data, uint16_t len) {
	if (len < 4)
		return;

	// Descriptor frame
	if (data[0] == 0xA5 && data[1] == 0x5A) {
		// TODO descriptor parsing
		return;
	}

	// Cabin frame
	if (len == 96) {
		_RPLIDAR_ParseCabins(data);
		return;
	}

	// Ignore other packet sizes
}

static void _ParseCabins(uint8_t *frame) {
	typedef struct __attribute__((packed)) {
		uint16_t angle_q6;   // angle × 64
		uint16_t dist_q2;    // distance × 4
	} Cabin;

	Cabin *cabins = (Cabin*) (frame + 4); // skip 4-byte header

	for (int i = 0; i < 32; i++) {
		float angle = cabins[i].angle_q6 / 64.0f;
		float dist = cabins[i].dist_q2 / 4.0f;

		RPLIDAR_OnMeasurement(angle, dist);
	}
}


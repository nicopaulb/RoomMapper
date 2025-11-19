#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include "rplidar.h"
#include "stm32f4xx_hal.h"

typedef enum
{
    None,
    cb_RPLIDAR_OnDeviceInfo,
    cb_RPLIDAR_OnHealth,
    cb_RPLIDAR_OnSamplerate,
    cb_RPLIDAR_OnConfiguration,
    cb_RPLIDAR_OnSingleMeasurement
} callback_type_t;

UART_HandleTypeDef huart1;

extern uint8_t *buf;
static uint16_t head = 0;
static callback_type_t cb_type = None;

static void test_device_info_request(void);
static void test_health_request(void);
static void test_samplerate_request(void);
static void test_configuration_request(void);
static void test_scan_request(void);

int main()
{
    printf("START TESTS\n");
    RPLIDAR_Init(&huart1);

    test_device_info_request();
    test_health_request();
    test_samplerate_request();
    test_configuration_request();
    test_scan_request();
}

static void test_device_info_request(void)
{
    printf("test_device_info_request : ");
    cb_type = None;

    // Send info descriptor
    buf[head++] = 0xA5;
    buf[head++] = 0x5A;
    buf[head++] = 0x14;
    buf[head++] = 0x00;
    buf[head++] = 0x00;
    buf[head++] = 0x00;
    buf[head++] = 0x04;
#define MODEL_MAJOR 4
#define MODEL_SUB 1
#define FW_MAJOR 2
#define FW_MINOR 3
#define HARDWARE 5
    buf[head++] = MODEL_SUB << 4 | MODEL_MAJOR;
    buf[head++] = FW_MINOR;
    buf[head++] = FW_MAJOR;
    buf[head++] = HARDWARE;
    strncpy(&buf[head], "RPLIDARC1 123456", 16);
    head += 16;
    HAL_UARTEx_RxEventCallback(&huart1, head);
    assert(cb_type == cb_RPLIDAR_OnDeviceInfo);
    printf("SUCCESS\n");
}

void RPLIDAR_OnDeviceInfo(rplidar_info_t *info)
{
    assert(info->model_major == MODEL_MAJOR);
    assert(info->model_sub == MODEL_SUB);
    assert(info->fw_major == FW_MAJOR);
    assert(info->fw_minor == FW_MINOR);
    assert(info->hardware == HARDWARE);
    cb_type = cb_RPLIDAR_OnDeviceInfo;
}

static void test_health_request(void)
{
    printf("test_health_request : ");
    cb_type = None;

    // Send info descriptor
    buf[head++] = 0xA5;
    buf[head++] = 0x5A;
    buf[head++] = 0x03;
    buf[head++] = 0x00;
    buf[head++] = 0x00;
    buf[head++] = 0x00;
    buf[head++] = 0x06;
#define STATUS 1
#define ERROR_CODE 263
    buf[head++] = STATUS;
    buf[head++] = ERROR_CODE & 0xFF;
    buf[head++] = (ERROR_CODE & 0xFF00) >> 8;
    HAL_UARTEx_RxEventCallback(&huart1, head);
    assert(cb_type == cb_RPLIDAR_OnHealth);
    printf("SUCCESS\n");
}

void RPLIDAR_OnHealth(rplidar_health_t *health)
{
    assert(health->status == STATUS);
    assert(health->error_code == ERROR_CODE);
    cb_type = cb_RPLIDAR_OnHealth;
}

static void test_samplerate_request(void)
{
    printf("test_samplerate_request : ");
    cb_type = None;

    // Send info descriptor
    buf[head++] = 0xA5;
    buf[head++] = 0x5A;
    buf[head++] = 0x04;
    buf[head++] = 0x00;
    buf[head++] = 0x00;
    buf[head++] = 0x00;
    buf[head++] = 0x15;
#define TSTANDARD 1256
#define TEXPRESS 10389
    buf[head++] = TSTANDARD & 0xFF;
    buf[head++] = (TSTANDARD & 0xFF00) >> 8;
    buf[head++] = TEXPRESS & 0xFF;
    buf[head++] = (TEXPRESS & 0xFF00) >> 8;
    HAL_UARTEx_RxEventCallback(&huart1, head);
    assert(cb_type == cb_RPLIDAR_OnSamplerate);
    printf("SUCCESS\n");
}

void RPLIDAR_OnSampleRate(rplidar_samplerate_t *samplerate)
{
    assert(samplerate->tstandart == TSTANDARD);
    assert(samplerate->texpress == TEXPRESS);
    cb_type = cb_RPLIDAR_OnSamplerate;
}

static void test_configuration_request(void)
{
    printf("test_configuration_request : ");
    cb_type = None;

    // Send info descriptor
    buf[head++] = 0xA5;
    buf[head++] = 0x5A;
    buf[head++] = 0x06;
    buf[head++] = 0x00;
    buf[head++] = 0x00;
    buf[head++] = 0x00;
    buf[head++] = 0x20;
#define TYPE 0x70
#define PAYLOAD 3
    buf[head++] = TYPE & 0xFF;
    buf[head++] = (TYPE & 0xFF00) >> 8;
    buf[head++] = (TYPE & 0xFF0000) >> 16;
    buf[head++] = (TYPE & 0xFF000000) >> 24;
    buf[head++] = (PAYLOAD & 0xFF00) >> 8;
    buf[head++] = PAYLOAD & 0xFF;
    HAL_UARTEx_RxEventCallback(&huart1, head);
    assert(cb_type == cb_RPLIDAR_OnConfiguration);
    printf("SUCCESS\n");
}

void RPLIDAR_OnConfiguration(uint32_t type, uint8_t *payload, uint16_t payload_size)
{
    assert(type == TYPE);
    assert(payload_size == 2);
    assert(payload[0] == (PAYLOAD & 0xFF00) >> 8);
    assert(payload[1] == PAYLOAD & 0xFF);
    cb_type = cb_RPLIDAR_OnConfiguration;
}

static void test_scan_request(void)
{
    printf("test_scan_request : ");
    cb_type = None;

    // Send info descriptor
    buf[head++] = 0xA5;
    buf[head++] = 0x5A;
    buf[head++] = 0x05;
    buf[head++] = 0x00;
    buf[head++] = 0x00;
    buf[head++] = 0x40;
    buf[head++] = 0x81;
#define START_FLAG 0x01
#define QUALITY 60
#define ANGLE 1200
#define DISTANCE 8150
    buf[head++] = ((QUALITY << 2) & 0xFC) | 0x01;
    buf[head++] = (((ANGLE & 0xFF) << 1) & 0xFE) | 1;
    buf[head++] = (ANGLE & 0xFF80) >> 7;
    buf[head++] = DISTANCE & 0xFF;
    buf[head++] = (DISTANCE & 0xFF00) >> 8;
    HAL_UARTEx_RxEventCallback(&huart1, head);
    assert(cb_type == cb_RPLIDAR_OnSingleMeasurement);
    printf("SUCCESS\n");
}

void RPLIDAR_OnSingleMeasurement(rplidar_measurement_t *measurement)
{
    assert(measurement->start == START_FLAG);
    assert(measurement->quality == QUALITY);
    assert(measurement->check == 1);
    assert(measurement->angle == ANGLE);
    assert(measurement->distance == DISTANCE);
    cb_type = cb_RPLIDAR_OnSingleMeasurement;
}
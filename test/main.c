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
    cb_RPLIDAR_OnSingleMeasurement,
    cb_RPLIDAR_OnDenseMeasurements
} callback_type_t;

UART_HandleTypeDef huart1;

extern uint8_t *buf;
static callback_type_t cb_type = None;

static void test_device_info_request(void);
static void test_health_request(void);
static void test_samplerate_request(void);
static void test_configuration_request(void);
static void test_scan_request(void);
static void test_scan_express_request(void);

int main()
{
    printf("START TESTS\n");
    RPLIDAR_Init(&huart1);

    test_device_info_request();
    test_health_request();
    test_samplerate_request();
    test_configuration_request();
    test_scan_request();
    test_scan_express_request();
}

static void test_device_info_request(void)
{
    uint16_t head = 0;
    printf("test_device_info_request : ");
    cb_type = None;

    RPLIDAR_RequestDeviceInfo(NULL, 0);

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
    buf[head++] = MODEL_MAJOR << 4 | MODEL_SUB;
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
    uint16_t head = 0;
    printf("test_health_request : ");
    cb_type = None;

    RPLIDAR_RequestHealth(NULL, 0);

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
    uint16_t head = 0;
    printf("test_samplerate_request : ");
    cb_type = None;

    RPLIDAR_RequestSampleRate(NULL, 0);

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
    uint16_t head = 0;
    uint8_t payload[3] = {0};
    printf("test_configuration_request : ");
    cb_type = None;

#define TYPE 0x70
#define PAYLOAD 3
    RPLIDAR_RequestConfiguration(TYPE, payload, PAYLOAD, NULL, 0);

    // Send info descriptor
    buf[head++] = 0xA5;
    buf[head++] = 0x5A;
    buf[head++] = 0x06;
    buf[head++] = 0x00;
    buf[head++] = 0x00;
    buf[head++] = 0x00;
    buf[head++] = 0x20;
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

void RPLIDAR_OnConfiguration(rplidar_configuration_t *config)
{
    assert(config->type == TYPE);
    assert(config->payload_size == 2);
    assert(config->payload[0] == (PAYLOAD & 0xFF00) >> 8);
    assert(config->payload[1] == PAYLOAD & 0xFF);
    cb_type = cb_RPLIDAR_OnConfiguration;
}

static void test_scan_request(void)
{
    uint16_t head = 0;
    printf("test_scan_request : ");
    cb_type = None;

    RPLIDAR_StartScan(NULL, 0, 0);

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

static void test_scan_express_request(void)
{
    uint16_t head = 0;
    printf("test_scan_express_request : ");
    cb_type = None;

    RPLIDAR_StartScanExpress(NULL, 0, 0);

    // Send info descriptor
    buf[head++] = 0xA5;
    buf[head++] = 0x5A;
    buf[head++] = 0x54;
    buf[head++] = 0x00;
    buf[head++] = 0x00;
    buf[head++] = 0x40;
    buf[head++] = 0x85;
#define SYNC_FLAG_EXPR 0xA5
#define ANGLE_EXPR 1200
#define START_FLAG_EXPR 0x01
#define DISTANCE_EXPR 1000
    buf[head++] = SYNC_FLAG_EXPR & 0xF0;
    buf[head++] = (SYNC_FLAG_EXPR & 0x0F) << 4;
    buf[head++] = ANGLE_EXPR & 0xFF;
    buf[head++] = ((ANGLE_EXPR & 0x7F00) >> 8) | (0x01 << 7);
    for (uint8_t i = 0; i < 40; i++)
    {
        buf[head++] = (DISTANCE_EXPR + i) & 0xFF;
        buf[head++] = ((DISTANCE_EXPR + i) & 0xFF00) >> 8;
    }
    HAL_UARTEx_RxEventCallback(&huart1, head);
    assert(cb_type == cb_RPLIDAR_OnDenseMeasurements);
    printf("SUCCESS\n");
}

void RPLIDAR_OnDenseMeasurements(rplidar_dense_measurements_t *measurement)
{
    assert(measurement->sync1 == 0xA);
    assert(measurement->sync2 == 0x5);
    assert(measurement->angle == ANGLE_EXPR);
    assert(measurement->start == START_FLAG_EXPR);
    for (uint8_t i = 0; i < 40; i++)
    {
        assert(measurement->distance[i] == DISTANCE_EXPR + i);
    }

    cb_type = cb_RPLIDAR_OnDenseMeasurements;
}

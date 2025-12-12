/*
 * diag.c
 *
 *  Created on: 8 déc. 2025
 *      Author: nicob
 */

/*
 * map.c
 *
 *  Created on: 28 nov. 2025
 *      Author: nicob
 */

#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include "diag.h"
#include "menu.h"
#include "rplidar.h"
#include "ILI9488.h"
#include "buzzer.h"

#define DIAG_BUTTON_CLOSE_X (ILI9488_HEIGHT - 50)
#define DIAG_BUTTON_CLOSE_Y 20
#define DIAG_BUTTON_CLOSE_W 24
#define DIAG_BUTTON_CLOSE_H 24

#define DIAG_BUTTON_HEALTH_X 90
#define DIAG_BUTTON_HEALTH_Y 70
#define DIAG_BUTTON_HEALTH_W 100
#define DIAG_BUTTON_HEALTH_H 45

#define DIAG_BUTTON_DEVICE_X 189
#define DIAG_BUTTON_DEVICE_Y 70
#define DIAG_BUTTON_DEVICE_W 100
#define DIAG_BUTTON_DEVICE_H 45

#define DIAG_BUTTON_SAMPLE_X 288
#define DIAG_BUTTON_SAMPLE_Y 70
#define DIAG_BUTTON_SAMPLE_W 101
#define DIAG_BUTTON_SAMPLE_H 45

#define DIAG_BOX_X 90
#define DIAG_BOX_Y 115
#define DIAG_BOX_W 300
#define DIAG_BOX_H 150

#define DIAG_BUTTON_DEBOUNCE_TIMER 1000
#define DIAG_REQUEST_TIMEOUT 500

static void _TestHealth(void);
static void _TestDevice(void);
static void _TestRate(void);

void DIAG_Show(void)
{
    ILI9488_FillScreen(ORANGE);
    ILI9488_CString(0, 20, ILI9488_HEIGHT, 20, "DIAGNOSTICS", Font24, 1, WHITE, ORANGE);
    ILI9488_DrawImage(DIAG_BUTTON_CLOSE_X, DIAG_BUTTON_CLOSE_Y, DIAG_BUTTON_CLOSE_W, DIAG_BUTTON_CLOSE_H, cross, sizeof(cross));

    ILI9488_CString(DIAG_BUTTON_HEALTH_X, DIAG_BUTTON_HEALTH_Y, DIAG_BUTTON_HEALTH_W + DIAG_BUTTON_HEALTH_X - 1,
    DIAG_BUTTON_HEALTH_Y + DIAG_BUTTON_HEALTH_H - 1,
                    "HEALTH", Font16, 1, WHITE, D_GREEN);

    ILI9488_CString(DIAG_BUTTON_DEVICE_X, DIAG_BUTTON_DEVICE_Y, DIAG_BUTTON_DEVICE_W + DIAG_BUTTON_DEVICE_X - 1,
    DIAG_BUTTON_DEVICE_Y + DIAG_BUTTON_DEVICE_H - 1,
                    "DEVICE", Font16, 1, WHITE, RED);

    ILI9488_CString(DIAG_BUTTON_SAMPLE_X, DIAG_BUTTON_SAMPLE_Y, DIAG_BUTTON_SAMPLE_W + DIAG_BUTTON_SAMPLE_X - 1,
    DIAG_BUTTON_SAMPLE_Y + DIAG_BUTTON_SAMPLE_H - 1,
                    "RATE", Font16, 1, WHITE, BLUE);

    ILI9488_FillArea(DIAG_BOX_X, DIAG_BOX_Y, DIAG_BOX_W, DIAG_BOX_H, WHITE);
    ILI9488_DrawBorder(DIAG_BOX_X, DIAG_BOX_Y - DIAG_BUTTON_SAMPLE_H, DIAG_BOX_W, DIAG_BOX_H + DIAG_BUTTON_SAMPLE_H, 2,
    WHITE);

    ILI9488_CString(DIAG_BOX_X + 1, DIAG_BOX_Y, DIAG_BOX_X + DIAG_BOX_W - 1, DIAG_BOX_Y + DIAG_BOX_H,
                    "Press a request to test", Font16, 1, BLACK, WHITE);
}

void DIAG_Touch(uint16_t x, uint16_t y)
{
    uint32_t tick_cur = HAL_GetTick();

    if (x >= DIAG_BUTTON_HEALTH_X && x < DIAG_BUTTON_HEALTH_X + DIAG_BUTTON_HEALTH_W && y >= DIAG_BUTTON_HEALTH_Y
            && y < DIAG_BUTTON_HEALTH_Y + DIAG_BUTTON_HEALTH_H)
    {
        static uint32_t tick_pressed = 0;
        if (tick_cur - tick_pressed < DIAG_BUTTON_DEBOUNCE_TIMER)
        {
            return;
        }
        tick_pressed = tick_cur;
        Buzzer_Play_Menu_Touch();

        _TestHealth();
    }
    else if (x >= DIAG_BUTTON_DEVICE_X && x < DIAG_BUTTON_DEVICE_X + DIAG_BUTTON_DEVICE_W && y >= DIAG_BUTTON_DEVICE_Y
            && y < DIAG_BUTTON_DEVICE_Y + DIAG_BUTTON_DEVICE_H)
    {
        static uint32_t tick_pressed = 0;
        if (tick_cur - tick_pressed < DIAG_BUTTON_DEBOUNCE_TIMER)
        {
            return;
        }
        tick_pressed = tick_cur;
        Buzzer_Play_Menu_Touch();

        _TestDevice();
    }
    else if (x >= DIAG_BUTTON_SAMPLE_X && x < DIAG_BUTTON_SAMPLE_X + DIAG_BUTTON_SAMPLE_W && y >= DIAG_BUTTON_SAMPLE_Y
            && y < DIAG_BUTTON_SAMPLE_Y + DIAG_BUTTON_SAMPLE_H)
    {
        static uint32_t tick_pressed = 0;
        if (tick_cur - tick_pressed < DIAG_BUTTON_DEBOUNCE_TIMER)
        {
            return;
        }
        tick_pressed = tick_cur;
        Buzzer_Play_Menu_Touch();

        _TestRate();
    }
    else if (x >= DIAG_BUTTON_CLOSE_X && x < DIAG_BUTTON_CLOSE_X + DIAG_BUTTON_CLOSE_W && y >= DIAG_BUTTON_CLOSE_Y
            && y < DIAG_BUTTON_CLOSE_Y + DIAG_BUTTON_CLOSE_H)
    {
        static uint32_t tick_pressed = 0;
        if (tick_cur - tick_pressed < DIAG_BUTTON_DEBOUNCE_TIMER)
        {
            return;
        }
        tick_pressed = tick_cur;
        Buzzer_Play_Menu_Out();

        MENU_SetScreen(MENU_SCREEN_MAIN);
    }
}

static void _TestHealth(void)
{
    rplidar_health_t health;
    char str[128];

    ILI9488_FillArea(DIAG_BOX_X, DIAG_BOX_Y, DIAG_BOX_W, DIAG_BOX_H, D_GREEN);
    ILI9488_DrawBorder(DIAG_BOX_X, DIAG_BOX_Y - DIAG_BUTTON_SAMPLE_H, DIAG_BOX_W, DIAG_BOX_H + DIAG_BUTTON_SAMPLE_H, 2,
    WHITE);

    if (RPLIDAR_RequestHealth(&health, DIAG_REQUEST_TIMEOUT))
    {
        snprintf(str, sizeof(str), "STATUS : %hu -> %s", health.status,
                 health.status != 0 ? (health.status == 1 ? "WARNING" : "ERROR") : "GOOD");
        ILI9488_WString(DIAG_BOX_X + 20, DIAG_BOX_Y + 30, str, Font16, 1, WHITE, D_GREEN);
        snprintf(str, sizeof(str), "ERROR :  %hu", health.error_code);
        ILI9488_WString(DIAG_BOX_X + 20, DIAG_BOX_Y + 50, str, Font16, 1, WHITE, D_GREEN);
    }
    else
    {
        ILI9488_CString(DIAG_BOX_X + 3, DIAG_BOX_Y + 3, DIAG_BOX_X + DIAG_BOX_W - 6, DIAG_BOX_Y + DIAG_BOX_H - 6,
                        "ERROR : No response", Font16, 1, WHITE, D_GREEN);
    }
}

static void _TestDevice(void)
{
    rplidar_info_t info;
    char str[128];
    uint8_t char_nb = 0;
    uint8_t i = 0;

    ILI9488_FillArea(DIAG_BOX_X, DIAG_BOX_Y, DIAG_BOX_W, DIAG_BOX_H, RED);
    ILI9488_DrawBorder(DIAG_BOX_X, DIAG_BOX_Y - DIAG_BUTTON_SAMPLE_H, DIAG_BOX_W, DIAG_BOX_H + DIAG_BUTTON_SAMPLE_H, 2,
    WHITE);

    if (RPLIDAR_RequestDeviceInfo(&info, DIAG_REQUEST_TIMEOUT))
    {
        snprintf(str, sizeof(str), "MODEL :    %hu.%hu", info.model_major, info.model_sub);
        ILI9488_WString(DIAG_BOX_X + 20, DIAG_BOX_Y + 20, str, Font16, 1, WHITE, RED);
        snprintf(str, sizeof(str), "FIRMWARE : %hu.%hu", info.fw_major, info.fw_minor);
        ILI9488_WString(DIAG_BOX_X + 20, DIAG_BOX_Y + 40, str, Font16, 1, WHITE, RED);
        snprintf(str, sizeof(str), "HARDWARE : %hu", info.hardware);
        ILI9488_WString(DIAG_BOX_X + 20, DIAG_BOX_Y + 60, str, Font16, 1, WHITE, RED);
        ILI9488_WString(DIAG_BOX_X + 20, DIAG_BOX_Y + 80, "SERIAL :   ", Font16, 1, WHITE, RED);
        for (i = 0; i < 16; i++)
        {
            char_nb += snprintf(&str[char_nb], sizeof(str) - char_nb, "%2hX", info.serial_nb[i]);
            if (i % 6 == 5 || i == 15)
            {
                // Write serial number on 3 lines
                ILI9488_WString(DIAG_BOX_X + 140, DIAG_BOX_Y + 80 + 20 * (i / 6), str, Font16, 1, WHITE, RED);
                char_nb = 0;
            }
        }
    }
    else
    {
        ILI9488_CString(DIAG_BOX_X + 3, DIAG_BOX_Y + 3, DIAG_BOX_X + DIAG_BOX_W - 6, DIAG_BOX_Y + DIAG_BOX_H - 6,
                        "ERROR : No response", Font16, 1, WHITE, RED);
    }
}

static void _TestRate(void)
{
    rplidar_samplerate_t rate;
    char str[128];

    ILI9488_FillArea(DIAG_BOX_X, DIAG_BOX_Y, DIAG_BOX_W, DIAG_BOX_H, BLUE);
    ILI9488_DrawBorder(DIAG_BOX_X, DIAG_BOX_Y - DIAG_BUTTON_SAMPLE_H, DIAG_BOX_W, DIAG_BOX_H + DIAG_BUTTON_SAMPLE_H, 2,
    WHITE);

    if (RPLIDAR_RequestSampleRate(&rate, DIAG_REQUEST_TIMEOUT))
    {
        snprintf(str, sizeof(str), "STANDART RATE : %hu", rate.tstandart);
        ILI9488_WString(DIAG_BOX_X + 20, DIAG_BOX_Y + 30, str, Font16, 1, WHITE, BLUE);
        snprintf(str, sizeof(str), "EXPRESS RATE :  %hu", rate.texpress);
        ILI9488_WString(DIAG_BOX_X + 20, DIAG_BOX_Y + 50, str, Font16, 1, WHITE, BLUE);
    }
    else
    {
        ILI9488_CString(DIAG_BOX_X + 3, DIAG_BOX_Y + 3, DIAG_BOX_X + DIAG_BOX_W - 6, DIAG_BOX_Y + DIAG_BOX_H - 6,
                        "ERROR : No response", Font16, 1, WHITE, BLUE);
    }
}

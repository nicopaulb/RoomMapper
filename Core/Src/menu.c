/*
 * menu.c
 *
 *  Created on: 8 déc. 2025
 *      Author: nicob
 */

#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include "menu.h"
#include "ILI9488.h"
#include "XPT2046.h"
#include "map.h"
#include "diag.h"
#include "buzzer.h"

#define MENU_BUTTON_DEBOUNCE_TIMER 100 // ms

#define MENU_BUTTON_START_X 80
#define MENU_BUTTON_START_Y 0
#define MENU_BUTTON_START_W 320
#define MENU_BUTTON_START_H 320

#define MENU_BUTTON_DIAG_X 5
#define MENU_BUTTON_DIAG_Y (ILI9488_WIDTH - 55)
#define MENU_BUTTON_DIAG_W 67
#define MENU_BUTTON_DIAG_H 45

#define MENU_BUTTON_VOL_UP_X (ILI9488_HEIGHT - 40)
#define MENU_BUTTON_VOL_UP_Y (((ILI9488_WIDTH-MENU_VOLUME_BAR_HEIGHT)/2) - 40)
#define MENU_BUTTON_VOL_UP_W 30
#define MENU_BUTTON_VOL_UP_H 30

#define MENU_BUTTON_VOL_DOWN_X (ILI9488_HEIGHT - 40)
#define MENU_BUTTON_VOL_DOWN_Y (((ILI9488_WIDTH-MENU_VOLUME_BAR_HEIGHT)/2) + MENU_VOLUME_BAR_HEIGHT + 10)
#define MENU_BUTTON_VOL_DOWN_W 30
#define MENU_BUTTON_VOL_DOWN_H 30

#define MENU_VOLUME_BAR_HEIGHT 150

static void _MainTouch(uint16_t x, uint16_t y);
static void _MainDrawVolumeBar(void);
static void _MainShow(void);

static menu_screen_e menu_screen = MENU_SCREEN_MAIN;
static bool menu_screen_initialized = false;

void MENU_SetScreen(menu_screen_e screen)
{
    menu_screen = screen;
    menu_screen_initialized = false;
}

void MENU_HandleTouch(void)
{
    uint16_t x = 0;
    uint16_t y = 0;

    if (XPT2046_GotATouch() && XPT2046_GetTouchPosition(&x, &y))
    {
        switch (menu_screen)
        {
            case MENU_SCREEN_MAIN:
                _MainTouch(x, y);
                break;
            case MENU_SCREEN_MAP:
                MAP_Touch(x, y);
                break;
            case MENU_SCREEN_DIAG:
                DIAG_Touch(x, y);
                break;
        }
    }
}

void MENU_UpdateScreen(void)
{
    switch (menu_screen)
    {
        case MENU_SCREEN_MAIN:
            if (!menu_screen_initialized)
            {
                _MainShow();
                menu_screen_initialized = true;
            }
            break;
        case MENU_SCREEN_MAP:
            if (!menu_screen_initialized)
            {
                MAP_Show();
                menu_screen_initialized = true;
            }
            MAP_DrawSamples();
            break;
        case MENU_SCREEN_DIAG:
            if (!menu_screen_initialized)
            {
                DIAG_Show();
                menu_screen_initialized = true;
            }
            break;
    }
}

static void _MainShow(void)
{
    ILI9488_FillScreen(BLACK);
    ILI9488_DrawImage(100, ILI9488_WIDTH / 2 - 62, 64, 64, logo, sizeof(logo));
    ILI9488_WString(180, ILI9488_WIDTH / 2 - 42, "ROOM MAPPER", Font24, 1, WHITE, BLACK);
    ILI9488_DrawBorder(80, ILI9488_WIDTH / 2 - 80, 320, 100, 2, WHITE);
    ILI9488_CString(0, 220, ILI9488_HEIGHT, 250, "Press to START", Font20, 1, GREEN, BLACK);
    ILI9488_CString(0, ILI9488_WIDTH - 16, ILI9488_HEIGHT, ILI9488_WIDTH - 16, "Version 1.1", Font16, 1, DD_WHITE,
    BLACK);

    ILI9488_CString(MENU_BUTTON_DIAG_X, MENU_BUTTON_DIAG_Y, MENU_BUTTON_DIAG_X + MENU_BUTTON_DIAG_W - 1,
    MENU_BUTTON_DIAG_Y + MENU_BUTTON_DIAG_H,
                    "DIAG", Font16, 1, WHITE, ORANGE);
    ILI9488_DrawBorder(MENU_BUTTON_DIAG_X, MENU_BUTTON_DIAG_Y, MENU_BUTTON_DIAG_W, MENU_BUTTON_DIAG_H, 2, WHITE);

    // Draw + button
    ILI9488_FillArea(ILI9488_HEIGHT - 35, ((ILI9488_WIDTH - MENU_VOLUME_BAR_HEIGHT) / 2) - 25, 20, 2, WHITE);
    ILI9488_FillArea(ILI9488_HEIGHT - 26, ((ILI9488_WIDTH - MENU_VOLUME_BAR_HEIGHT) / 2) - 34, 2, 20, WHITE);
    ILI9488_DrawCircle(ILI9488_HEIGHT - 25, ((ILI9488_WIDTH - MENU_VOLUME_BAR_HEIGHT) / 2) - 25, 15, WHITE);

    _MainDrawVolumeBar();

    // Draw - button
    ILI9488_FillArea(ILI9488_HEIGHT - 34, ((ILI9488_WIDTH - MENU_VOLUME_BAR_HEIGHT) / 2) + MENU_VOLUME_BAR_HEIGHT + 25,
                     20, 2, WHITE);
    ILI9488_DrawCircle(ILI9488_HEIGHT - 25,
                       ((ILI9488_WIDTH - MENU_VOLUME_BAR_HEIGHT) / 2) + MENU_VOLUME_BAR_HEIGHT + 25, 15, WHITE);

    ILI9488_DrawImage(ILI9488_HEIGHT - 34, ILI9488_WIDTH - 30, 24, 24, volume, sizeof(volume));
}

static void _MainDrawVolumeBar(void)
{
    uint8_t filled_bar_height = Buzzer_GetVolume() * MENU_VOLUME_BAR_HEIGHT / 100;
    ILI9488_FillArea(ILI9488_HEIGHT - 30, ((ILI9488_WIDTH - MENU_VOLUME_BAR_HEIGHT) / 2), 10,
    MENU_VOLUME_BAR_HEIGHT - filled_bar_height,
                     DDD_WHITE);
    ILI9488_FillArea(ILI9488_HEIGHT - 30,
                     ((ILI9488_WIDTH - MENU_VOLUME_BAR_HEIGHT) / 2) + (MENU_VOLUME_BAR_HEIGHT - filled_bar_height), 10,
                     filled_bar_height, WHITE);
}

static void _MainTouch(uint16_t x, uint16_t y)
{
    uint32_t tick_cur = HAL_GetTick();

    if (x >= MENU_BUTTON_VOL_UP_X && x < MENU_BUTTON_VOL_UP_X + MENU_BUTTON_VOL_UP_W && y >= MENU_BUTTON_VOL_UP_Y
            && y < MENU_BUTTON_VOL_UP_Y + MENU_BUTTON_VOL_UP_H)
    {
        // Button VOLUME UP pressed
        static uint32_t tick_pressed = 0;
        if (tick_cur - tick_pressed < MENU_BUTTON_DEBOUNCE_TIMER)
        {
            return;
        }
        tick_pressed = tick_cur;

        uint8_t volume_cur = Buzzer_GetVolume();
        if (volume_cur < 100)
        {
            Buzzer_SetVolume(volume_cur + 25);
        }
        Buzzer_Play_Menu_Touch();
        _MainDrawVolumeBar();

    }
    else if (x >= MENU_BUTTON_VOL_DOWN_X && x < MENU_BUTTON_VOL_DOWN_X + MENU_BUTTON_VOL_DOWN_W
            && y >= MENU_BUTTON_VOL_DOWN_Y && y < MENU_BUTTON_VOL_DOWN_Y + MENU_BUTTON_VOL_DOWN_H)
    {
        // Button VOLUME DOWN pressed
        static uint32_t tick_pressed = 0;
        if (tick_cur - tick_pressed < MENU_BUTTON_DEBOUNCE_TIMER)
        {
            return;
        }
        tick_pressed = tick_cur;

        uint8_t volume_cur = Buzzer_GetVolume();
        if (volume_cur > 0)
        {
            Buzzer_SetVolume(volume_cur - 25);
        }
        Buzzer_Play_Menu_Touch();
        _MainDrawVolumeBar();
    }
    else if (x >= MENU_BUTTON_DIAG_X && x < MENU_BUTTON_DIAG_X + MENU_BUTTON_DIAG_W && y >= MENU_BUTTON_DIAG_Y
            && y < MENU_BUTTON_DIAG_Y + MENU_BUTTON_DIAG_H)
    {
        Buzzer_Play_Menu_In();
        MENU_SetScreen(MENU_SCREEN_DIAG);
    }
    else if (x >= MENU_BUTTON_START_X && x < MENU_BUTTON_START_X + MENU_BUTTON_START_W && y >= MENU_BUTTON_START_Y
            && y < MENU_BUTTON_START_Y + MENU_BUTTON_START_H)
    {
        Buzzer_Play_Menu_In();
        MENU_SetScreen(MENU_SCREEN_MAP);
    }
}

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

#define MENU_BUTTON_DIAG_X 5
#define MENU_BUTTON_DIAG_Y (ILI9488_WIDTH - 55)
#define MENU_BUTTON_DIAG_W 67
#define MENU_BUTTON_DIAG_H 45

static void _MainTouch(uint16_t x, uint16_t y);
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
}

static void _MainTouch(uint16_t x, uint16_t y)
{
    if (x >= MENU_BUTTON_DIAG_X && x < MENU_BUTTON_DIAG_X + MENU_BUTTON_DIAG_W && y >= MENU_BUTTON_DIAG_Y
            && y < MENU_BUTTON_DIAG_Y + MENU_BUTTON_DIAG_H)
    {
        MENU_SetScreen(MENU_SCREEN_DIAG);
    }
    else
    {
        MENU_SetScreen(MENU_SCREEN_MAP);
    }
}

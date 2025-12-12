/*
 * map.c
 *
 *  Created on: 28 nov. 2025
 *      Author: nicob
 */

#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdbool.h>

#include "map.h"
#include "rplidar.h"
#include "ILI9488.h"
#include "XPT2046.h"
#include "buzzer.h"

#define SAMPLE_BUF_SIZE 1024
#define SAMPLE_QUALITY_DEFAULT 18
#define POINT_BUF_SIZE 8096

#define MAP_SIZE ILI9488_WIDTH
#define MAP_DEFAULT_DISTANCE_MAX 1000.0f
#define MAP_DEFAULT_SCALE ((MAP_SIZE / 2) / MAP_DEFAULT_DISTANCE_MAX)
#define MAP_PERSISTENCE_ONESHOT_DURATION 2000 //ms

#define MAP_TOOLBAR_WIDTH ((ILI9488_HEIGHT - ILI9488_WIDTH) / 2)

#define MAP_BUTTON_DEBOUNCE_TIMER 500 // ms
#define MAP_BUTTON_START_DEBOUNCE_TIMER 2000 // ms

#define MAP_BUTTON_START_X 5
#define MAP_BUTTON_START_Y (ILI9488_WIDTH - 55)
#define MAP_BUTTON_START_W (MAP_TOOLBAR_WIDTH - 13)
#define MAP_BUTTON_START_H 45

#define MAP_BUTTON_SCALE_X 5
#define MAP_BUTTON_SCALE_Y (MAP_SIZE / 5 + 10)
#define MAP_BUTTON_SCALE_W (MAP_TOOLBAR_WIDTH - 13)
#define MAP_BUTTON_SCALE_H 45

#define MAP_BUTTON_QUAL_MINUS_X ILI9488_HEIGHT - MAP_TOOLBAR_WIDTH + 3
#define MAP_BUTTON_QUAL_MINUS_Y 155
#define MAP_BUTTON_QUAL_MINUS_W 20
#define MAP_BUTTON_QUAL_MINUS_H 20

#define MAP_BUTTON_QUAL_PLUS_X ILI9488_HEIGHT - MAP_TOOLBAR_WIDTH + 60
#define MAP_BUTTON_QUAL_PLUS_Y 155
#define MAP_BUTTON_QUAL_PLUS_W 20
#define MAP_BUTTON_QUAL_PLUS_H 20

#define MAP_BUTTON_PERS_MODE_X (ILI9488_HEIGHT - MAP_TOOLBAR_WIDTH + 11)
#define MAP_BUTTON_PERS_MODE_Y 215
#define MAP_BUTTON_PERS_MODE_W (MAP_TOOLBAR_WIDTH - 13)
#define MAP_BUTTON_PERS_MODE_H 45

#define MAP_BUTTON_PERS_CLEAR_X (ILI9488_HEIGHT - MAP_TOOLBAR_WIDTH + 11)
#define MAP_BUTTON_PERS_CLEAR_Y (ILI9488_WIDTH - 55)
#define MAP_BUTTON_PERS_CLEAR_W (MAP_TOOLBAR_WIDTH - 13)
#define MAP_BUTTON_PERS_CLEAR_H 45

#define MAP_QUALITY_GRADIENT_X (ILI9488_HEIGHT - MAP_TOOLBAR_WIDTH + 15)
#define MAP_QUALITY_GRADIENT_Y 10
#define MAP_QUALITY_GRADIENT_W 10
#define MAP_QUALITY_GRADIENT_H 100
#define MAP_QUALITY_GRADIENT_NB 10

typedef struct
{
    uint16_t x;
    uint16_t y;
    uint16_t color;
} point_t;

static const point_t map_center_point = {.x = ILI9488_HEIGHT / 2, .y = ILI9488_WIDTH / 2, .color = WHITE};
static const point_t map_invalid_point = {0};

static rplidar_measurement_t map_sample_buf[SAMPLE_BUF_SIZE];
static uint16_t map_sample_write_idx = 0;
static uint16_t map_sample_read_idx = 0;
static uint16_t map_sample_count = 0;

static point_t map_point_buf[POINT_BUF_SIZE] = {0};
static uint16_t map_point_idx = 0;

static uint8_t map_quality_min = SAMPLE_QUALITY_DEFAULT; // 0-63
static map_scale_mode_e map_scale_mode = MAP_SCALE_AUTO;
static map_persistence_mode_e map_persistence_mode = MAP_PERSIST_OFF;
static uint32_t map_persistence_start_tick = 0;
static double map_scale_distance_max = MAP_DEFAULT_DISTANCE_MAX;
static double map_scale_factor = MAP_DEFAULT_SCALE;
static point_t map_selected_point[2] = {map_invalid_point, map_invalid_point};
static uint8_t map_selected_point_idx = 0;

static bool _ConvertSampleToPoint(rplidar_measurement_t *sample, point_t *point);
static void _DrawGrid(void);
static void _DrawMapScale(double scale);
static void _DrawButtonStart(bool is_started);
static void _DrawButtonScale(map_scale_mode_e mode);
static void _DrawQualityGradient(void);
static void _DrawQualityMinimum(uint8_t quality);
static void _DrawPersistanceButtons(map_persistence_mode_e mode);
static void _DrawDistanceInfo(const point_t *p1, const point_t *p2);
static double _GetDistancePoints(const point_t *p1, const point_t *p2);

void MAP_Show(void)
{
    MAP_SetScaleMode(MAP_SCALE_AUTO);
    MAP_SetQuality(SAMPLE_QUALITY_DEFAULT);
    MAP_DrawMenu();
}

void MAP_DrawMenu(void)
{
    ILI9488_FillScreen(BLACK);
    _DrawGrid();
    _DrawMapScale(map_scale_distance_max / 5000.0);
    _DrawButtonStart(false);
    _DrawButtonScale(map_scale_mode);
    _DrawQualityGradient();
    _DrawQualityMinimum(map_quality_min);
    _DrawPersistanceButtons(map_persistence_mode);
    _DrawDistanceInfo(&map_selected_point[0], &map_selected_point[1]);
}

void MAP_DrawSamples(void)
{
    while (map_sample_count)
    {
        point_t new_point;
        bool is_valid = _ConvertSampleToPoint(&map_sample_buf[map_sample_read_idx], &new_point);
        map_sample_read_idx = (map_sample_read_idx + 1) % SAMPLE_BUF_SIZE;
        map_sample_count--;

        if (is_valid)
        {
            point_t *point = &map_point_buf[map_point_idx];
            if (point->x != 0 || point->y != 0)
            {
                switch (map_persistence_mode)
                {
                    default:
                    case MAP_PERSIST_OFF:
                        // Remove old point from the screen
                        ILI9488_Pixel(point->x, point->y, BLACK);
                        break;
                    case MAP_PERSIST_ON:
                        // Accumulate points on the screen
                        break;
                    case MAP_PERSIST_ONESHOT:
                        // Accumulate points and stop after a specific time
                        if (map_persistence_start_tick == 0)
                        {
                            map_persistence_start_tick = HAL_GetTick();
                        }
                        if (HAL_GetTick() - map_persistence_start_tick > MAP_PERSISTENCE_ONESHOT_DURATION /*ms*/)
                        {
                            // Immediatly return to not draw the new point
                            map_point_idx = (map_point_idx + 1) % POINT_BUF_SIZE;
                            return;
                        }
                        break;
                }
            }

            *point = new_point;

            // Draw new point
            ILI9488_Pixel(point->x, point->y, point->color);
            map_point_idx = (map_point_idx + 1) % POINT_BUF_SIZE;
        }
    }
}

void MAP_Touch(uint16_t x, uint16_t y)
{
    uint32_t tick_cur = HAL_GetTick();

    if (x >= MAP_BUTTON_START_X && x < MAP_BUTTON_START_X + MAP_BUTTON_START_W && y >= MAP_BUTTON_START_Y
            && y < MAP_BUTTON_START_Y + MAP_BUTTON_START_H)
    {
        // Button START/STOP pressed
        static uint32_t tick_pressed = 0;
        if (tick_cur - tick_pressed < MAP_BUTTON_START_DEBOUNCE_TIMER)
        {
            return;
        }
        tick_pressed = tick_cur;
        Buzzer_Play_Menu_Touch();

        static bool running = false;
        if (running)
        {
            RPLIDAR_StopScan();
        }
        else
        {
            RPLIDAR_StartScan(NULL, 0, 0);
        }
        running = !running;
        _DrawButtonStart(running);
    }
    else if (x >= MAP_BUTTON_SCALE_X && x < MAP_BUTTON_SCALE_X + MAP_BUTTON_SCALE_W && y >= MAP_BUTTON_SCALE_Y
            && y < MAP_BUTTON_SCALE_Y + MAP_BUTTON_SCALE_H)
    {
        // Button SCALE MODE pressed
        static uint32_t tick_pressed = 0;
        if (tick_cur - tick_pressed < MAP_BUTTON_DEBOUNCE_TIMER)
        {
            return;
        }
        tick_pressed = tick_cur;
        Buzzer_Play_Menu_Touch();

        map_scale_mode_e new_mode = (map_scale_mode + 1) % MAP_SCALE_MAX;
        MAP_SetScaleMode(new_mode);
        _DrawButtonScale(map_scale_mode);
        _DrawMapScale(map_scale_distance_max / 5000.0);
        MAP_ClearPoints(false);
    }
    else if (x >= MAP_BUTTON_QUAL_MINUS_X && x < MAP_BUTTON_QUAL_MINUS_X + MAP_BUTTON_QUAL_MINUS_W
            && y >= MAP_BUTTON_QUAL_MINUS_Y && y < MAP_BUTTON_QUAL_MINUS_Y + MAP_BUTTON_QUAL_MINUS_H)
    {
        // Button QUALITY - pressed
        static uint32_t tick_pressed = 0;
        if (tick_cur - tick_pressed < MAP_BUTTON_DEBOUNCE_TIMER)
        {
            return;
        }
        tick_pressed = tick_cur;

        uint8_t new_quality = map_quality_min < 6 ? 0 : (map_quality_min - 6);
        MAP_SetQuality(new_quality);
        _DrawQualityMinimum(map_quality_min);
        MAP_ClearPoints(true);
    }
    else if (x >= MAP_BUTTON_QUAL_PLUS_X && x < MAP_BUTTON_QUAL_PLUS_X + MAP_BUTTON_QUAL_PLUS_W
            && y >= MAP_BUTTON_QUAL_PLUS_Y && y < MAP_BUTTON_QUAL_PLUS_Y + MAP_BUTTON_QUAL_PLUS_H)
    {
        // Button QUALITY + pressed
        static uint32_t tick_pressed = 0;
        if (tick_cur - tick_pressed < MAP_BUTTON_DEBOUNCE_TIMER)
        {
            return;
        }
        tick_pressed = tick_cur;
        Buzzer_Play_Menu_Touch();

        uint8_t new_quality = map_quality_min > 57 ? 63 : (map_quality_min + 6);
        MAP_SetQuality(new_quality);
        _DrawQualityMinimum(map_quality_min);
        MAP_ClearPoints(true);
    }
    else if (x >= MAP_BUTTON_PERS_MODE_X && x < MAP_BUTTON_PERS_MODE_X + MAP_BUTTON_PERS_MODE_W
            && y >= MAP_BUTTON_PERS_MODE_Y && y < MAP_BUTTON_PERS_MODE_Y + MAP_BUTTON_PERS_MODE_H)
    {
        // Button PERSISTENCE MODE pressed
        static uint32_t tick_pressed = 0;
        if (tick_cur - tick_pressed < MAP_BUTTON_DEBOUNCE_TIMER)
        {
            return;
        }
        tick_pressed = tick_cur;
        Buzzer_Play_Menu_Touch();

        map_persistence_mode_e new_mode = (map_persistence_mode + 1) % MAP_PERSIST_MAX;
        MAP_SetPersistanceMode(new_mode);
        _DrawPersistanceButtons(map_persistence_mode);
        MAP_ClearPoints(false);
    }
    else if (x >= MAP_BUTTON_PERS_CLEAR_X && x < MAP_BUTTON_PERS_CLEAR_X + MAP_BUTTON_PERS_CLEAR_W
            && y >= MAP_BUTTON_PERS_CLEAR_Y && y < MAP_BUTTON_PERS_CLEAR_Y + MAP_BUTTON_PERS_CLEAR_H)
    {
        // Button CLEAR pressed
        static uint32_t tick_pressed = 0;
        if (tick_cur - tick_pressed < MAP_BUTTON_DEBOUNCE_TIMER)
        {
            return;
        }
        tick_pressed = tick_cur;
        Buzzer_Play_Menu_Touch();

        MAP_ClearPoints(true);
    }
    else if (x >= MAP_TOOLBAR_WIDTH && x <= (MAP_TOOLBAR_WIDTH + MAP_SIZE))
    {
        // Press on RADAR area
        static uint32_t tick_pressed = 0;
        if (tick_cur - tick_pressed < MAP_BUTTON_DEBOUNCE_TIMER)
        {
            return;
        }
        tick_pressed = tick_cur;
        Buzzer_Play_Menu_Touch();

        // Remove old point and redraw center reference point
        if (map_selected_point[map_selected_point_idx].x != 0)
        {
            ILI9488_FillCircle(map_selected_point[map_selected_point_idx].x,
                               map_selected_point[map_selected_point_idx].y, 3, BLACK);
            ILI9488_FillCircle(map_center_point.x, map_center_point.y, 3, map_center_point.color);
        }

        map_selected_point[map_selected_point_idx].x = x;
        map_selected_point[map_selected_point_idx].y = y;
        map_selected_point[map_selected_point_idx].color = map_selected_point_idx == 0 ? BLUE : CYAN;
        ILI9488_FillCircle(x, y, 3, map_selected_point[map_selected_point_idx].color);
        _DrawDistanceInfo(&map_selected_point[0], &map_selected_point[1]);
        map_selected_point_idx = (map_selected_point_idx + 1) % 2;
    }
}

void MAP_ClearPoints(bool erase_buffers)
{
    // Erase all points from screen and redraw grid
    ILI9488_FillArea(MAP_TOOLBAR_WIDTH, 0, MAP_SIZE, ILI9488_WIDTH, BLACK);
    _DrawGrid();
    map_selected_point[0] = map_invalid_point;
    map_selected_point[1] = map_invalid_point;
    _DrawDistanceInfo(&map_selected_point[0], &map_selected_point[1]);
    map_persistence_start_tick = 0;

    if (erase_buffers)
    {
        // Empty all buffers
        memset(&map_point_buf[0], 0, sizeof(point_t) * POINT_BUF_SIZE);
        map_point_idx = 0;

        memset(&map_sample_buf[0], 0, sizeof(rplidar_measurement_t) * SAMPLE_BUF_SIZE);
        map_sample_write_idx = 0;
        map_sample_count = 0;
    }
}

void MAP_SetScaleMode(map_scale_mode_e mode)
{
    switch (mode)
    {
        default:
        case MAP_SCALE_AUTO:
            // Will be auto adjusted based on the longest sample distance
            map_scale_distance_max = MAP_DEFAULT_DISTANCE_MAX;
            map_scale_factor = MAP_DEFAULT_SCALE;
            break;
        case MAP_SCALE_1000:
            map_scale_distance_max = 1000.0;
            map_scale_factor = (MAP_SIZE / 2) / map_scale_distance_max;
            break;
        case MAP_SCALE_2500:
            map_scale_distance_max = 2500.0;
            map_scale_factor = (MAP_SIZE / 2) / map_scale_distance_max;
            break;
        case MAP_SCALE_5000:
            map_scale_distance_max = 5000.0;
            map_scale_factor = (MAP_SIZE / 2) / map_scale_distance_max;
            break;
        case MAP_SCALE_10000:
            map_scale_distance_max = 10000.0;
            map_scale_factor = (MAP_SIZE / 2) / map_scale_distance_max;
            break;
    }
    map_scale_mode = mode;
}

void MAP_SetQuality(uint8_t quality)
{
    map_quality_min = quality;
}

void MAP_SetPersistanceMode(map_persistence_mode_e mode)
{
    map_persistence_mode = mode;
}

void RPLIDAR_OnSingleMeasurement(rplidar_measurement_t *measurement)
{
    if (map_sample_count < SAMPLE_BUF_SIZE)
    {
        if ((measurement->distance != 0) && (measurement->quality >= map_quality_min))
        {
            // Filter only valid and good quality measurement
            memcpy(&map_sample_buf[map_sample_write_idx], measurement, sizeof(rplidar_measurement_t));
            map_sample_write_idx = (map_sample_write_idx + 1) % SAMPLE_BUF_SIZE;
            map_sample_count++;
        }
    }
}

static bool _ConvertSampleToPoint(rplidar_measurement_t *sample, point_t *point)
{
    double distance_mm = sample->distance / 4.0;
    double angle_rad = (sample->angle / 64.0) * M_PI / 180;
    double y = -distance_mm * cosf(angle_rad);
    double x = distance_mm * sinf(angle_rad);

    if (map_scale_mode == MAP_SCALE_AUTO)
    {
        if (distance_mm > map_scale_distance_max)
        {
            map_scale_distance_max = distance_mm;
            map_scale_factor = ((MAP_SIZE / 2) / map_scale_distance_max);
            _DrawMapScale(map_scale_distance_max / 5000.0);
            MAP_ClearPoints(false);
        }
    }

    point->x = x * map_scale_factor + ILI9488_HEIGHT / 2;
    point->y = y * map_scale_factor + ILI9488_WIDTH / 2;
    point->color = color565(0xFF - (sample->quality * 4), sample->quality * 4, 0x00); // Quality range:  0-63

    return (point->x >= MAP_TOOLBAR_WIDTH) && (point->x <= (MAP_TOOLBAR_WIDTH + MAP_SIZE)) && (point->y <= MAP_SIZE);
}

static void _DrawGrid(void)
{
    // Draw center reference point
    ILI9488_FillCircle(map_center_point.x, map_center_point.y, 3, map_center_point.color);

    for (uint8_t i = 0; i < 6; i++)
    {
        // For the last horizontal line, -1 pixel offset to make the line fits in the screen
        ILI9488_FillArea(MAP_TOOLBAR_WIDTH, i != 5 ? (MAP_SIZE / 5) * i : MAP_SIZE - 1, MAP_SIZE, 1,
        DDDD_WHITE);
        ILI9488_FillArea(MAP_TOOLBAR_WIDTH + (MAP_SIZE / 5) * i, 0, 1, MAP_SIZE,
        DDDD_WHITE);
    }
}

static void _DrawMapScale(double scale)
{
    char scale_txt[10];
    snprintf(scale_txt, sizeof(scale_txt), "%.2f m", scale);
    ILI9488_FillArea(MAP_TOOLBAR_WIDTH - 12, 2, 2, MAP_SIZE / 5 - 2, WHITE);
    ILI9488_FillArea(MAP_TOOLBAR_WIDTH - 16, 0, 10, 2, WHITE);
    ILI9488_FillArea(MAP_TOOLBAR_WIDTH - 16, MAP_SIZE / 5, 10, 2, WHITE);
    ILI9488_CString(0, 0, MAP_TOOLBAR_WIDTH - 16, MAP_SIZE / 5, scale_txt, Font12, 1, WHITE, BLACK);
}

static void _DrawButtonStart(bool is_started)
{
    ILI9488_CString(MAP_BUTTON_START_X, MAP_BUTTON_START_Y,
    MAP_TOOLBAR_WIDTH - 10,
                    ILI9488_WIDTH - 10, is_started ? "STOP" : "START", Font16, 1, WHITE, is_started ? RED : DD_GREEN);
    ILI9488_DrawBorder(MAP_BUTTON_START_X, MAP_BUTTON_START_Y,
    MAP_BUTTON_START_W,
                       MAP_BUTTON_START_H, 2,
                       WHITE);
}

static void _DrawButtonScale(map_scale_mode_e mode)
{
    const char *str = NULL;
    switch (mode)
    {
        default:
        case MAP_SCALE_AUTO:
            str = "AUTO";
            break;
        case MAP_SCALE_1000:
            str = "1.0m";
            break;
        case MAP_SCALE_2500:
            str = "2.5m";
            break;
        case MAP_SCALE_5000:
            str = "5.0m";
            break;
        case MAP_SCALE_10000:
            str = "10.0m";
            break;
    }

    ILI9488_CString(MAP_BUTTON_SCALE_X, MAP_BUTTON_SCALE_Y,
    MAP_TOOLBAR_WIDTH - 10,
                    MAP_SIZE / 5 + 54, str, Font16, 1, WHITE, BLUE);
    ILI9488_DrawBorder(MAP_BUTTON_SCALE_X, MAP_BUTTON_SCALE_Y,
    MAP_BUTTON_SCALE_W,
                       MAP_BUTTON_SCALE_H, 2,
                       WHITE);
}

static void _DrawQualityGradient(void)
{
    for (uint8_t i = 0; i < MAP_QUALITY_GRADIENT_NB; i++)
    {
        ILI9488_FillArea(MAP_QUALITY_GRADIENT_X,
                         i * (MAP_QUALITY_GRADIENT_H / MAP_QUALITY_GRADIENT_NB) + MAP_QUALITY_GRADIENT_Y,
                         MAP_QUALITY_GRADIENT_W,
                         MAP_QUALITY_GRADIENT_H / MAP_QUALITY_GRADIENT_NB, color565(i * 25, 0xFF - i * 25, 0x00));
    }
    ILI9488_FillArea(MAP_QUALITY_GRADIENT_X + 10, MAP_QUALITY_GRADIENT_Y, 5, 2,
    WHITE);
    ILI9488_FillArea(MAP_QUALITY_GRADIENT_X + 10,
    MAP_QUALITY_GRADIENT_Y + MAP_QUALITY_GRADIENT_H - 1,
                     5, 2, WHITE);
    ILI9488_WString(MAP_QUALITY_GRADIENT_X + 17, MAP_QUALITY_GRADIENT_Y - 5, "100", Font12, 1, WHITE, BLACK);
    ILI9488_WString(MAP_QUALITY_GRADIENT_X + 17,
    MAP_QUALITY_GRADIENT_Y + MAP_QUALITY_GRADIENT_H - 5,
                    "0", Font12, 1,
                    WHITE,
                    BLACK);
    ILI9488_Orientation(ILI9488_Orientation_180);
    ILI9488_CString(10, 5, MAP_QUALITY_GRADIENT_H + MAP_QUALITY_GRADIENT_Y + 5, 10, "QUALITY(%)", Font16, 1, WHITE,
    BLACK);
    ILI9488_Orientation(ILI9488_Orientation_90);
}

static void _DrawQualityMinimum(uint8_t quality)
{
    char quality_txt[5];
    snprintf(quality_txt, sizeof(quality_txt), "%hu", (uint8_t) (round((quality / 63.0) * 10)) * 10);

    ILI9488_CString(ILI9488_HEIGHT - MAP_TOOLBAR_WIDTH + 1, 130, ILI9488_HEIGHT, 130, "MIN", Font16, 1, WHITE, BLACK);
    ILI9488_CString(ILI9488_HEIGHT - MAP_TOOLBAR_WIDTH + 1, 155, ILI9488_HEIGHT, 175, quality_txt, Font16, 1, WHITE,
    BLACK);
    // Sign -
    ILI9488_DrawBorder(MAP_BUTTON_QUAL_MINUS_X, MAP_BUTTON_QUAL_MINUS_Y,
    MAP_BUTTON_QUAL_MINUS_W,
                       MAP_BUTTON_QUAL_MINUS_H, 2, WHITE);
    ILI9488_FillArea(MAP_BUTTON_QUAL_MINUS_X + 5, MAP_BUTTON_QUAL_MINUS_Y + 9,
    MAP_BUTTON_QUAL_MINUS_W / 2,
                     2, WHITE);
    // Sign +
    ILI9488_DrawBorder(MAP_BUTTON_QUAL_PLUS_X, MAP_BUTTON_QUAL_PLUS_Y,
    MAP_BUTTON_QUAL_PLUS_W,
                       MAP_BUTTON_QUAL_PLUS_H, 2, WHITE);
    ILI9488_FillArea(MAP_BUTTON_QUAL_PLUS_X + 5, MAP_BUTTON_QUAL_PLUS_Y + 9,
    MAP_BUTTON_QUAL_MINUS_W / 2,
                     2, WHITE);
    ILI9488_FillArea(MAP_BUTTON_QUAL_PLUS_X + 9, MAP_BUTTON_QUAL_PLUS_Y + 5, 2,
    MAP_BUTTON_QUAL_MINUS_H / 2,
                     WHITE);
}

static void _DrawPersistanceButtons(map_persistence_mode_e mode)
{
    const char *str = NULL;
    switch (mode)
    {
        default:
        case MAP_PERSIST_OFF:
            str = "OFF";
            break;
        case MAP_PERSIST_ON:
            str = "ON";
            break;
        case MAP_PERSIST_ONESHOT:
            str = "SHOT";
            break;
    }

    ILI9488_CString(ILI9488_HEIGHT - MAP_TOOLBAR_WIDTH + 1, 190, ILI9488_HEIGHT, 190, "PERSIS", Font16, 1, WHITE,
    BLACK);
    ILI9488_CString(MAP_BUTTON_PERS_MODE_X, MAP_BUTTON_PERS_MODE_Y,
    ILI9488_HEIGHT - 4,
                    MAP_BUTTON_PERS_MODE_Y + MAP_BUTTON_PERS_MODE_H - 1, str, Font16, 1,
                    WHITE,
                    MAGENTA);
    ILI9488_DrawBorder(MAP_BUTTON_PERS_MODE_X, MAP_BUTTON_PERS_MODE_Y,
    MAP_BUTTON_PERS_MODE_W,
                       MAP_BUTTON_PERS_MODE_H, 2, WHITE);

    ILI9488_CString(MAP_BUTTON_PERS_CLEAR_X, MAP_BUTTON_PERS_CLEAR_Y,
    ILI9488_HEIGHT - 4,
                    ILI9488_WIDTH - 11, "CLEAR", Font16, 1, WHITE,
                    ORANGE);
    ILI9488_DrawBorder(MAP_BUTTON_PERS_CLEAR_X, MAP_BUTTON_PERS_CLEAR_Y,
    MAP_BUTTON_PERS_CLEAR_W,
                       MAP_BUTTON_PERS_CLEAR_H, 2, WHITE);
}

static void _DrawDistanceInfo(const point_t *p1, const point_t *p2)
{
    char dist_mm[10];

    ILI9488_CString(0, 130, MAP_TOOLBAR_WIDTH, 130, "METER", Font16, 1, WHITE,
    BLACK);

    if (p1->x != map_invalid_point.x)
    {
        // Draw distance from center
        ILI9488_FillCircle(12, 159, 3, map_center_point.color);
        ILI9488_CString(0, 153, MAP_TOOLBAR_WIDTH, 153, "<->", Font16, 1, WHITE,
        BLACK);
        ILI9488_FillCircle(MAP_TOOLBAR_WIDTH - 12, 159, 3, p1->color);
        snprintf(dist_mm, sizeof(dist_mm), "%lu mm", (uint32_t) _GetDistancePoints(&map_center_point, p1));
        ILI9488_CString(0, 170, MAP_TOOLBAR_WIDTH, 182, dist_mm, Font12, 1,
        WHITE,
                        BLACK);

        if (p2->x != map_invalid_point.x)
        {
            // Draw distance from center
            ILI9488_FillCircle(12, 195, 3, map_center_point.color);
            ILI9488_CString(0, 189, MAP_TOOLBAR_WIDTH, 189, "<->", Font16, 1,
            WHITE,
                            BLACK);
            ILI9488_FillCircle(MAP_TOOLBAR_WIDTH - 12, 195, 3, p2->color);
            snprintf(dist_mm, sizeof(dist_mm), "%lu mm", (uint32_t) _GetDistancePoints(&map_center_point, p2));
            ILI9488_CString(0, 205, MAP_TOOLBAR_WIDTH, 217, dist_mm, Font12, 1,
            WHITE,
                            BLACK);

            // Draw distance from 2 selected points
            ILI9488_FillCircle(12, 230, 3, p1->color);
            ILI9488_CString(0, 224, MAP_TOOLBAR_WIDTH, 224, "<->", Font16, 1,
            WHITE,
                            BLACK);
            ILI9488_FillCircle(MAP_TOOLBAR_WIDTH - 12, 230, 3, p2->color);
            snprintf(dist_mm, sizeof(dist_mm), "%lu mm", (uint32_t) _GetDistancePoints(p1, p2));
            ILI9488_CString(0, 240, MAP_TOOLBAR_WIDTH, 252, dist_mm, Font12, 1,
            WHITE,
                            BLACK);
        }
        else
        {
            ILI9488_FillArea(0, 189, MAP_TOOLBAR_WIDTH, 60, BLACK);
        }
    }
    else
    {
        ILI9488_FillArea(0, 150, MAP_TOOLBAR_WIDTH, 100, BLACK);
    }
}

/* Return distance in mm */
static double _GetDistancePoints(const point_t *p1, const point_t *p2)
{
    return sqrt(pow(p2->x - p1->x, 2) + pow(p2->y - p1->y, 2)) / map_scale_factor;
}

/*
 * map.c
 *
 *  Created on: 28 nov. 2025
 *      Author: nicob
 */

#include <string.h>
#include <math.h>
#include <stdio.h>

#include "map.h"
#include "rplidar.h"
#include "ILI9488.h"

#define SAMPLE_BUF_SIZE 1024
#define SAMPLE_QUALITY_DEFAULT 20

#define POINT_BUF_SIZE 8192

#define MAP_SIZE ILI9488_WIDTH
#define MAP_DEFAULT_DISTANCE_MAX 1000.0f
#define MAP_DEFAULT_SCALE ((MAP_SIZE / 2) / MAP_DEFAULT_DISTANCE_MAX)

typedef struct {
	uint16_t x;
	uint16_t y;
	uint16_t color;
} point_t;

static rplidar_measurement_t map_sample_buf[SAMPLE_BUF_SIZE];
static uint16_t map_sample_write_idx = 0;
static uint16_t map_sample_read_idx = 0;
static uint16_t map_sample_count = 0;

static point_t map_point_buf[POINT_BUF_SIZE] = {0};
static uint16_t map_point_idx = 0;

static uint8_t map_quality_min = SAMPLE_QUALITY_DEFAULT;
static map_scale_mode_e map_scale_mode = MAP_SCALE_AUTO;
static double map_scale_distance_max = MAP_DEFAULT_DISTANCE_MAX;
static double map_scale_factor = MAP_DEFAULT_SCALE;

static void _ConvertSampleToPoint(rplidar_measurement_t *sample, point_t *point);
static void _DrawGrid(void);
static void _DrawMapScale(double scale);

void MAP_Init(void) {
	MAP_SetScaleMode(MAP_SCALE_AUTO);
	MAP_DrawMenu();
	RPLIDAR_StartScan(NULL, 0, 0);
}

void MAP_DrawMenu(void) {
	ILI9488_FillScreen(BLACK);
	ILI9488_FillCircle(ILI9488_HEIGHT / 2, ILI9488_WIDTH / 2, 3, RED);
	_DrawGrid();
}

void MAP_DrawSamples(void) {
	while (map_sample_count) {
		point_t* old_point = &map_point_buf[map_point_idx];
		if(old_point->x != 0 || old_point->y != 0) {
			// Remove old point from the screen
			ILI9488_Pixel(old_point->x, old_point->y, BLACK);
		}

		_ConvertSampleToPoint(&map_sample_buf[map_sample_read_idx], &map_point_buf[map_point_idx]);
		map_sample_read_idx = (map_sample_read_idx + 1) % SAMPLE_BUF_SIZE;
		map_sample_count--;

		ILI9488_Pixel(map_point_buf[map_point_idx].x, map_point_buf[map_point_idx].y, map_point_buf[map_point_idx].color);
		map_point_idx = (map_point_idx + 1) % POINT_BUF_SIZE;
	}
}

void MAP_Reset(void) {
	// Erase all points from screen
	MAP_DrawMenu();

	// Empty all buffers
	memset(&map_point_buf[0], 0, sizeof(point_t)*POINT_BUF_SIZE);
	map_point_idx = 0;

	memset(&map_sample_buf[0], 0, sizeof(rplidar_measurement_t)*SAMPLE_BUF_SIZE);
	map_sample_write_idx = 0;
	map_sample_count = 0;
}

void MAP_SetScaleMode(map_scale_mode_e mode) {
	switch(mode) {
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
}

void MAP_SetQuality(uint8_t quality) {
	map_quality_min = quality;

}

void RPLIDAR_OnSingleMeasurement(rplidar_measurement_t *measurement) {
	if (map_sample_count < SAMPLE_BUF_SIZE) {
		if ((measurement->distance != 0)
				&& (measurement->quality > map_quality_min)) {
			// Filter only valid and good quality measurement
			memcpy(&map_sample_buf[map_sample_write_idx], measurement,
					sizeof(rplidar_measurement_t));
			map_sample_write_idx = (map_sample_write_idx + 1) % SAMPLE_BUF_SIZE;
			map_sample_count++;
		}
	}
}

static void _ConvertSampleToPoint(rplidar_measurement_t *sample, point_t *point) {
	double distance_mm = sample->distance / 4.0;
	double angle_rad = (sample->angle / 64.0) * M_PI / 180;
	double y = distance_mm * cosf(angle_rad);
	double x = distance_mm * sinf(angle_rad);

	if(map_scale_mode == MAP_SCALE_AUTO) {
		if(distance_mm > map_scale_distance_max) {
			map_scale_distance_max = distance_mm;
			map_scale_factor = ((MAP_SIZE / 2) / map_scale_distance_max);
			_DrawMapScale(map_scale_distance_max / 5000.0); // To remove
		}
	}

	point->x = x * map_scale_factor + ILI9488_HEIGHT / 2;
	point->y = y * map_scale_factor + ILI9488_WIDTH / 2;
	point->color = YELLOW;
}

static void _DrawGrid(void) {
	for(uint8_t i=0; i<6; i++) {
		// For the last horizontal line, -1 pixel offset to make the line fits in the screen
		ILI9488_FillArea((ILI9488_HEIGHT - ILI9488_WIDTH) / 2, i != 5 ? (MAP_SIZE / 5)*i : MAP_SIZE-1, MAP_SIZE, 1, DDDD_WHITE);
		ILI9488_FillArea((ILI9488_HEIGHT - ILI9488_WIDTH) / 2 + (MAP_SIZE / 5)*i, 0, 1, MAP_SIZE, DDDD_WHITE);
	}
	ILI9488_FillArea((ILI9488_HEIGHT - ILI9488_WIDTH) / 2 - 25, 2, 2, MAP_SIZE / 5 - 2, WHITE);
	ILI9488_FillArea((ILI9488_HEIGHT - ILI9488_WIDTH) / 2 - 29, 0, 10, 2, WHITE);
	ILI9488_FillArea((ILI9488_HEIGHT - ILI9488_WIDTH) / 2 - 29, MAP_SIZE / 5, 10, 2, WHITE);
	_DrawMapScale(map_scale_distance_max / 5000.0);
}

static void _DrawMapScale(double scale) {
	char scale_txt[10];
	snprintf(scale_txt, sizeof(scale_txt), "%.2f m", scale);
	ILI9488_CString(0, 0, (ILI9488_HEIGHT - ILI9488_WIDTH) / 2 - 29,  MAP_SIZE / 5, scale_txt, Font12, 1, WHITE, BLACK);
}

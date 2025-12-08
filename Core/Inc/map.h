/*
 * rplidar.h
 *
 *  Created on: Nov 17, 2025
 *      Author: Nicolas BESNARD
 */

#ifndef INC_MAP_H_
#define INC_MAP_H_

#include <stdint.h>

typedef enum
{
    MAP_SCALE_AUTO, MAP_SCALE_1000, MAP_SCALE_2500, MAP_SCALE_5000, MAP_SCALE_10000, MAP_SCALE_MAX
} map_scale_mode_e;

typedef enum
{
    MAP_PERSIST_OFF, MAP_PERSIST_ON, MAP_PERSIST_ONESHOT, MAP_PERSIST_MAX
} map_persistence_mode_e;

void MAP_Show(void);
void MAP_DrawMenu(void);
void MAP_DrawSamples(void);
void MAP_Touch(uint16_t x, uint16_t y);
void MAP_SetScaleMode(map_scale_mode_e mode);
void MAP_SetQuality(uint8_t quality);
void MAP_SetPersistanceMode(map_persistence_mode_e mode);
void MAP_ClearPoints(bool erase_buffers);

#endif /* INC_MAP_H_ */

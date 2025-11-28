/*
 * rplidar.h
 *
 *  Created on: Nov 17, 2025
 *      Author: Nicolas BESNARD
 */

#ifndef INC_MAP_H_
#define INC_MAP_H_

typedef enum {
	MAP_SCALE_AUTO,
	MAP_SCALE_1000,
	MAP_SCALE_2500,
	MAP_SCALE_5000,
	MAP_SCALE_10000,
} map_scale_mode_e;

void MAP_Init(void);
void MAP_DrawMenu(void);
void MAP_DrawSamples(void);
void MAP_SetScaleMode(map_scale_mode_e mode);
void MAP_Reset(void);

#endif /* INC_MAP_H_ */

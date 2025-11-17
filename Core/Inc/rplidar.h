/*
 * rplidar.h
 *
 *  Created on: Nov 17, 2025
 *      Author: nicob
 */

#ifndef INC_RPLIDAR_H_
#define INC_RPLIDAR_H_

#include <stdint.h>

void RPLIDAR_Init(void);
void RPLIDAR_StartScan(void);
void RPLIDAR_Stop(void);
void RPLIDAR_RequestHealth(void);
void RPLIDAR_RequestDeviceInfo(void);
void RPLIDAR_OnMeasurement(float angle_deg, float distance_mm);

#endif /* INC_RPLIDAR_H_ */

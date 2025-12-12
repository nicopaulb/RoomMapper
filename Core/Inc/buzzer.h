/*
 * buzzer.h
 *
 *  Created on: 10 Dec 2025
 *      Author: nicob
 */
#ifndef INC_BUZZER_H_
#define INC_BUZZER_H_

#include "stm32f4xx_hal.h"

void Buzzer_Init(TIM_HandleTypeDef *freq_timer, uint32_t freq_channel, TIM_HandleTypeDef *duration_timer);
void Buzzer_Play(const uint16_t *tone, const uint16_t *duration, uint32_t size);
uint8_t Buzzer_GetVolume(void);
void Buzzer_SetVolume(int8_t level_perc);
void Buzzer_Play_Boot(void);
void Buzzer_Play_Menu_In(void);
void Buzzer_Play_Menu_Out(void);
void Buzzer_Play_Menu_Touch(void);

#endif /* INC_BUZZER_H_ */

/*
 * buzzer.c
 *
 *  Created on: 10 Dec 2025
 *      Author: nicob
 */
#include <math.h>
#include "buzzer.h"
#include "stm32f4xx_hal.h"

#define TIMR_FREQ 96000000
#define TIMR_ARR 1000
#define TIMR_PRS(freq) ((TIMR_FREQ/(TIMR_ARR*freq))-1)

#define MAX_VOLUME (TIMR_ARR / 2)
#define VOLUME_LEVEL_NB 4

typedef struct tone
{
    const uint16_t *frequency;
    const uint16_t *duration;
    uint32_t size;
} tone_t;

static TIM_HandleTypeDef *buzzer_timer_freq = NULL;
static TIM_HandleTypeDef *buzzer_timer_duration = NULL;
static uint32_t buzzer_timer_channel = 0;
static tone_t tone_buf;
static uint32_t tone_id;
static uint16_t volume[] = {0, 10, 100, MAX_VOLUME};
static uint8_t volume_level = 0;

static const uint16_t boot_freqs[] = {523, 659, 784, 1047};
static const uint16_t boot_durations[] = {150, 150, 150, 300};

static const uint16_t menu_in_freqs[] = {880, 988};
static const uint16_t menu_in_durations[] = {80, 100};

static const uint16_t menu_out_freqs[] = {988, 880};
static const uint16_t menu_out_durations[] = {80, 100};

static const uint16_t menu_touch_freqs[] = {600};
static const uint16_t menu_touch_durations[] = {50};

static void _Play_Tone(uint16_t frequency, uint16_t duration);

void Buzzer_Init(TIM_HandleTypeDef *freq_timer, uint32_t freq_channel, TIM_HandleTypeDef *duration_timer)
{
    buzzer_timer_freq = freq_timer;
    buzzer_timer_channel = freq_channel;
    buzzer_timer_duration = duration_timer;
}

void Buzzer_Play(const uint16_t *tone, const uint16_t *duration, uint32_t size)
{
    tone_buf.frequency = tone;
    tone_buf.duration = duration;
    tone_buf.size = size;
    tone_id = 0;

    // Play first tone
    _Play_Tone(tone_buf.frequency[tone_id], tone_buf.duration[tone_id]);
}

uint8_t Buzzer_GetVolume(void)
{
    return (volume_level * 100 / ((sizeof(volume) / sizeof(uint16_t)) - 1));
}

void Buzzer_SetVolume(int8_t level_perc)
{
    if (level_perc > 100)
    {
        level_perc = 100;
    }
    else if (level_perc < 0)
    {
        level_perc = 0;
    }

    volume_level = (level_perc * ((sizeof(volume) / sizeof(uint16_t)) - 1) / 100.0) + 0.5;
}

void Buzzer_Play_Boot(void)
{
    Buzzer_Play(boot_freqs, boot_durations, sizeof(boot_freqs) / sizeof(uint16_t));
}

void Buzzer_Play_Menu_In(void)
{
    Buzzer_Play(menu_in_freqs, menu_in_durations, sizeof(menu_in_freqs) / sizeof(uint16_t));
}

void Buzzer_Play_Menu_Out(void)
{
    Buzzer_Play(menu_out_freqs, menu_out_durations, sizeof(menu_out_freqs) / sizeof(uint16_t));
}

void Buzzer_Play_Menu_Touch(void)
{
    Buzzer_Play(menu_touch_freqs, menu_touch_durations, sizeof(menu_touch_freqs) / sizeof(uint16_t));
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == buzzer_timer_duration->Instance)
    {
        HAL_TIM_PWM_Stop(buzzer_timer_freq, buzzer_timer_channel);
        tone_id++;
        if (tone_id < tone_buf.size)
        {
            _Play_Tone(tone_buf.frequency[tone_id], tone_buf.duration[tone_id]);
        }
    }
}

static void _Play_Tone(uint16_t frequency, uint16_t duration)
{
    __HAL_TIM_SET_COUNTER(buzzer_timer_duration, duration * 2);

    if (frequency != 0)
    {
        __HAL_TIM_SET_COMPARE(buzzer_timer_freq, buzzer_timer_channel, volume[volume_level]);
        __HAL_TIM_SET_PRESCALER(buzzer_timer_freq, TIMR_PRS(frequency));
        HAL_TIM_PWM_Start(buzzer_timer_freq, buzzer_timer_channel);
    }

    __HAL_TIM_ENABLE_IT(buzzer_timer_duration, TIM_IT_UPDATE);
    __HAL_TIM_ENABLE(buzzer_timer_duration);
}

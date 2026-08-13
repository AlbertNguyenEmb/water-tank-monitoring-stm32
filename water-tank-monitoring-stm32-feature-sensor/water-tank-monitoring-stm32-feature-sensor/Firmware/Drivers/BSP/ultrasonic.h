#ifndef ULTRASONIC_H
#define ULTRASONIC_H

#include "stm32f1xx_hal.h"

#include <stdbool.h>
#include <stdint.h>

void Ultrasonic_Init(
    TIM_HandleTypeDef *htim,
    uint32_t channel,
    GPIO_TypeDef *trig_port,
    uint16_t trig_pin
);

bool Ultrasonic_Start(void);

void Ultrasonic_Abort(void);

void Ultrasonic_IC_CaptureCallback(
    TIM_HandleTypeDef *htim
);

bool Ultrasonic_IsReady(void);

bool Ultrasonic_IsValid(void);

uint32_t Ultrasonic_GetDistanceMm(void);

uint32_t Ultrasonic_GetPulseWidthUs(void);

#endif
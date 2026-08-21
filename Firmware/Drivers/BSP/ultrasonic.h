#ifndef ULTRASONIC_H
#define ULTRASONIC_H

#include "main.h"
#include "tim.h"

#include <stdbool.h>
#include <stdint.h>

#define ULTRASONIC_TRIG_PORT           GPIOA
#define ULTRASONIC_TRIG_PIN            GPIO_PIN_1
#define ULTRASONIC_ECHO_PORT           GPIOA
#define ULTRASONIC_ECHO_PIN            GPIO_PIN_0
#define ULTRASONIC_ECHO_TIMER          htim2
#define ULTRASONIC_ECHO_CHANNEL        TIM_CHANNEL_1

#define ULTRASONIC_TIMEOUT_MS          12U
#define ULTRASONIC_TIMEOUT_US          6000U
#define ULTRASONIC_MIN_STABLE_ECHO_US  0U
#define ULTRASONIC_MIN_DISTANCE_CM     0.0f
#define ULTRASONIC_MAX_DISTANCE_CM     40.0f

typedef enum
{
    ULTRASONIC_STATUS_IDLE = 0,
    ULTRASONIC_STATUS_WAIT_RISING,
    ULTRASONIC_STATUS_WAIT_FALLING,
    ULTRASONIC_STATUS_DONE,
    ULTRASONIC_STATUS_TIMEOUT,
    ULTRASONIC_STATUS_OUT_OF_RANGE,
    ULTRASONIC_STATUS_ERROR
} UltrasonicStatus_t;

void Ultrasonic_Init(void);
bool Ultrasonic_Start(void);
void Ultrasonic_Process(void);
bool Ultrasonic_MeasureDistanceCm(float *distance_cm);

bool Ultrasonic_IsBusy(void);
bool Ultrasonic_IsDistanceReady(void);
bool Ultrasonic_ReadDistanceCm(float *distance_cm);

float Ultrasonic_GetDistanceCm(void);
uint32_t Ultrasonic_GetEchoTimeUs(void);
UltrasonicStatus_t Ultrasonic_GetStatus(void);
const char *Ultrasonic_GetStatusName(UltrasonicStatus_t status);

#endif /* ULTRASONIC_H */

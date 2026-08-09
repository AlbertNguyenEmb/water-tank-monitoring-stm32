#include "logger.h"
#include "fsm.h"
#include "water_sensor.h"
#include <stdio.h>
#include "usart.h"
extern UART_HandleTypeDef huart2;

static const char* StateToString(FSM_State_t state)
{
    switch (state)
    {
    case FSM_STATE_INIT:     return "INIT";
    case FSM_STATE_FILLING:  return "FILLING";
    case FSM_STATE_FULL:     return "FULL";
    case FSM_STATE_OVERFLOW: return "OVERFLOW";
    case FSM_STATE_ERROR:    return "ERROR";
    default:                 return "UNKNOWN";
    }
}

void Logger_Init(void)
{

}

void Logger_Run(void)
{
    static uint32_t s_tick = 0;
    char line[96];

    /* Logger doc truc tiep tu WaterSensor + FSM, khong can qua Control */
    float       percent = WaterSensor_ReadPercent();
    FSM_State_t state    = FSM_GetState();

    int len = snprintf(line, sizeof(line), "%lu,%.1f,%s\r\n",
                        (unsigned long)s_tick++, percent, StateToString(state));

    if (len > 0)
    {
        HAL_UART_Transmit(&huart2, (uint8_t *)line, (uint16_t)len, 100);
    }
}
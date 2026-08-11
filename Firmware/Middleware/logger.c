#include "logger.h"
#include "fsm.h"

#include <stdio.h>
#include "usart.h"

extern UART_HandleTypeDef huart2;


static const char* StateToString(FSM_State_t state)
{
    switch (state)
    {
    case FSM_STATE_INIT:
        return "INIT";

    case FSM_STATE_FILLING:
        return "FILLING";

    case FSM_STATE_FULL:
        return "FULL";

    case FSM_STATE_OVERFLOW:
        return "OVERFLOW";

    case FSM_STATE_ERROR:
        return "ERROR";

    default:
        return "UNKNOWN";
    }
}


void Logger_Init(void)
{
    const char header[] = "time_ms,level_percent,state\r\n";

    HAL_UART_Transmit(
        &huart2,
        (uint8_t *)header,
        (uint16_t)(sizeof(header) - 1U),
        100
    );
}


void Logger_Run(void)
{
    char line[96];

    uint32_t time_ms = HAL_GetTick();

    float level_percent = FSM_GetLevelPercent();

    FSM_State_t state = FSM_GetState();

    int len = snprintf(
        line,
        sizeof(line),
        "%lu,%.1f,%s\r\n",
        (unsigned long)time_ms,
        level_percent,
        StateToString(state)
    );

    if (len > 0)
    {
        if (len >= (int)sizeof(line))
        {
            len = (int)sizeof(line) - 1;
        }

        HAL_UART_Transmit(
            &huart2,
            (uint8_t *)line,
            (uint16_t)len,
            100
        );
    }
}
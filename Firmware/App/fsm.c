#include "fsm.h"
/*
#include "control.h"
#include "water_sensor.h"
#include "relay.h"
#include "buzzer.h"
*/

static FSM_State_t currentState;

// Private funtion
static void FSM_State_Init(void);
static void FSM_State_Monitoring(void);
static void FSM_State_Filling(void);
static void FSM_State_Overflow(void);
static void FSM_State_Error(void);

static const int LOW_LEVEL_THRESHOLD = 20;
static const int OVERFLOW_THRESHOLD = 80;
static const int HIGH_LEVEL_THRESHOLD = 90;
// Public funtion
void FSM_Init(void) {
    currentState = FSM_STATE_INIT;
}

FSM_State_t FSM_GetState(void)
{
    return currentState;
}
void FSM_Run(void)
{
    switch(currentState)
    {
        case FSM_STATE_INIT:
            FSM_State_Init();
            break;
        case FSM_STATE_MONITORING:
            FSM_State_Monitoring();
            break;
        case FSM_STATE_FILLING:
            FSM_State_Filling();
            break;
        case FSM_STATE_OVERFLOW:
            FSM_State_Overflow();
            break;
        case FSM_STATE_ERROR:
            FSM_State_Error();
            break;
        default:
            currentState = FSM_STATE_ERROR;
            break;
    }
}

static void FSM_State_Init(void) {
    float level = WaterSensor_ReadPercent();

    if (!WaterSensor_IsValid()) {
        currentState = FSM_STATE_ERROR;
        return;
    }

    if(level <= LOW_LEVEL_THRESHOLD)
    {
        currentState = FSM_STATE_FILLING;
    }
    else if(level >= OVERFLOW_THRESHOLD)
    {
        currentState = FSM_STATE_OVERFLOW;
    }
    else
    {
        currentState = FSM_STATE_MONITORING;
    }
} 

static void FSM_State_Monitoring() {
    float level = WaterSensor_ReadPercent();

    if (!WaterSensor_IsValid()) {
        currentState = FSM_STATE_ERROR;
        return;
    }

    if(level <= LOW_LEVEL_THRESHOLD)
    {
        currentState = FSM_STATE_FILLING;
    }
    else if(level >= OVERFLOW_THRESHOLD)
    {
        currentState = FSM_STATE_OVERFLOW;
    }
}

static void FSM_State_FILLING() {
    float level = WaterSensor_ReadPercent();

    Relay_On();

    if(!WaterSensor_IsValid())
    {
        currentState = FSM_STATE_ERROR;
        return;
    }

    if(level >= HIGH_LEVEL_THRESHOLD)
    {
        currentState = FSM_STATE_MONITORING;
    }
}

static void FSM_State_Overflow(void)
{
    float level = WaterSensor_ReadPercent();

    Relay_Off();

    Buzzer_On();

    if(!WaterSensor_IsValid())
    {
        currentState = FSM_STATE_ERROR;
        return;
    }

    if(level < OVERFLOW_THRESHOLD)
    {
        Buzzer_Off();

        currentState = FSM_STATE_MONITORING;
    }
}

static void FSM_State_Error(void)
{
    Relay_Off();

    Buzzer_On();

    if(WaterSensor_IsValid())
    {
        Buzzer_Off();

        currentState = FSM_STATE_INIT;
    }
}
#include "fsm_logic.h"

#define FSM_DEFAULT_LOW_LEVEL_PERCENT          20.0f
#define FSM_DEFAULT_FILL_STOP_PERCENT          90.0f
#define FSM_DEFAULT_OVERFLOW_PERCENT           98.0f
#define FSM_DEFAULT_OVERFLOW_CLEAR_PERCENT     95.0f
#define FSM_DEFAULT_FILL_TIMEOUT_MS            120000U

typedef struct
{
    FsmState_t state;
    FsmError_t error;
    FsmConfig_t config;
    uint32_t state_entered_at_ms;
} FsmContext_t;

static FsmContext_t fsm;

static FsmConfig_t Fsm_DefaultConfig(void)
{
    FsmConfig_t config;

    config.low_level_percent = FSM_DEFAULT_LOW_LEVEL_PERCENT;
    config.fill_stop_percent = FSM_DEFAULT_FILL_STOP_PERCENT;
    config.overflow_percent = FSM_DEFAULT_OVERFLOW_PERCENT;
    config.overflow_clear_percent = FSM_DEFAULT_OVERFLOW_CLEAR_PERCENT;
    config.fill_timeout_ms = FSM_DEFAULT_FILL_TIMEOUT_MS;

    return config;
}

static void Fsm_EnterState(FsmState_t next_state, uint32_t now_ms)
{
    if (fsm.state == next_state)
    {
        return;
    }

    fsm.state = next_state;
    fsm.state_entered_at_ms = now_ms;

    if (next_state != FSM_STATE_ERROR)
    {
        fsm.error = FSM_ERROR_NONE;
    }
}

static bool Fsm_HasTimedOut(uint32_t now_ms, uint32_t timeout_ms)
{
    return ((uint32_t)(now_ms - fsm.state_entered_at_ms) >= timeout_ms);
}

static void Fsm_EnterError(FsmError_t error, uint32_t now_ms)
{
    fsm.error = error;
    Fsm_EnterState(FSM_STATE_ERROR, now_ms);
}

static FsmOutput_t Fsm_BuildOutput(void)
{
    FsmOutput_t output;

    output.state = fsm.state;
    output.error = fsm.error;
    output.pump_on = false;
    output.buzzer_on = false;

    switch (fsm.state)
    {
    case FSM_STATE_FILLING:
        output.pump_on = true;
        break;

    case FSM_STATE_OVERFLOW:
    case FSM_STATE_ERROR:
        output.buzzer_on = true;
        break;

    case FSM_STATE_INIT:
    case FSM_STATE_MONITORING:
    default:
        break;
    }

    return output;
}

void Fsm_Init(void)
{
    Fsm_InitWithConfig(0);
}

void Fsm_InitWithConfig(const FsmConfig_t *config)
{
    if (config == 0)
    {
        fsm.config = Fsm_DefaultConfig();
    }
    else
    {
        fsm.config = *config;
    }

    fsm.state = FSM_STATE_INIT;
    fsm.error = FSM_ERROR_NONE;
    fsm.state_entered_at_ms = 0U;
}

FsmOutput_t Fsm_Update(const FsmInput_t *input)
{
    if (input == 0)
    {
        return Fsm_BuildOutput();
    }

    switch (fsm.state)
    {
    case FSM_STATE_INIT:
        Fsm_EnterState(FSM_STATE_MONITORING, input->now_ms);
        break;

    case FSM_STATE_MONITORING:
        if (!input->sensor_ok)
        {
            Fsm_EnterError(FSM_ERROR_SENSOR, input->now_ms);
        }
        else if (input->water_level_percent >= fsm.config.overflow_percent)
        {
            Fsm_EnterState(FSM_STATE_OVERFLOW, input->now_ms);
        }
        else if (input->water_level_percent <= fsm.config.low_level_percent)
        {
            Fsm_EnterState(FSM_STATE_FILLING, input->now_ms);
        }
        break;

    case FSM_STATE_FILLING:
        if (!input->sensor_ok)
        {
            Fsm_EnterError(FSM_ERROR_SENSOR, input->now_ms);
        }
        else if (input->water_level_percent >= fsm.config.fill_stop_percent)
        {
            Fsm_EnterState(FSM_STATE_MONITORING, input->now_ms);
        }
        else if (Fsm_HasTimedOut(input->now_ms, fsm.config.fill_timeout_ms))
        {
            Fsm_EnterError(FSM_ERROR_FILL_TIMEOUT, input->now_ms);
        }
        break;

    case FSM_STATE_OVERFLOW:
        if (!input->sensor_ok)
        {
            Fsm_EnterError(FSM_ERROR_SENSOR, input->now_ms);
        }
        else if (input->water_level_percent <= fsm.config.overflow_clear_percent)
        {
            Fsm_EnterState(FSM_STATE_MONITORING, input->now_ms);
        }
        break;

    case FSM_STATE_ERROR:
        if (input->user_reset ||
            ((fsm.error == FSM_ERROR_SENSOR) && input->sensor_ok))
        {
            Fsm_EnterState(FSM_STATE_MONITORING, input->now_ms);
        }
        break;

    default:
        Fsm_Init();
        Fsm_EnterState(FSM_STATE_MONITORING, input->now_ms);
        break;
    }

    return Fsm_BuildOutput();
}

FsmState_t Fsm_GetState(void)
{
    return fsm.state;
}

FsmError_t Fsm_GetError(void)
{
    return fsm.error;
}

const char *Fsm_GetStateName(FsmState_t state)
{
    switch (state)
    {
    case FSM_STATE_INIT:
        return "INIT";

    case FSM_STATE_MONITORING:
        return "MONITORING";

    case FSM_STATE_FILLING:
        return "FILLING";

    case FSM_STATE_OVERFLOW:
        return "OVERFLOW";

    case FSM_STATE_ERROR:
        return "ERROR";

    default:
        return "UNKNOWN";
    }
}

const char *Fsm_GetErrorName(FsmError_t error)
{
    switch (error)
    {
    case FSM_ERROR_NONE:
        return "NONE";

    case FSM_ERROR_SENSOR:
        return "SENSOR";

    case FSM_ERROR_FILL_TIMEOUT:
        return "FILL_TIMEOUT";

    default:
        return "UNKNOWN";
    }
}

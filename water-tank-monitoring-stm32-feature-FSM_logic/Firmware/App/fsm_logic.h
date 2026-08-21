#ifndef FSM_LOGIC_H
#define FSM_LOGIC_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    FSM_STATE_INIT = 0,
    FSM_STATE_MONITORING,
    FSM_STATE_FILLING,
    FSM_STATE_OVERFLOW,
    FSM_STATE_ERROR
} FsmState_t;

typedef enum
{
    FSM_ERROR_NONE = 0,
    FSM_ERROR_SENSOR,
    FSM_ERROR_FILL_TIMEOUT
} FsmError_t;

typedef struct
{
    float low_level_percent;
    float fill_stop_percent;
    float overflow_percent;
    float overflow_clear_percent;
    uint32_t fill_timeout_ms;
} FsmConfig_t;

typedef struct
{
    float water_level_percent;
    bool sensor_ok;
    bool user_reset;
    uint32_t now_ms;
} FsmInput_t;

typedef struct
{
    FsmState_t state;
    FsmError_t error;
    bool pump_on;
    bool buzzer_on;
} FsmOutput_t;

void Fsm_Init(void);
void Fsm_InitWithConfig(const FsmConfig_t *config);
FsmOutput_t Fsm_Update(const FsmInput_t *input);

FsmState_t Fsm_GetState(void);
FsmError_t Fsm_GetError(void);

const char *Fsm_GetStateName(FsmState_t state);
const char *Fsm_GetErrorName(FsmError_t error);

#endif /* FSM_LOGIC_H */

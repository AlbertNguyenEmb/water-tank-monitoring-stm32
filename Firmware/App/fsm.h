#ifndef FSM_H
#define FSM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/*=============================
 * FSM States
 *=============================*/

typedef enum
{
    FSM_STATE_INIT = 0,

    FSM_STATE_MONITORING,

    FSM_STATE_FILLING,

    FSM_STATE_OVERFLOW,

    FSM_STATE_ERROR

} FSM_State_t;

/*=============================
 * API
 *=============================*/

void FSM_Init(void);
void FSM_Run(void);
FSM_State_t FSM_GetState(void);

#ifdef __cplusplus
}
#endif
#endif
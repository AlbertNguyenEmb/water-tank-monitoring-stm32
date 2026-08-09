#include "app.h"

#include "fsm.h"

void App_Init(void)
{
    /* Initialize Application Modules */
    FSM_Init();
}

void App_Run(void)
{
    /* Run Application */
    FSM_Run();
}
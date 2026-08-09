#include "buzzer.h"
#include "main.h"

/* Active buzzer nối với PA8 */
#define BUZZER_PORT          GPIOA
#define BUZZER_PIN           GPIO_PIN_8

/*
 * 1: Buzzer kích mức cao
 * 0: Buzzer kích mức thấp
 */
#define BUZZER_ACTIVE_HIGH   1U

static GPIO_PinState Buzzer_GetOnLevel(void)
{
#if BUZZER_ACTIVE_HIGH
    return GPIO_PIN_SET;
#else
    return GPIO_PIN_RESET;
#endif
}

static GPIO_PinState Buzzer_GetOffLevel(void)
{
#if BUZZER_ACTIVE_HIGH
    return GPIO_PIN_RESET;
#else
    return GPIO_PIN_SET;
#endif
}

void Buzzer_On(void)
{
    HAL_GPIO_WritePin(
        BUZZER_PORT,
        BUZZER_PIN,
        Buzzer_GetOnLevel()
    );
}

void Buzzer_Off(void)
{
    HAL_GPIO_WritePin(
        BUZZER_PORT,
        BUZZER_PIN,
        Buzzer_GetOffLevel()
    );
}
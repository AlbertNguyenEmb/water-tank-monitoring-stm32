#include "buzzer.h"

/*
 * Doi thanh 1 neu mach buzzer kich muc LOW.
 * Doi thanh 0 neu buzzer kich muc HIGH.
 */
#define BUZZER_ACTIVE_LOW   0

static bool buzzer_state = false;

void Buzzer_Init(void)
{
    buzzer_state = false;
    Buzzer_Off();
}

void Buzzer_On(void)
{
#if BUZZER_ACTIVE_LOW
    HAL_GPIO_WritePin(
        BUZZER_GPIO_Port,
        BUZZER_Pin,
        GPIO_PIN_RESET
    );
#else
    HAL_GPIO_WritePin(
        BUZZER_GPIO_Port,
        BUZZER_Pin,
        GPIO_PIN_SET
    );
#endif

    buzzer_state = true;
}

void Buzzer_Off(void)
{
#if BUZZER_ACTIVE_LOW
    HAL_GPIO_WritePin(
        BUZZER_GPIO_Port,
        BUZZER_Pin,
        GPIO_PIN_SET
    );
#else
    HAL_GPIO_WritePin(
        BUZZER_GPIO_Port,
        BUZZER_Pin,
        GPIO_PIN_RESET
    );
#endif

    buzzer_state = false;
}

void Buzzer_Toggle(void)
{
    if (buzzer_state)
    {
        Buzzer_Off();
    }
    else
    {
        Buzzer_On();
    }
}

void Buzzer_Set(bool on)
{
    if (on)
    {
        Buzzer_On();
    }
    else
    {
        Buzzer_Off();
    }
}

bool Buzzer_IsOn(void)
{
    return buzzer_state;
}

#include "ln298n.h"

static bool l298n_state = false;

void L298N_Init(void)
{
    l298n_state = false;
    L298N_Off();
}

void L298N_On(void)
{
    /* Forward drive for pump demo: IN1=HIGH, IN2=LOW, ENA jumpered/high. */
    HAL_GPIO_WritePin(IN1_GPIO_Port, IN1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(IN2_GPIO_Port, IN2_Pin, GPIO_PIN_RESET);
    l298n_state = true;
}

void L298N_Off(void)
{
    HAL_GPIO_WritePin(IN1_GPIO_Port, IN1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(IN2_GPIO_Port, IN2_Pin, GPIO_PIN_RESET);
    l298n_state = false;
}

void L298N_Toggle(void)
{
    if (l298n_state)
    {
        L298N_Off();
    }
    else
    {
        L298N_On();
    }
}

bool L298N_IsOn(void)
{
    return l298n_state;
}

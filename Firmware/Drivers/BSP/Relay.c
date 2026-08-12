#include "relay.h"
#include "stdint.h"

#define RELAY_GPIO_PORT    GPIOB
#define RELAY_GPIO_PIN     GPIO_PIN_0

/*
 * Đổi thành 1 nếu relay của bạn kích mức LOW.
 * Đổi thành 0 nếu relay kích mức HIGH.
 */
#define RELAY_ACTIVE_LOW   0

static bool relay_state = false;


void Relay_Init(void)
{
    relay_state = false;

    Relay_Off();
}


void Relay_On(void)
{
#if RELAY_ACTIVE_LOW
    HAL_GPIO_WritePin(
        RELAY_GPIO_PORT,
        RELAY_GPIO_PIN,
        GPIO_PIN_RESET
    );
#else
    HAL_GPIO_WritePin(
        RELAY_GPIO_PORT,
        RELAY_GPIO_PIN,
        GPIO_PIN_SET
    );
#endif

    relay_state = true;
}


void Relay_Off(void)
{
#if RELAY_ACTIVE_LOW
    HAL_GPIO_WritePin(
        RELAY_GPIO_PORT,
        RELAY_GPIO_PIN,
        GPIO_PIN_SET
    );
#else
    HAL_GPIO_WritePin(
        RELAY_GPIO_PORT,
        RELAY_GPIO_PIN,
        GPIO_PIN_RESET
    );
#endif

    relay_state = false;
}


void Relay_Toggle(void)
{
    if (relay_state)
    {
        Relay_Off();
    }
    else
    {
        Relay_On();
    }
}


bool Relay_IsOn(void)
{
    return relay_state;
}
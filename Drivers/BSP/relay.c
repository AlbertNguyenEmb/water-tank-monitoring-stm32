#include "relay.h"
#include "main.h"

#define RELAY_PORT         GPIOB
#define RELAY_PIN          GPIO_PIN_1

/*
 * 1: Relay kích mức thấp
 * 0: Relay kích mức cao
 */
#define RELAY_ACTIVE_LOW   1U

static bool relay_state = false;

static GPIO_PinState Relay_GetOnLevel(void)
{
#if RELAY_ACTIVE_LOW
    return GPIO_PIN_RESET;
#else
    return GPIO_PIN_SET;
#endif
}

static GPIO_PinState Relay_GetOffLevel(void)
{
#if RELAY_ACTIVE_LOW
    return GPIO_PIN_SET;
#else
    return GPIO_PIN_RESET;
#endif
}

void Relay_Init(void)
{
    Relay_Off();
}

void Relay_On(void)
{
    HAL_GPIO_WritePin(
        RELAY_PORT,
        RELAY_PIN,
        Relay_GetOnLevel()
    );

    relay_state = true;
}

void Relay_Off(void)
{
    HAL_GPIO_WritePin(
        RELAY_PORT,
        RELAY_PIN,
        Relay_GetOffLevel()
    );

    relay_state = false;
}

bool Relay_GetState(void)
{
    return relay_state;
}
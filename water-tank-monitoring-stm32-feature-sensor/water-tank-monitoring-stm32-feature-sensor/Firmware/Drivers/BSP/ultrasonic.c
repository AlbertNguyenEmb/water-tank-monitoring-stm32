#include "ultrasonic.h"

#define ULTRASONIC_MIN_DISTANCE_MM   20U
#define ULTRASONIC_MAX_DISTANCE_MM   4000U

typedef enum
{
    ULTRASONIC_IDLE = 0,
    ULTRASONIC_WAIT_RISING,
    ULTRASONIC_WAIT_FALLING,
    ULTRASONIC_DONE
} Ultrasonic_State_t;

static TIM_HandleTypeDef *s_htim = NULL;
static uint32_t s_channel = 0U;

static GPIO_TypeDef *s_trig_port = NULL;
static uint16_t s_trig_pin = 0U;

static volatile Ultrasonic_State_t s_state =
    ULTRASONIC_IDLE;

static volatile bool s_ready = false;
static volatile bool s_valid = false;

static volatile uint32_t s_rising = 0U;
static volatile uint32_t s_falling = 0U;

static volatile uint32_t s_pulse_us = 0U;
static volatile uint32_t s_distance_mm = 0U;


static void DWT_Init(void)
{
    CoreDebug->DEMCR |=
        CoreDebug_DEMCR_TRCENA_Msk;

    DWT->CYCCNT = 0U;

    DWT->CTRL |=
        DWT_CTRL_CYCCNTENA_Msk;
}


static void DelayUs(uint32_t us)
{
    uint32_t start =
        DWT->CYCCNT;

    uint32_t ticks =
        us *
        (SystemCoreClock / 1000000U);

    while ((DWT->CYCCNT - start) < ticks)
    {
    }
}


static void SetCaptureRising(void)
{
    __HAL_TIM_SET_CAPTUREPOLARITY(
        s_htim,
        s_channel,
        TIM_INPUTCHANNELPOLARITY_RISING
    );
}


static void SetCaptureFalling(void)
{
    __HAL_TIM_SET_CAPTUREPOLARITY(
        s_htim,
        s_channel,
        TIM_INPUTCHANNELPOLARITY_FALLING
    );
}


void Ultrasonic_Init(
    TIM_HandleTypeDef *htim,
    uint32_t channel,
    GPIO_TypeDef *trig_port,
    uint16_t trig_pin
)
{
    s_htim = htim;
    s_channel = channel;

    s_trig_port = trig_port;
    s_trig_pin = trig_pin;

    s_state = ULTRASONIC_IDLE;

    s_ready = false;
    s_valid = false;

    s_rising = 0U;
    s_falling = 0U;

    s_pulse_us = 0U;
    s_distance_mm = 0U;

    HAL_GPIO_WritePin(
        s_trig_port,
        s_trig_pin,
        GPIO_PIN_RESET
    );

    DWT_Init();

    SetCaptureRising();

    HAL_TIM_IC_Start_IT(
        s_htim,
        s_channel
    );
}


bool Ultrasonic_Start(void)
{
    if ((s_state == ULTRASONIC_WAIT_RISING) ||
        (s_state == ULTRASONIC_WAIT_FALLING))
    {
        return false;
    }

    s_ready = false;
    s_valid = false;

    s_pulse_us = 0U;
    s_distance_mm = 0U;

    SetCaptureRising();

    __HAL_TIM_SET_COUNTER(
        s_htim,
        0U
    );

    s_state =
        ULTRASONIC_WAIT_RISING;


    HAL_GPIO_WritePin(
        s_trig_port,
        s_trig_pin,
        GPIO_PIN_RESET
    );

    DelayUs(2U);


    HAL_GPIO_WritePin(
        s_trig_port,
        s_trig_pin,
        GPIO_PIN_SET
    );

    DelayUs(10U);


    HAL_GPIO_WritePin(
        s_trig_port,
        s_trig_pin,
        GPIO_PIN_RESET
    );


    return true;
}


void Ultrasonic_Abort(void)
{
    SetCaptureRising();

    s_state = ULTRASONIC_IDLE;

    s_ready = false;
    s_valid = false;

    HAL_GPIO_WritePin(
        s_trig_port,
        s_trig_pin,
        GPIO_PIN_RESET
    );
}


void Ultrasonic_IC_CaptureCallback(
    TIM_HandleTypeDef *htim
)
{
    if (htim != s_htim)
    {
        return;
    }

    if (htim->Channel !=
        HAL_TIM_ACTIVE_CHANNEL_1)
    {
        return;
    }


    /*
     * Rising edge
     */
    if (s_state ==
        ULTRASONIC_WAIT_RISING)
    {
        s_rising =
            HAL_TIM_ReadCapturedValue(
                s_htim,
                s_channel
            );

        SetCaptureFalling();

        s_state =
            ULTRASONIC_WAIT_FALLING;

        return;
    }


    /*
     * Falling edge
     */
    if (s_state ==
        ULTRASONIC_WAIT_FALLING)
    {
        s_falling =
            HAL_TIM_ReadCapturedValue(
                s_htim,
                s_channel
            );


        uint32_t period =
            __HAL_TIM_GET_AUTORELOAD(
                s_htim
            );


        if (s_falling >= s_rising)
        {
            s_pulse_us =
                s_falling -
                s_rising;
        }
        else
        {
            s_pulse_us =
                (period + 1U - s_rising)
                +
                s_falling;
        }


        /*
         * Speed of sound:
         *
         * ~343 mm/ms
         * = 0.343 mm/us
         *
         * distance =
         * pulse * 343 / 2000
         */
        s_distance_mm =
            (s_pulse_us * 343U + 1000U)
            /
            2000U;


        if ((s_distance_mm >=
             ULTRASONIC_MIN_DISTANCE_MM) &&
            (s_distance_mm <=
             ULTRASONIC_MAX_DISTANCE_MM))
        {
            s_valid = true;
        }
        else
        {
            s_valid = false;
        }


        s_ready = true;

        s_state =
            ULTRASONIC_DONE;

        SetCaptureRising();
    }
}


bool Ultrasonic_IsReady(void)
{
    return s_ready;
}


bool Ultrasonic_IsValid(void)
{
    return s_valid;
}


uint32_t Ultrasonic_GetDistanceMm(void)
{
    return s_distance_mm;
}


uint32_t Ultrasonic_GetPulseWidthUs(void)
{
    return s_pulse_us;
}
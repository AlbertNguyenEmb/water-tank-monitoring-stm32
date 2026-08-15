#include "ultrasonic.h"

typedef struct
{
    volatile UltrasonicStatus_t status;
    volatile bool distance_ready;
    volatile uint32_t echo_start_us;
    volatile uint32_t echo_time_us;
    volatile uint32_t measurement_started_ms;
    float distance_cm;
} UltrasonicContext_t;

static UltrasonicContext_t ultrasonic;

static void Ultrasonic_DelayUs(uint16_t delay_us)
{
    __HAL_TIM_SET_COUNTER(&ULTRASONIC_ECHO_TIMER, 0U);
    __HAL_TIM_ENABLE(&ULTRASONIC_ECHO_TIMER);

    while (__HAL_TIM_GET_COUNTER(&ULTRASONIC_ECHO_TIMER) < delay_us)
    {
    }
}

static void Ultrasonic_StartUsCounter(void)
{
    __HAL_TIM_SET_COUNTER(&ULTRASONIC_ECHO_TIMER, 0U);
    __HAL_TIM_ENABLE(&ULTRASONIC_ECHO_TIMER);
}

static bool Ultrasonic_HasTimedOutUs(uint32_t timeout_us)
{
    return (__HAL_TIM_GET_COUNTER(&ULTRASONIC_ECHO_TIMER) >= timeout_us);
}

static bool Ultrasonic_WaitEchoState(GPIO_PinState state, uint32_t timeout_us)
{
    uint32_t started_ms = HAL_GetTick();
    uint32_t timeout_ms = (timeout_us / 1000U) + 2U;

    Ultrasonic_StartUsCounter();

    while (HAL_GPIO_ReadPin(ULTRASONIC_ECHO_PORT, ULTRASONIC_ECHO_PIN) != state)
    {
        if (Ultrasonic_HasTimedOutUs(timeout_us))
        {
            return false;
        }

        if ((uint32_t)(HAL_GetTick() - started_ms) >= timeout_ms)
        {
            return false;
        }
    }

    return true;
}

static void Ultrasonic_SetCapturePolarity(uint32_t polarity)
{
    TIM_IC_InitTypeDef config = {0};

    HAL_TIM_IC_Stop_IT(
        &ULTRASONIC_ECHO_TIMER,
        ULTRASONIC_ECHO_CHANNEL
    );

    config.ICPolarity = polarity;
    config.ICSelection = TIM_ICSELECTION_DIRECTTI;
    config.ICPrescaler = TIM_ICPSC_DIV1;
    config.ICFilter = 0U;

    (void)HAL_TIM_IC_ConfigChannel(
        &ULTRASONIC_ECHO_TIMER,
        &config,
        ULTRASONIC_ECHO_CHANNEL
    );
}

static void Ultrasonic_StopCapture(void)
{
    HAL_TIM_IC_Stop_IT(
        &ULTRASONIC_ECHO_TIMER,
        ULTRASONIC_ECHO_CHANNEL
    );

    Ultrasonic_SetCapturePolarity(TIM_INPUTCHANNELPOLARITY_RISING);
}

static bool Ultrasonic_IsValidDistance(float distance_cm)
{
    return ((distance_cm >= ULTRASONIC_MIN_DISTANCE_CM) &&
            (distance_cm <= ULTRASONIC_MAX_DISTANCE_CM));
}

static void Ultrasonic_SendTriggerPulse(void)
{
    HAL_GPIO_WritePin(
        ULTRASONIC_TRIG_PORT,
        ULTRASONIC_TRIG_PIN,
        GPIO_PIN_RESET
    );
    Ultrasonic_DelayUs(2U);

    HAL_GPIO_WritePin(
        ULTRASONIC_TRIG_PORT,
        ULTRASONIC_TRIG_PIN,
        GPIO_PIN_SET
    );
    Ultrasonic_DelayUs(10U);

    HAL_GPIO_WritePin(
        ULTRASONIC_TRIG_PORT,
        ULTRASONIC_TRIG_PIN,
        GPIO_PIN_RESET
    );
}

void Ultrasonic_Init(void)
{
    (void)HAL_TIM_Base_Start(&ULTRASONIC_ECHO_TIMER);

    HAL_GPIO_WritePin(
        ULTRASONIC_TRIG_PORT,
        ULTRASONIC_TRIG_PIN,
        GPIO_PIN_RESET
    );

    Ultrasonic_SetCapturePolarity(TIM_INPUTCHANNELPOLARITY_RISING);

    ultrasonic.status = ULTRASONIC_STATUS_IDLE;
    ultrasonic.distance_ready = false;
    ultrasonic.echo_start_us = 0U;
    ultrasonic.echo_time_us = 0U;
    ultrasonic.measurement_started_ms = 0U;
    ultrasonic.distance_cm = 0.0f;
}

bool Ultrasonic_Start(void)
{
    if (Ultrasonic_IsBusy())
    {
        return false;
    }

    ultrasonic.status = ULTRASONIC_STATUS_WAIT_RISING;
    ultrasonic.distance_ready = false;
    ultrasonic.echo_start_us = 0U;
    ultrasonic.echo_time_us = 0U;
    ultrasonic.measurement_started_ms = HAL_GetTick();

    Ultrasonic_SetCapturePolarity(TIM_INPUTCHANNELPOLARITY_RISING);

    HAL_GPIO_WritePin(
        ULTRASONIC_TRIG_PORT,
        ULTRASONIC_TRIG_PIN,
        GPIO_PIN_RESET
    );
    Ultrasonic_DelayUs(2U);

    HAL_GPIO_WritePin(
        ULTRASONIC_TRIG_PORT,
        ULTRASONIC_TRIG_PIN,
        GPIO_PIN_SET
    );
    Ultrasonic_DelayUs(10U);

    HAL_GPIO_WritePin(
        ULTRASONIC_TRIG_PORT,
        ULTRASONIC_TRIG_PIN,
        GPIO_PIN_RESET
    );

    __HAL_TIM_SET_COUNTER(&ULTRASONIC_ECHO_TIMER, 0U);

    if (HAL_TIM_IC_Start_IT(
            &ULTRASONIC_ECHO_TIMER,
            ULTRASONIC_ECHO_CHANNEL) != HAL_OK)
    {
        ultrasonic.status = ULTRASONIC_STATUS_ERROR;
        return false;
    }

    return true;
}

void Ultrasonic_Process(void)
{
    if (!Ultrasonic_IsBusy())
    {
        return;
    }

    if ((uint32_t)(HAL_GetTick() - ultrasonic.measurement_started_ms) >=
        ULTRASONIC_TIMEOUT_MS)
    {
        Ultrasonic_StopCapture();
        ultrasonic.status = ULTRASONIC_STATUS_TIMEOUT;
    }
}

bool Ultrasonic_MeasureDistanceCm(float *distance_cm)
{
    uint32_t pulse_start_us;
    uint32_t echo_time_us;
    uint32_t last_short_echo_us = 0U;
    float measured_distance_cm;

    if (distance_cm == 0)
    {
        return false;
    }

    HAL_TIM_IC_Stop_IT(
        &ULTRASONIC_ECHO_TIMER,
        ULTRASONIC_ECHO_CHANNEL
    );

    __HAL_TIM_ENABLE(&ULTRASONIC_ECHO_TIMER);

    ultrasonic.distance_ready = false;
    ultrasonic.echo_start_us = 0U;
    ultrasonic.echo_time_us = 0U;
    ultrasonic.status = ULTRASONIC_STATUS_WAIT_RISING;

    if (!Ultrasonic_WaitEchoState(GPIO_PIN_RESET, ULTRASONIC_TIMEOUT_US))
    {
        ultrasonic.status = ULTRASONIC_STATUS_TIMEOUT;
        return false;
    }

    Ultrasonic_SendTriggerPulse();
    Ultrasonic_StartUsCounter();

    while (!Ultrasonic_HasTimedOutUs(ULTRASONIC_TIMEOUT_US))
    {
        ultrasonic.status = ULTRASONIC_STATUS_WAIT_RISING;

        while (HAL_GPIO_ReadPin(ULTRASONIC_ECHO_PORT, ULTRASONIC_ECHO_PIN) == GPIO_PIN_RESET)
        {
            if (Ultrasonic_HasTimedOutUs(ULTRASONIC_TIMEOUT_US))
            {
                ultrasonic.status =
                    (last_short_echo_us > 0U) ?
                    ULTRASONIC_STATUS_OUT_OF_RANGE :
                    ULTRASONIC_STATUS_TIMEOUT;
                ultrasonic.echo_time_us = last_short_echo_us;
                return false;
            }
        }

        pulse_start_us = __HAL_TIM_GET_COUNTER(&ULTRASONIC_ECHO_TIMER);
        ultrasonic.status = ULTRASONIC_STATUS_WAIT_FALLING;

        while (HAL_GPIO_ReadPin(ULTRASONIC_ECHO_PORT, ULTRASONIC_ECHO_PIN) == GPIO_PIN_SET)
        {
            if (Ultrasonic_HasTimedOutUs(ULTRASONIC_TIMEOUT_US))
            {
                ultrasonic.status = ULTRASONIC_STATUS_TIMEOUT;
                ultrasonic.echo_time_us =
                    __HAL_TIM_GET_COUNTER(&ULTRASONIC_ECHO_TIMER) - pulse_start_us;
                return false;
            }
        }

        echo_time_us = __HAL_TIM_GET_COUNTER(&ULTRASONIC_ECHO_TIMER) - pulse_start_us;
        ultrasonic.echo_time_us = echo_time_us;

#if (ULTRASONIC_MIN_STABLE_ECHO_US > 0U)
        if (echo_time_us < ULTRASONIC_MIN_STABLE_ECHO_US)
        {
            last_short_echo_us = echo_time_us;
            continue;
        }
#endif

        measured_distance_cm = ((float)echo_time_us * 0.0343f) / 2.0f;

        if (!Ultrasonic_IsValidDistance(measured_distance_cm))
        {
            ultrasonic.status = ULTRASONIC_STATUS_OUT_OF_RANGE;
            return false;
        }

        ultrasonic.distance_cm = measured_distance_cm;
        ultrasonic.status = ULTRASONIC_STATUS_DONE;
        *distance_cm = measured_distance_cm;

        return true;
    }

    ultrasonic.status =
        (last_short_echo_us > 0U) ?
        ULTRASONIC_STATUS_OUT_OF_RANGE :
        ULTRASONIC_STATUS_TIMEOUT;
    ultrasonic.echo_time_us = last_short_echo_us;

    return false;
}

bool Ultrasonic_IsBusy(void)
{
    return ((ultrasonic.status == ULTRASONIC_STATUS_WAIT_RISING) ||
            (ultrasonic.status == ULTRASONIC_STATUS_WAIT_FALLING));
}

bool Ultrasonic_IsDistanceReady(void)
{
    return ultrasonic.distance_ready;
}

bool Ultrasonic_ReadDistanceCm(float *distance_cm)
{
    if ((distance_cm == 0) || !ultrasonic.distance_ready)
    {
        return false;
    }

    *distance_cm = ultrasonic.distance_cm;
    ultrasonic.distance_ready = false;

    return true;
}

float Ultrasonic_GetDistanceCm(void)
{
    return ultrasonic.distance_cm;
}

uint32_t Ultrasonic_GetEchoTimeUs(void)
{
    return ultrasonic.echo_time_us;
}

UltrasonicStatus_t Ultrasonic_GetStatus(void)
{
    return ultrasonic.status;
}

const char *Ultrasonic_GetStatusName(UltrasonicStatus_t status)
{
    switch (status)
    {
    case ULTRASONIC_STATUS_IDLE:
        return "IDLE";

    case ULTRASONIC_STATUS_WAIT_RISING:
        return "WAIT_RISING";

    case ULTRASONIC_STATUS_WAIT_FALLING:
        return "WAIT_FALLING";

    case ULTRASONIC_STATUS_DONE:
        return "DONE";

    case ULTRASONIC_STATUS_TIMEOUT:
        return "TIMEOUT";

    case ULTRASONIC_STATUS_OUT_OF_RANGE:
        return "OUT_OF_RANGE";

    case ULTRASONIC_STATUS_ERROR:
        return "ERROR";

    default:
        return "UNKNOWN";
    }
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    uint32_t capture_us;
    uint32_t pulse_width_us;
    float distance_cm;

    if ((htim->Instance != ULTRASONIC_ECHO_TIMER.Instance) ||
        (htim->Channel != HAL_TIM_ACTIVE_CHANNEL_1))
    {
        return;
    }

    capture_us = HAL_TIM_ReadCapturedValue(
        htim,
        ULTRASONIC_ECHO_CHANNEL
    );

    if (ultrasonic.status == ULTRASONIC_STATUS_WAIT_RISING)
    {
        ultrasonic.echo_start_us = capture_us;
        ultrasonic.status = ULTRASONIC_STATUS_WAIT_FALLING;

        Ultrasonic_SetCapturePolarity(TIM_INPUTCHANNELPOLARITY_FALLING);
        (void)HAL_TIM_IC_Start_IT(
            &ULTRASONIC_ECHO_TIMER,
            ULTRASONIC_ECHO_CHANNEL
        );

        return;
    }

    if (ultrasonic.status == ULTRASONIC_STATUS_WAIT_FALLING)
    {
        if (capture_us >= ultrasonic.echo_start_us)
        {
            pulse_width_us = capture_us - ultrasonic.echo_start_us;
        }
        else
        {
            pulse_width_us =
                (uint32_t)(ULTRASONIC_ECHO_TIMER.Init.Period + 1U) -
                ultrasonic.echo_start_us +
                capture_us;
        }

        ultrasonic.echo_time_us = pulse_width_us;
        distance_cm = ((float)pulse_width_us * 0.0343f) / 2.0f;

        if (Ultrasonic_IsValidDistance(distance_cm))
        {
            ultrasonic.distance_cm = distance_cm;
            ultrasonic.distance_ready = true;
            ultrasonic.status = ULTRASONIC_STATUS_DONE;
        }
        else
        {
            ultrasonic.status = ULTRASONIC_STATUS_OUT_OF_RANGE;
        }

        Ultrasonic_StopCapture();
    }
}

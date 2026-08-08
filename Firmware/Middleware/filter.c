#include "filter.h"


 /*================================================================
 * ADC_RAW_AT_EMPTY : gia tri ADC do duoc khi be can (0%)
 * ADC_RAW_AT_FULL  : gia tri ADC do duoc khi be day  (100%)
 * ================================================================ */
#define ADC_RAW_AT_EMPTY    200.0f
#define ADC_RAW_AT_FULL     3800.0f

static float   s_buffer[FILTER_WINDOW_SIZE];
static uint8_t s_index  = 0U;
static uint8_t s_filled = 0U; /* =1 khi bo dem da day it nhat 1 vong */

/**
 * @brief  Trung binh dong (moving average) tren FILTER_WINDOW_SIZE mau gan nhat.
 */
static float Filter_MovingAverage(float new_sample)
{
    s_buffer[s_index] = new_sample;
    s_index = (uint8_t)((s_index + 1U) % FILTER_WINDOW_SIZE);
    if (!s_filled && (s_index == 0U))
    {
        s_filled = 1U;
    }

    uint8_t count = s_filled ? FILTER_WINDOW_SIZE : s_index;
    if (count == 0U) { count = 1U; } /* tranh chia cho 0 ngay lan doc dau tien */

    float sum = 0.0f;
    for (uint8_t i = 0U; i < count; i++)
    {
        sum += s_buffer[i];
    }
    return sum / (float)count;
}

/**
 * @brief  Quy doi gia tri ADC (da loc) sang % muc nuoc, gioi han [0,100].
 */
static float Filter_AdcToPercent(float adc_value)
{
    float percent = (adc_value - ADC_RAW_AT_EMPTY) * 100.0f /
                     (ADC_RAW_AT_FULL - ADC_RAW_AT_EMPTY);

    if (percent < 0.0f)   { percent = 0.0f; }
    if (percent > 100.0f) { percent = 100.0f; }
    return percent;
}

void Filter_Init(void)
{
    for (uint8_t i = 0U; i < FILTER_WINDOW_SIZE; i++)
    {
        s_buffer[i] = 0.0f;
    }
    s_index  = 0U;
    s_filled = 0U;
}

float Filter_Update(float input)
{
    float smoothed_adc = Filter_MovingAverage(input);
    return Filter_AdcToPercent(smoothed_adc);
}
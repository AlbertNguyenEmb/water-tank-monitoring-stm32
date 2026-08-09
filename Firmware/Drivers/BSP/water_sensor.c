#include "water_sensor.h"
#include "main.h"

#include <stdint.h>
#include <stdbool.h>

extern ADC_HandleTypeDef hadc1;

/* =========================
 * ADC calibration values
 * ========================= */
#define ADC_VAL_EMPTY          500U
#define ADC_VAL_FULL           3200U

#define ADC_ERR_LOWER          50U
#define ADC_ERR_UPPER          4050U

#define ADC_TIMEOUT_MS         10U

/* Lấy 10 mẫu mỗi lần đọc */
#define ADC_SAMPLE_COUNT       10U


/* =========================
 * Internal state
 * ========================= */
static bool sensor_initialized = false;
static bool sensor_valid = false;

static uint32_t current_adc_value = 0U;
static float current_percent = -1.0f;


/* =========================
 * Clamp
 * ========================= */
static float WaterSensor_Clamp(
    float value,
    float minimum,
    float maximum
)
{
    if (value < minimum)
    {
        return minimum;
    }

    if (value > maximum)
    {
        return maximum;
    }

    return value;
}


/* =========================
 * Đọc 1 mẫu ADC
 * ========================= */
static bool WaterSensor_ReadAdcSample(uint32_t *adc_value)
{
    if (adc_value == NULL)
    {
        return false;
    }

    /*
     * Bắt đầu ADC conversion
     */
    if (HAL_ADC_Start(&hadc1) != HAL_OK)
    {
        return false;
    }

    /*
     * Chờ ADC conversion hoàn thành
     */
    if (HAL_ADC_PollForConversion(
            &hadc1,
            ADC_TIMEOUT_MS
        ) != HAL_OK)
    {
        HAL_ADC_Stop(&hadc1);
        return false;
    }

    /*
     * Lấy giá trị ADC
     */
    *adc_value = HAL_ADC_GetValue(&hadc1);

    HAL_ADC_Stop(&hadc1);

    /*
     * STM32F103 ADC 12-bit:
     *
     * 0 ... 4095
     */
    if (*adc_value > 4095U)
    {
        return false;
    }

    return true;
}


/* =========================
 * Init sensor
 * ========================= */
void WaterSensor_Init(void)
{
    sensor_initialized = false;
    sensor_valid = false;

    current_adc_value = 0U;
    current_percent = -1.0f;

    /*
     * Calibration ADC STM32F1
     */
    if (HAL_ADCEx_Calibration_Start(&hadc1) != HAL_OK)
    {
        return;
    }

    sensor_initialized = true;
}


/* =========================
 * Read water level %
 * ========================= */
float WaterSensor_ReadPercent(void)
{
    if (!sensor_initialized)
    {
        sensor_valid = false;
        current_percent = -1.0f;

        return -1.0f;
    }

    /*
     * Kiểm tra calibration constants
     */
    if (ADC_VAL_FULL <= ADC_VAL_EMPTY)
    {
        sensor_valid = false;
        current_percent = -1.0f;

        return -1.0f;
    }

    uint32_t adc_sum = 0U;

    /*
     * =========================
     * Lấy 10 mẫu ADC
     * =========================
     */
    for (uint32_t i = 0U; i < ADC_SAMPLE_COUNT; i++)
    {
        uint32_t adc_sample = 0U;

        if (!WaterSensor_ReadAdcSample(&adc_sample))
        {
            sensor_valid = false;
            current_percent = -1.0f;

            return -1.0f;
        }

        adc_sum += adc_sample;
    }

    /*
     * =========================
     * Tính ADC trung bình
     * =========================
     *
     * Ví dụ:
     *
     * 1000
     * 1010
     * 995
     * ...
     *
     * sum / 10
     */
    current_adc_value =
        (adc_sum + (ADC_SAMPLE_COUNT / 2U))
        / ADC_SAMPLE_COUNT;

    /*
     * =========================
     * Kiểm tra sensor
     * =========================
     */
    if ((current_adc_value <= ADC_ERR_LOWER) ||
        (current_adc_value >= ADC_ERR_UPPER))
    {
        sensor_valid = false;
        current_percent = -1.0f;

        return -1.0f;
    }

    /*
     * =========================
     * ADC -> %
     * =========================
     *
     * ADC_EMPTY = 500  -> 0%
     * ADC_FULL  = 3200 -> 100%
     *
     * percent =
     *
     * ADC - EMPTY
     * -----------
     * FULL - EMPTY
     *
     * x 100
     */
    current_percent =
        ((float)(
            (int32_t)current_adc_value -
            (int32_t)ADC_VAL_EMPTY
        ) * 100.0f)
        /
        (float)(
            ADC_VAL_FULL -
            ADC_VAL_EMPTY
        );

    /*
     * Nếu thấp hơn EMPTY -> 0%
     * Nếu cao hơn FULL  -> 100%
     */
    current_percent = WaterSensor_Clamp(
        current_percent,
        0.0f,
        100.0f
    );

    sensor_valid = true;

    return current_percent;
}


/* =========================
 * Check sensor state
 * ========================= */
bool WaterSensor_IsValid(void)
{
    return sensor_valid;
}
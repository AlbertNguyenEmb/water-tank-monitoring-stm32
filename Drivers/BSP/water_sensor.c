#include "water_sensor.h"
#include "main.h"

#include <stdint.h>

extern ADC_HandleTypeDef hadc1;
#define ADC_VAL_EMPTY          500U
#define ADC_VAL_FULL           3200U
#define ADC_ERR_LOWER          50U
#define ADC_ERR_UPPER          4050U

#define ADC_TIMEOUT_MS         10U

static bool sensor_initialized = false;
static bool sensor_valid = false;

static uint32_t current_adc_value = 0U;
static float current_percent = -1.0f;

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

void WaterSensor_Init(void)
{
    sensor_initialized = false;
    sensor_valid = false;

    current_adc_value = 0U;
    current_percent = -1.0f;

    if (HAL_ADCEx_Calibration_Start(&hadc1) != HAL_OK)
    {
        return;
    }

    sensor_initialized = true;
}

float WaterSensor_ReadPercent(void)
{
    if (!sensor_initialized)
    {
        sensor_valid = false;
        current_percent = -1.0f;
        return -1.0f;
    }

    if (HAL_ADC_Start(&hadc1) != HAL_OK)
    {
        sensor_valid = false;
        current_percent = -1.0f;
        return -1.0f;
    }

    if (HAL_ADC_PollForConversion(
            &hadc1,
            ADC_TIMEOUT_MS
        ) != HAL_OK)
    {
        HAL_ADC_Stop(&hadc1);

        sensor_valid = false;
        current_percent = -1.0f;
        return -1.0f;
    }

    current_adc_value = HAL_ADC_GetValue(&hadc1);

    HAL_ADC_Stop(&hadc1);

    if (current_adc_value > 4095U)
    {
        sensor_valid = false;
        current_percent = -1.0f;
        return -1.0f;
    }

    if ((current_adc_value <= ADC_ERR_LOWER) ||
        (current_adc_value >= ADC_ERR_UPPER))
    {
        sensor_valid = false;
        current_percent = -1.0f;
        return -1.0f;
    }
    if (ADC_VAL_FULL <= ADC_VAL_EMPTY)
    {
        sensor_valid = false;
        current_percent = -1.0f;
        return -1.0f;
    }
    current_percent =
        ((float)(
            (int32_t)current_adc_value -
            (int32_t)ADC_VAL_EMPTY
        ) * 100.0f) /
        (float)(ADC_VAL_FULL - ADC_VAL_EMPTY);

    current_percent = WaterSensor_Clamp(
        current_percent,
        0.0f,
        100.0f
    );

    sensor_valid = true;

    return current_percent;
}

bool WaterSensor_IsValid(void)
{
    return sensor_valid;
}
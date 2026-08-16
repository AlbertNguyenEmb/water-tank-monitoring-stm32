#include "filter.h"

static bool Filter_IsValidDistance(float distance_cm)
{
    return ((distance_cm >= FILTER_MIN_DISTANCE_CM) &&
            (distance_cm <= FILTER_MAX_DISTANCE_CM));
}

static bool Filter_IsValidTankHeight(float tank_height_cm)
{
    return (tank_height_cm >= FILTER_MIN_TANK_HEIGHT_CM);
}

static float Filter_ClampPercent(float percent)
{
    if (percent < 0.0f)
    {
        return 0.0f;
    }

    if (percent > 100.0f)
    {
        return 100.0f;
    }

    return percent;
}

void Filter_Init(DistanceFilter_t *filter)
{
    if (filter == 0)
    {
        return;
    }

    filter->tank_height_cm = FILTER_DEFAULT_TANK_HEIGHT_CM;
    Filter_Reset(filter);
}

void Filter_Reset(DistanceFilter_t *filter)
{
    if (filter == 0)
    {
        return;
    }

    for (uint8_t i = 0U; i < FILTER_WINDOW_SIZE; i++)
    {
        filter->samples[i] = 0.0f;
    }

    if (!Filter_IsValidTankHeight(filter->tank_height_cm))
    {
        filter->tank_height_cm = FILTER_DEFAULT_TANK_HEIGHT_CM;
    }

    filter->sum = 0.0f;
    filter->filtered_distance_cm = 0.0f;
    filter->water_level_percent = 0.0f;
    filter->pending_outlier_cm = 0.0f;
    filter->index = 0U;
    filter->count = 0U;
    filter->outlier_count = 0U;
}

bool Filter_SetTankHeightCm(DistanceFilter_t *filter, float tank_height_cm)
{
    if ((filter == 0) || !Filter_IsValidTankHeight(tank_height_cm))
    {
        return false;
    }

    filter->tank_height_cm = tank_height_cm;
    filter->water_level_percent = Filter_DistanceToWaterLevelPercent(
        filter->filtered_distance_cm,
        filter->tank_height_cm
    );

    return true;
}

bool Filter_Update(
    DistanceFilter_t *filter,
    float distance_cm,
    float *filtered_distance_cm
)
{
    if (filter == 0)
    {
        return false;
    }

    if (!Filter_IsValidDistance(distance_cm))
    {
        if (filtered_distance_cm != 0)
        {
            *filtered_distance_cm = filter->filtered_distance_cm;
        }

        return false;
    }

    if (filter->count == 0U)
    {
        filter->filtered_distance_cm = distance_cm;
        filter->count = 1U;
    }
    else
    {
        filter->filtered_distance_cm =
            (FILTER_EMA_ALPHA * distance_cm) +
            ((1.0f - FILTER_EMA_ALPHA) * filter->filtered_distance_cm);

        if (filter->count < FILTER_WINDOW_SIZE)
        {
            filter->count++;
        }
    }

    filter->pending_outlier_cm = 0.0f;
    filter->outlier_count = 0U;

    filter->water_level_percent = Filter_DistanceToWaterLevelPercent(
        filter->filtered_distance_cm,
        filter->tank_height_cm
    );

    if (filtered_distance_cm != 0)
    {
        *filtered_distance_cm = filter->filtered_distance_cm;
    }

    return true;
}

bool Filter_UpdateWaterLevel(
    DistanceFilter_t *filter,
    float distance_cm,
    float *filtered_distance_cm,
    float *water_level_percent
)
{
    bool valid = Filter_Update(
        filter,
        distance_cm,
        filtered_distance_cm
    );

    if ((filter != 0) && (water_level_percent != 0))
    {
        *water_level_percent = filter->water_level_percent;
    }

    return valid;
}

float Filter_DistanceToWaterLevelPercent(
    float filtered_distance_cm,
    float tank_height_cm
)
{
    float percent;
    float usable_height_cm;

    if (!Filter_IsValidTankHeight(tank_height_cm))
    {
        return 0.0f;
    }

    if (filtered_distance_cm <= FILTER_FULL_DISTANCE_CM)
    {
        return 100.0f;
    }

    if (filtered_distance_cm >= tank_height_cm)
    {
        return 0.0f;
    }

    usable_height_cm = tank_height_cm - FILTER_FULL_DISTANCE_CM;
    if (usable_height_cm <= 0.0f)
    {
        return 0.0f;
    }

    percent =
        ((tank_height_cm - filtered_distance_cm) * 100.0f) /
        usable_height_cm;

    return Filter_ClampPercent(percent);
}

float Filter_GetDistanceCm(const DistanceFilter_t *filter)
{
    if (filter == 0)
    {
        return 0.0f;
    }

    return filter->filtered_distance_cm;
}

float Filter_GetWaterLevelPercent(const DistanceFilter_t *filter)
{
    if (filter == 0)
    {
        return 0.0f;
    }

    return filter->water_level_percent;
}

float Filter_GetTankHeightCm(const DistanceFilter_t *filter)
{
    if (filter == 0)
    {
        return 0.0f;
    }

    return filter->tank_height_cm;
}

bool Filter_IsReady(const DistanceFilter_t *filter)
{
    if (filter == 0)
    {
        return false;
    }

    return (filter->count >= FILTER_WINDOW_SIZE);
}

uint8_t Filter_GetSampleCount(const DistanceFilter_t *filter)
{
    if (filter == 0)
    {
        return 0U;
    }

    return filter->count;
}

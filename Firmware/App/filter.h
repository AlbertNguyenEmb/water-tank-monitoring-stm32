#ifndef FILTER_H
#define FILTER_H

#include <stdbool.h>
#include <stdint.h>

#define FILTER_WINDOW_SIZE        5U
#define FILTER_MIN_DISTANCE_CM    2.0f
#define FILTER_MAX_DISTANCE_CM    400.0f
#define FILTER_DEFAULT_TANK_HEIGHT_CM    100.0f
#define FILTER_MIN_TANK_HEIGHT_CM        1.0f

typedef struct
{
    float samples[FILTER_WINDOW_SIZE];
    float sum;
    float filtered_distance_cm;
    float water_level_percent;
    float tank_height_cm;
    uint8_t index;
    uint8_t count;
} DistanceFilter_t;

void Filter_Init(DistanceFilter_t *filter);
void Filter_Reset(DistanceFilter_t *filter);
bool Filter_SetTankHeightCm(DistanceFilter_t *filter, float tank_height_cm);

bool Filter_Update(
    DistanceFilter_t *filter,
    float distance_cm,
    float *filtered_distance_cm
);

bool Filter_UpdateWaterLevel(
    DistanceFilter_t *filter,
    float distance_cm,
    float *filtered_distance_cm,
    float *water_level_percent
);

float Filter_DistanceToWaterLevelPercent(
    float filtered_distance_cm,
    float tank_height_cm
);

float Filter_GetDistanceCm(const DistanceFilter_t *filter);
float Filter_GetWaterLevelPercent(const DistanceFilter_t *filter);
float Filter_GetTankHeightCm(const DistanceFilter_t *filter);
bool Filter_IsReady(const DistanceFilter_t *filter);
uint8_t Filter_GetSampleCount(const DistanceFilter_t *filter);

#endif /* FILTER_H */

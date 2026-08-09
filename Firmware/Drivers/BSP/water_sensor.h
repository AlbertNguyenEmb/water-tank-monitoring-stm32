#ifndef WATERSENSOR_H
#define WATERSENSOR_H

#include <stdbool.h>
#include <stdint.h>

void WaterSensor_Init(void);
float WaterSensor_ReadPercent(void);
bool WaterSensor_IsValid(void);
uint32_t WaterSensor_GetRawADC(void);

#endif /* WATERSENSOR_H */
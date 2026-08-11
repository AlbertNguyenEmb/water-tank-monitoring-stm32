#ifndef WATERSENSOR_H
#define WATERSENSOR_H

#include <stdbool.h>

void WaterSensor_Init(void);
float WaterSensor_ReadPercent(void);
bool WaterSensor_IsValid(void);

#endif /* WATERSENSOR_H */
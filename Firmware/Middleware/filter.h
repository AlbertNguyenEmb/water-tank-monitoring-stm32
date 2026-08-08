#ifndef FILTER_H
#define FILTER_H

#include <stdint.h>

/* So mau dung de tinh trung binh dong (moving average).
   Tang len de muot hon (nhung tre hon), giam de nhanh hon (nhung on nhieu hon). */
#define FILTER_WINDOW_SIZE   8U

/**
 * @brief  Khoi tao module Filter: reset bo dem trung binh dong.
 *         Goi 1 lan trong App_Init() truoc khi dung Filter_Update().
 */
void Filter_Init(void);

/**
 * @brief  Nhan 1 mau % muc nuoc (da duoc WaterSensor_ReadPercent() quy doi
 *         san tu ADC), loc nhieu bang trung binh dong, tra ve % da lam muot.
 * @param  input: % muc nuoc tho, lay tu WaterSensor_ReadPercent()
 * @return % muc nuoc da loc, gioi han trong [0.0f, 100.0f]
 */
float Filter_Update(float input);

#endif /* FILTER_H */
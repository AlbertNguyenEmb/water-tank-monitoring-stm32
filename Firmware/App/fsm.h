#ifndef FSM_H
#define FSM_H

#include <stdint.h>

/* Nguong hysteresis - chinh lai cho khop voi be nuoc thuc te.
 * Luu y: TH_NGUONG_TRAN phai LON HON TH_NGUONG_DAY (day la binh thuong,
 * tran la su co - xem giai thich trong fsm.c). */
#define TH_NGUONG_CAN     20.0f
#define TH_NGUONG_DAY     90.0f
#define TH_NGUONG_TRAN     98.0f

typedef enum
{
    FSM_STATE_INIT = 0,
    FSM_STATE_FILLING,
    FSM_STATE_FULL,
    FSM_STATE_OVERFLOW,
    FSM_STATE_ERROR
} FSM_State_t;

/**
 * @brief  Dua may trang thai ve INIT. Goi 1 lan trong App_Init().
 */
void FSM_Init(void);

/**
 * @brief  Chay 1 buoc cua may trang thai: tu doc WaterSensor_IsValid() +
 *         WaterSensor_ReadPercent(), dua qua Filter_Update() de lam muot,
 *         roi quyet dinh chuyen trang thai. Goi moi chu ky (trong App_Run()).
 *         KHONG nhan tham so - tu lay du lieu ben trong.
 */
void FSM_Run(void);

/**
 * @brief  Lay trang thai hien tai de Control/Logger su dung.
 */
FSM_State_t FSM_GetState(void);

#endif /* FSM_H */
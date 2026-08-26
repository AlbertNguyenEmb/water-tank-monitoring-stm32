#include "control.h"
#include "fsm.h"
#include "water_sensor.h"
#include "ln298n.h"
#include "buzzer.h"
#include "oled.h"

void Control_Init(void)
{
    L298N_Off();
    Buzzer_Off();
    OLED_ShowWaterState(-1.0f, "Khoi dong...");
}

void Control_Update(void)
{
    /* FSM_Run() da chay va cap nhat trang thai truoc khi ham nay duoc goi
     * (thu tu goi dung trong App_Run(): FSM_Run() -> Control_Update()).
     * O day CHI doc trang thai va xuat lenh, KHONG tu doc/loc cam bien lai. */
    FSM_State_t state = FSM_GetState();

    /* Doc % rieng de hien thi OLED (WaterSensor_ReadPercent() la ham doc
     * nhanh, khong ton chi phi loc lai; muc dich chi de hien thi, khong
     * anh huong den logic FSM). */
    float percent = WaterSensor_ReadPercent();

    switch (state)
    {
    case FSM_STATE_INIT:
        L298N_Off();
        Buzzer_Off();
        OLED_ShowWaterState(percent, "Khoi dong...");
        break;

    case FSM_STATE_FILLING:
        L298N_On();
        Buzzer_Off();
        OLED_ShowWaterState(percent, "Dang bom nuoc");
        break;

    case FSM_STATE_FULL:
        L298N_Off();
        Buzzer_Off();
        OLED_ShowWaterState(percent, "Be day - OK");
        break;

    case FSM_STATE_OVERFLOW:
        L298N_Off();      /* cuong buc tat bom, bat ke trang thai truoc do */
        Buzzer_On();
        OLED_ShowWaterState(percent, "CANH BAO TRAN!");
        break;

    case FSM_STATE_ERROR:
        L298N_Off();      /* an toan: khong doan mo bom khi khong biet muc nuoc */
        Buzzer_On();
        OLED_ShowWaterState(percent, "LOI CAM BIEN!");
        break;

    default:
        break;
    }
}

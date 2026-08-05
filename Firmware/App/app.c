#include "app.h"

/* BSP (Drivers/BSP) */
#include "relay.h"
#include "water_sensor.h"
#include "oled.h"
/* buzzer.h khong co ham Init rieng theo bang API - bo qua o day */

/* Middleware */
#include "filter.h"
#include "logger.h"

/* App (logic) */
#include "fsm.h"
#include "control.h"

void App_Init(void)
{
    /* 1) Khoi tao driver phan cung truoc */
    Relay_Init();
    WaterSensor_Init();
    OLED_Init();

    /* 2) Khoi tao Middleware */
    Filter_Init();
    Logger_Init();

    /* 3) Khoi tao logic (FSM truoc, Control sau vi Control can doc FSM) */
    FSM_Init();
    Control_Init();
}

void App_Run(void)
{
    /* Thu tu BAT BUOC: FSM chay truoc de co trang thai moi nhat,
     * roi Control moi doc trang thai do de dieu khien thiet bi,
     * cuoi cung Logger ghi lai ket qua cua chu ky nay. */
    FSM_Run();
    Control_Update();
    Logger_Run();
}
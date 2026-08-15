#ifndef APP_H
#define APP_H

/**
 * @brief  Khoi tao toan bo he thong theo dung thu tu:
 *         cac driver BSP truoc (Relay/WaterSensor/OLED), roi den
 *         Middleware (Filter/Logger), cuoi cung la logic (FSM/Control).
 *         Goi 1 lan trong main(), SAU KHI HAL_Init() va cac ngoai vi
 *         (ADC/UART/I2C/GPIO/Timer) da duoc CubeMX khoi tao xong.
 */
void App_Init(void);

/**
 * @brief  Chay 1 chu ky xu ly day du: FSM_Run() -> Control_Update()
 *         -> Logger_Run(). Goi moi khi ngat Timer (chu ky lay mau,
 *         vd 1-2 giay/lan) xay ra - KHONG goi lien tuc trong while(1)
 *         khong co dieu kien, se lam sai chu ky lay mau da thiet ke.
 */
void App_Run(void);

#endif /* APP_H */
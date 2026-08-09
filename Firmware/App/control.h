#ifndef CONTROL_H
#define CONTROL_H

/**
 * @brief  Khoi tao Control: dua Relay/Buzzer/OLED ve trang thai an toan
 *         ban dau (tat het). Goi 1 lan trong App_Init(), SAU KHI
 *         Relay_Init()/OLED_Init() da chay.
 */
void Control_Init(void);

/**
 * @brief  Doc FSM_GetState() (FSM_Run() da chay truoc do trong App_Run())
 *         va xuat lenh dieu khien tuong ung cho Relay/Buzzer/OLED.
 *         Goi moi chu ky, NGAY SAU FSM_Run().
 */
void Control_Update(void);

#endif /* CONTROL_H */
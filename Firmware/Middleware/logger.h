#ifndef LOGGER_H
#define LOGGER_H

/**
 * @brief  Khoi tao Logger (thuong khong can lam gi them vi UART da duoc
 *         CubeMX/HAL_UART_Init() khoi tao trong main.c). Goi 1 lan trong main().
 */
void Logger_Init(void);

/**
 * @brief  Gui 1 dong log qua UART dang CSV: tick,percent,state
 *         Goi sau Control_Update() moi chu ky, hoac cach vai chu ky
 *         (vd moi 1s) neu muon giam tai UART.
 */
void Logger_Run(void);

#endif /* LOGGER_H */
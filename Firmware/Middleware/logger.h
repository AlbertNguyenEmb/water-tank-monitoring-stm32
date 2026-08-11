#ifndef LOGGER_H
#define LOGGER_H

/**
 * @brief Khoi tao Logger va gui header CSV qua UART.
 */
void Logger_Init(void);

/**
 * @brief Ghi mot dong log cua chu ky hien tai.
 *
 * Du lieu duoc lay tu FSM:
 * - Timestamp: HAL_GetTick()
 * - Muc nuoc: gia tri da loc ma FSM su dung
 * - Trang thai: state hien tai cua FSM
 *
 * Format:
 * time_ms,level_percent,state
 */
void Logger_Run(void);

#endif /* LOGGER_H */
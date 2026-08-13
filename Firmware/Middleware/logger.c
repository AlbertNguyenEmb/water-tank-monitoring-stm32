#include "logger.h"
#include "main.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

extern UART_HandleTypeDef huart1;

void Logger_Init(void)
{
    // UART1 đã được khởi tạo bởi CubeMX.
}

void Logger_Print(const char *message)
{
    HAL_UART_Transmit(
        &huart1,
        (uint8_t *)message,
        strlen(message),
        HAL_MAX_DELAY
    );
}

void Logger_Printf(const char *format, ...)
{
    char buffer[128];

    va_list args;
    va_start(args, format);

    vsnprintf(buffer, sizeof(buffer), format, args);

    va_end(args);

    HAL_UART_Transmit(
        &huart1,
        (uint8_t *)buffer,
        strlen(buffer),
        HAL_MAX_DELAY
    );
}
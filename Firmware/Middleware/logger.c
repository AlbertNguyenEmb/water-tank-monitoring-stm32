#include "logger.h"
#include "main.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

extern UART_HandleTypeDef huart1;

void Logger_Init(void)
{
    /* UART1 is initialized by CubeMX before the logger is used. */
}

void Logger_Print(const char *message)
{
    if (message == 0)
    {
        return;
    }

    (void)HAL_UART_Transmit(
        &huart1,
        (uint8_t *)message,
        (uint16_t)strlen(message),
        HAL_MAX_DELAY
    );
}

void Logger_Printf(const char *format, ...)
{
    char buffer[160];
    va_list args;
    int length;

    if (format == 0)
    {
        return;
    }

    va_start(args, format);
    length = vsnprintf(
        buffer,
        sizeof(buffer),
        format,
        args
    );
    va_end(args);

    if (length <= 0)
    {
        return;
    }

    if ((uint32_t)length >= sizeof(buffer))
    {
        length = (int)sizeof(buffer) - 1;
    }

    (void)HAL_UART_Transmit(
        &huart1,
        (uint8_t *)buffer,
        (uint16_t)length,
        HAL_MAX_DELAY
    );
}

void Logger_Run(void)
{
    /* Kept as a no-op for legacy modules that may still call Logger_Run(). */
}

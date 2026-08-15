#ifndef LOGGER_H
#define LOGGER_H

void Logger_Init(void);
void Logger_Print(const char *message);
void Logger_Printf(const char *format, ...);
void Logger_Run(void);

#endif /* LOGGER_H */

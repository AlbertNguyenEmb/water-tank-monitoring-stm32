#ifndef OLED_H
#define OLED_H

#include <stdbool.h>
#include <stdint.h>

void OLED_SetI2CAddress(uint8_t address_7bit);
bool OLED_Init(void);
void OLED_ShowLevel(float level);
void OLED_ShowStatus(const char *st);
void OLED_ShowUltrasonicTest(
    uint32_t sample,
    float distance_cm,
    const char *status,
    bool distance_valid
);

#endif /* OLED_H */

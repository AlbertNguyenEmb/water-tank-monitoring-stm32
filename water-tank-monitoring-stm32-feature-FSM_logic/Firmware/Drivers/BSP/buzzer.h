#ifndef BUZZER_H
#define BUZZER_H

#include "main.h"
#include <stdbool.h>

void Buzzer_Init(void);

void Buzzer_On(void);
void Buzzer_Off(void);
void Buzzer_Toggle(void);
void Buzzer_Set(bool on);

bool Buzzer_IsOn(void);

#endif /* BUZZER_H */

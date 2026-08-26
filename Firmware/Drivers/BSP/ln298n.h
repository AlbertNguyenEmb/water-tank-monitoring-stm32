#ifndef LN298N_H
#define LN298N_H

#include "main.h"
#include <stdbool.h>

void L298N_Init(void);

void L298N_On(void);
void L298N_Off(void);
void L298N_Toggle(void);

bool L298N_IsOn(void);

#endif /* LN298N_H */

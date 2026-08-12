#ifndef RELAY_H
#define RELAY_H

#include "main.h"
#include <stdbool.h>

void Relay_Init(void);

void Relay_On(void);
void Relay_Off(void);
void Relay_Toggle(void);

bool Relay_IsOn(void);

#endif
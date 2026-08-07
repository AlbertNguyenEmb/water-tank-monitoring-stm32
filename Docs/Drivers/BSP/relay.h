#ifndef RELAY_H
#define RELAY_H

#include <stdbool.h>

void Relay_Init(void);
void Relay_On(void);
void Relay_Off(void);
bool Relay_GetState(void);

#endif /* RELAY_H */
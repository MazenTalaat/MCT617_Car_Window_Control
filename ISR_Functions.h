#ifndef ISR_FUNCTIONS_H
#define ISR_FUNCTIONS_H

#include "common.h"

void vISRFunctionPassengerUp(void);
void vISRFunctionPassengerDown(void);
void vISRFunctionDriverUp(void);
void vISRFunctionDriverDown(void);
void vISRFunctionLowerLimit(void);
void vISRFunctionUpperLimit(void);
void vISRFunctionJam(void);

#endif
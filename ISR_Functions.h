#ifndef ISR_FUNCTIONS_H
#define ISR_FUNCTIONS_H

#define PART_TM4C123GH6PM
#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_types.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"

#include "queue.h"
#include "timers.h"

void vISRFunctionPassengerUp(void);
void vISRFunctionPassengerDown(void);
void vISRFunctionDriverUp(void);
void vISRFunctionDriverDown(void);
void vISRFunctionLowerLimit(void);
void vISRFunctionUpperLimit(void);
void vISRFunctionJam(void);

#endif
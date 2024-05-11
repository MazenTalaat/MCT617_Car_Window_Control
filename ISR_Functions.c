#include "ISR_Functions.h"

extern SemaphoreHandle_t xSemaphoreDriverUp,
    xSemaphoreDriverDown,
    xSemaphorePassengerUp,
    xSemaphorePassengerDown,
    xSemaphoreLockWindow,
    xSemaphoreUpperLimit,
    xSemaphoreLowerLimit,
    xSemaphoreJamDetected;

/*********************************************************************************************************
** Function name:       vISRFunctionDriverUp
** Descriptions:        Give semaphore to xSemaphoreDriverUp then force context switching
** input parameters:    none
** output parameters:   none
** Returned value:      none
*********************************************************************************************************/
void vISRFunctionDriverUp(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(xSemaphoreDriverUp, &xHigherPriorityTaskWoken);
    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}

/*********************************************************************************************************
** Function name:       vISRFunctionDriverDown
** Descriptions:        Give semaphore to xSemaphoreDriverDown then force context switching
** input parameters:    none
** output parameters:   none
** Returned value:      none
*********************************************************************************************************/
void vISRFunctionDriverDown(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(xSemaphoreDriverDown, &xHigherPriorityTaskWoken);
    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}

/*********************************************************************************************************
** Function name:       vISRFunctionPassengerUp
** Descriptions:        Give semaphore to xSemaphorePassengerUp then force context switching
** input parameters:    none
** output parameters:   none
** Returned value:      none
*********************************************************************************************************/
void vISRFunctionPassengerUp(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(xSemaphorePassengerUp, &xHigherPriorityTaskWoken);
    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}

/*********************************************************************************************************
** Function name:       vISRFunctionPassengerDown
** Descriptions:        Give semaphore to xSemaphorePassengerDown then force context switching
** input parameters:    none
** output parameters:   none
** Returned value:      none
*********************************************************************************************************/
void vISRFunctionPassengerDown(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(xSemaphorePassengerDown, &xHigherPriorityTaskWoken);
    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}

/*********************************************************************************************************
** Function name:       vISRFunctionLowerLimit
** Descriptions:        Give semaphore to xSemaphoreLowerLimit then force context switching
** input parameters:    none
** output parameters:   none
** Returned value:      none
*********************************************************************************************************/
void vISRFunctionLowerLimit(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(xSemaphoreLowerLimit, &xHigherPriorityTaskWoken);
    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}

/*********************************************************************************************************
** Function name:       vISRFunctionUpperLimit
** Descriptions:        Give semaphore to xSemaphoreUpperLimit then force context switching
** input parameters:    none
** output parameters:   none
** Returned value:      none
*********************************************************************************************************/
void vISRFunctionUpperLimit(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(xSemaphoreUpperLimit, &xHigherPriorityTaskWoken);
    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}

/*********************************************************************************************************
** Function name:       vISRFunctionJam
** Descriptions:        Give semaphore to xSemaphoreJamDetected then force context switching
** input parameters:    none
** output parameters:   none
** Returned value:      none
*********************************************************************************************************/
void vISRFunctionJam(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(xSemaphoreJamDetected, &xHigherPriorityTaskWoken);
    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}
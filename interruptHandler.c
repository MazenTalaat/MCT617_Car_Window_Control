#include "interruptHandler.h"

// Ticks to wait to debounce the button
volatile TickType_t minTicksReq = 500u;

/*********************************************************************************************************
** Function name:       GPIOA_Handler
** Descriptions:        Handle interrupts from GPIOA pins
** input parameters:    none
** output parameters:   none
** Returned value:      none
*********************************************************************************************************/
void GPIOA_Handler(void)
{
    static TickType_t lastTickVal = 0U;
    TickType_t currentTickVal = xTaskGetTickCountFromISR();
    uint32_t intStatus = GPIOIntStatus(GPIO_PORTA_BASE, false);
    GPIOIntClear(GPIO_PORTA_BASE, GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_7);
    // Get rid of bouncing
    if ((currentTickVal - lastTickVal) > minTicksReq)
    {
        if (intStatus & GPIO_PIN_2)
        {
            GPIOIntClear(GPIO_PORTA_BASE, GPIO_PIN_2);
            vISRFunctionPassengerUp();
        }
        if (intStatus & GPIO_PIN_3)
        {
            GPIOIntClear(GPIO_PORTA_BASE, GPIO_PIN_3);
            vISRFunctionPassengerDown();
        }
        if (intStatus & GPIO_PIN_4)
        {
            GPIOIntClear(GPIO_PORTA_BASE, GPIO_PIN_4);
            vISRFunctionDriverUp();
        }
        if (intStatus & GPIO_PIN_5)
        {
            GPIOIntClear(GPIO_PORTA_BASE, GPIO_PIN_5);
            vISRFunctionDriverDown();
        }
        if (intStatus & GPIO_PIN_7)
        {
            GPIOIntClear(GPIO_PORTA_BASE, GPIO_PIN_7);
            vISRFunctionJam();
        }
    }
    lastTickVal = currentTickVal;
}

/*********************************************************************************************************
** Function name:       GPIOF_Handler
** Descriptions:        Handle interrupts from GPIOF pins
** input parameters:    none
** output parameters:   none
** Returned value:      none
*********************************************************************************************************/
void GPIOF_Handler(void)
{
    static TickType_t lastTickValF = 0U;
    TickType_t currentTickVal = xTaskGetTickCountFromISR();
    uint32_t intStatus = GPIOIntStatus(GPIO_PORTF_BASE, true);
    GPIOIntClear(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    // Get rid of bouncing
    if ((currentTickVal - lastTickValF) > minTicksReq)
    {
        if (intStatus & GPIO_PIN_1)
        {
            GPIOIntClear(GPIO_PORTF_BASE, GPIO_PIN_1);
            vISRFunctionLowerLimit();
        }
        if (intStatus & GPIO_PIN_0)
        {
             GPIOIntClear(GPIO_PORTF_BASE, GPIO_PIN_0);
            vISRFunctionUpperLimit();
        }
    }
    lastTickValF = currentTickVal;
}
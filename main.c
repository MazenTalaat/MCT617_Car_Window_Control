/**
 ********************************************************************************
 * @file    main
 * @author  Mazens
 * @date
 * @brief    Here is where the main function live :)
 ********************************************************************************
 */
/********************************************************************************
 * INCLUDES
 ********************************************************************************/
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

/********************************************************************************
 * PRIVATE MACROS AND DEFINES
 ********************************************************************************/
/// \brief Priorities at which the tasks are created
#define TASK_01_PRIORITY (tskIDLE_PRIORITY + 2)

/********************************************************************************
 * PRIVATE TYPEDEFS
 ********************************************************************************/

/********************************************************************************
 * STATIC VARIABLES
 ********************************************************************************/
static uint8_t LowerlimitReached = false;
static uint8_t UpperlimitReached = false;
static uint8_t Jam = false;
static uint32_t portAReq = 0u;
static uint32_t portAReq2 = 0u;
volatile TickType_t minTicksReq = 500u;
/********************************************************************************
 * GLOBAL VARIABLES
 ********************************************************************************/

// Declare semaphore handles for each task
SemaphoreHandle_t xSemaphorePassengerUp,
    xSemaphorePassengerDown,
    xSemaphoreDriverUp,
    xSemaphoreDriverDown,
    xSemaphoreLock,
    xSemaphoreUpperLimit,
    xSemaphoreLowerLimit,
    xSemaphoreJam;

/********************************************************************************
 * STATIC FUNCTION PROTOTYPES
 ********************************************************************************/

void GPIOPortA_Handler(void);
void GPIOPortF_Handler(void);

void vTaskFunctionPassengerUp(void *pvParameters);
void vTaskFunctionPassengerDown(void *pvParameters);
void vTaskFunctionDriverUp(void *pvParameters);
void vTaskFunctionDriverDown(void *pvParameters);
void vTaskFunctionLowerLimit(void *pvParameters);
void vTaskFunctionUpperLimit(void *pvParameters);
void vTaskFunctionJam(void *pvParameters);

/********************************************************************************
 * FUNCTION NAME:       main
 * \param  [in]         void
 * \param  [out]        int
 *
 *
 ********************************************************************************/
int main(void)
{
    // Set the system clock to 40MHz
    SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN);

    // Enable the GPIO port that is used for the driver and passenger
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    // Configure the control switch pins as inputs
    GPIOPinTypeGPIOInput(GPIO_PORTA_BASE, GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7);
    GPIOPadConfigSet(GPIO_PORTA_BASE, GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

    // Enable the GPIO port that is used for the motor
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

    // Configure the motor pins as outputs
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_2 | GPIO_PIN_3);

    // Configure the limit switch pins as inputs
    GPIOUnlockPin(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4);
    GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4);
    GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4, GPIO_STRENGTH_12MA, GPIO_PIN_TYPE_STD_WPU);

    // Create the semaphores
    xSemaphorePassengerUp = xSemaphoreCreateBinary();
    xSemaphorePassengerDown = xSemaphoreCreateBinary();
    xSemaphoreDriverUp = xSemaphoreCreateBinary();
    xSemaphoreDriverDown = xSemaphoreCreateBinary();
    xSemaphoreLock = xSemaphoreCreateBinary();
    xSemaphoreLowerLimit = xSemaphoreCreateBinary();
    xSemaphoreUpperLimit = xSemaphoreCreateBinary();
    xSemaphoreJam = xSemaphoreCreateBinary();

    // Create the tasks
    xTaskCreate(vTaskFunctionPassengerUp, "Task Passenger Up", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1U, NULL);
    xTaskCreate(vTaskFunctionPassengerDown, "Task Passenger Down", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1U, NULL);
    xTaskCreate(vTaskFunctionDriverUp, "Task Driver Up", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1U, NULL);
    xTaskCreate(vTaskFunctionDriverDown, "Task Driver Down", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1U, NULL);
    xTaskCreate(vTaskFunctionLowerLimit, "Task Lower Limit", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1U, NULL);
    xTaskCreate(vTaskFunctionUpperLimit, "Task Upper Limit", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1U, NULL);
    xTaskCreate(vTaskFunctionJam, "Task Jam", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1U, NULL);

    // Configure the interrupts for the switch pins of Port A
    IntEnable(INT_GPIOA);
    GPIOIntTypeSet(GPIO_PORTA_BASE, GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_7, GPIO_FALLING_EDGE);
    GPIOIntRegister(GPIO_PORTA_BASE, GPIOPortA_Handler);
    GPIOIntClear(GPIO_PORTA_BASE, GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_7);
    GPIOIntEnable(GPIO_PORTA_BASE, GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_7);
    IntPrioritySet(INT_GPIOA, configMAX_SYSCALL_INTERRUPT_PRIORITY + 1U);
    // Configure the interrupts for the switch pins of Port F
    IntEnable(INT_GPIOF);
    GPIOIntTypeSet(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4, GPIO_BOTH_EDGES);
    GPIOIntRegister(GPIO_PORTF_BASE, GPIOPortF_Handler);
    GPIOIntClear(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4);
    GPIOIntEnable(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4);
    IntPrioritySet(INT_GPIOF, configMAX_SYSCALL_INTERRUPT_PRIORITY + 1U);

    // Start the scheduler
    vTaskStartScheduler();

    // Should never reach here
    return 0;
}
/********************************************************************************
 * FUNCTION NAME:       vISRFunctionPassengerUp
 * \param  [in]         void
 * \param  [out]        void
 *
 *
 ********************************************************************************/
void vISRFunctionPassengerUp(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(xSemaphorePassengerUp, &xHigherPriorityTaskWoken);
    /* Giving the semaphore may have unblocked a task - if it did and the
    unblocked task has a priority equal to or above the currently executing
    task then xHigherPriorityTaskWoken will have been set to pdTRUE and
    portEND_SWITCHING_ISR() will force a context switch to the newly unblocked
    higher priority task.

    NOTE: The syntax for forcing a context switch within an ISR varies between
    FreeRTOS ports.  The portEND_SWITCHING_ISR() macro is provided as part of
    the Cortex M3 port layer for this purpose.  taskYIELD() must never be called
    from an ISR! */
    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
    // portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
/********************************************************************************
 * FUNCTION NAME:       vISRFunctionPassengerDown
 * \param  [in]         void
 * \param  [out]        void
 *
 *
 ********************************************************************************/
void vISRFunctionPassengerDown(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(xSemaphorePassengerDown, &xHigherPriorityTaskWoken);
    /* Giving the semaphore may have unblocked a task - if it did and the
    unblocked task has a priority equal to or above the currently executing
    task then xHigherPriorityTaskWoken will have been set to pdTRUE and
    portEND_SWITCHING_ISR() will force a context switch to the newly unblocked
    higher priority task.

    NOTE: The syntax for forcing a context switch within an ISR varies between
    FreeRTOS ports.  The portEND_SWITCHING_ISR() macro is provided as part of
    the Cortex M3 port layer for this purpose.  taskYIELD() must never be called
    from an ISR! */
    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
    // portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
/********************************************************************************
 * FUNCTION NAME:       vISRFunctionDriverUp
 * \param  [in]         void
 * \param  [out]        void
 *
 *
 ********************************************************************************/
void vISRFunctionDriverUp(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(xSemaphoreDriverUp, &xHigherPriorityTaskWoken);
    /* Giving the semaphore may have unblocked a task - if it did and the
    unblocked task has a priority equal to or above the currently executing
    task then xHigherPriorityTaskWoken will have been set to pdTRUE and
    portEND_SWITCHING_ISR() will force a context switch to the newly unblocked
    higher priority task.

    NOTE: The syntax for forcing a context switch within an ISR varies between
    FreeRTOS ports.  The portEND_SWITCHING_ISR() macro is provided as part of
    the Cortex M3 port layer for this purpose.  taskYIELD() must never be called
    from an ISR! */
    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
    // portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
/********************************************************************************
 * FUNCTION NAME:       vISRFunctionDriverDown
 * \param  [in]         void
 * \param  [out]        void
 *
 *
 ********************************************************************************/
void vISRFunctionDriverDown(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(xSemaphoreDriverDown, &xHigherPriorityTaskWoken);
    /* Giving the semaphore may have unblocked a task - if it did and the
    unblocked task has a priority equal to or above the currently executing
    task then xHigherPriorityTaskWoken will have been set to pdTRUE and
    portEND_SWITCHING_ISR() will force a context switch to the newly unblocked
    higher priority task.

    NOTE: The syntax for forcing a context switch within an ISR varies between
    FreeRTOS ports.  The portEND_SWITCHING_ISR() macro is provided as part of
    the Cortex M3 port layer for this purpose.  taskYIELD() must never be called
    from an ISR! */
    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
    // portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
/********************************************************************************
 * FUNCTION NAME:       vISRFunctionLowerLimit
 * \param  [in]         void
 * \param  [out]        void
 *
 *
 ********************************************************************************/
void vISRFunctionLowerLimit(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(xSemaphoreLowerLimit, &xHigherPriorityTaskWoken);
    /* Giving the semaphore may have unblocked a task - if it did and the
    unblocked task has a priority equal to or above the currently executing
    task then xHigherPriorityTaskWoken will have been set to pdTRUE and
    portEND_SWITCHING_ISR() will force a context switch to the newly unblocked
    higher priority task.

    NOTE: The syntax for forcing a context switch within an ISR varies between
    FreeRTOS ports.  The portEND_SWITCHING_ISR() macro is provided as part of
    the Cortex M3 port layer for this purpose.  taskYIELD() must never be called
    from an ISR! */
    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
    // portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
/********************************************************************************
 * FUNCTION NAME:       vISRFunctionUpperLimit
 * \param  [in]         void
 * \param  [out]        void
 *
 *
 ********************************************************************************/
void vISRFunctionUpperLimit(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(xSemaphoreUpperLimit, &xHigherPriorityTaskWoken);
    /* Giving the semaphore may have unblocked a task - if it did and the
    unblocked task has a priority equal to or above the currently executing
    task then xHigherPriorityTaskWoken will have been set to pdTRUE and
    portEND_SWITCHING_ISR() will force a context switch to the newly unblocked
    higher priority task.

    NOTE: The syntax for forcing a context switch within an ISR varies between
    FreeRTOS ports.  The portEND_SWITCHING_ISR() macro is provided as part of
    the Cortex M3 port layer for this purpose.  taskYIELD() must never be called
    from an ISR! */
    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
    // portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

// TODO added function
void vISRFunctionJam(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(xSemaphoreJam, &xHigherPriorityTaskWoken);
    /* Giving the semaphore may have unblocked a task - if it did and the
    unblocked task has a priority equal to or above the currently executing
    task then xHigherPriorityTaskWoken will have been set to pdTRUE and
    portEND_SWITCHING_ISR() will force a context switch to the newly unblocked
    higher priority task.

    NOTE: The syntax for forcing a context switch within an ISR varies between
    FreeRTOS ports.  The portEND_SWITCHING_ISR() macro is provided as part of
    the Cortex M3 port layer for this purpose.  taskYIELD() must never be called
    from an ISR! */
    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
    // portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
/********************************************************************************
 * FUNCTION NAME:       vTaskFunctionPassengerUp
 * \param  [in]         void*
 * \param  [out]        void
 *
 *
 ********************************************************************************/
void vTaskFunctionPassengerUp(void *pvParameters)
{
    while (1)
    {
        xSemaphoreTake(xSemaphorePassengerUp, portMAX_DELAY);
        // Process interrupt for pin 2 port A
        if ((GPIOPinRead(GPIO_PORTA_BASE, GPIO_PIN_6) & GPIO_PIN_6) == GPIO_PIN_6)
        {
            LowerlimitReached = false;
            UpperlimitReached = false;

            vTaskDelay(250 / portTICK_RATE_MS);
            if ((GPIOPinRead(GPIO_PORTA_BASE, GPIO_PIN_2) & GPIO_PIN_2) != GPIO_PIN_2)
            {
                vTaskDelay(250 / portTICK_RATE_MS);
                if ((GPIOPinRead(GPIO_PORTA_BASE, GPIO_PIN_2) & GPIO_PIN_2) != GPIO_PIN_2)
                {
                    while ((GPIOPinRead(GPIO_PORTA_BASE, GPIO_PIN_2) & GPIO_PIN_2) != GPIO_PIN_2)
                    {
                        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2);
                        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, false);
                        vTaskDelay(250 / portTICK_RATE_MS);
                    }
                    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, false);
                    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, false);
                }
                else
                {
                    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2);
                    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, false);
                }
            }
            else
            {
                // GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2);
                // GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, false);
                // vTaskDelay(5000 / portTICK_RATE_MS);
                if (false == UpperlimitReached)
                {
                    // GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, false);
                    // GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, GPIO_PIN_3);
                    // vTaskDelay( 2000 / portTICK_RATE_MS );
                    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, false);
                    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, false);
                    GPIOIntClear(GPIO_PORTA_BASE, GPIO_PIN_2);
                }
                // if (true == Jam)
                // {
                //     GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, false);
                //     GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, GPIO_PIN_3);
                //     vTaskDelay(1500 / portTICK_RATE_MS);
                //     GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, false);
                //     GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, false);
                //     Jam = false;
                // }
            }
        }
        taskYIELD();
    }
}
/********************************************************************************
 * FUNCTION NAME:       vTaskFunctionPassengerDown
 * \param  [in]         void*
 * \param  [out]        void
 *
 *
 ********************************************************************************/
void vTaskFunctionPassengerDown(void *pvParameters)
{
    while (1)
    {
        xSemaphoreTake(xSemaphorePassengerDown, portMAX_DELAY);
        // Process interrupt for pin 3 port A
        if ((GPIOPinRead(GPIO_PORTA_BASE, GPIO_PIN_6) & GPIO_PIN_6) == GPIO_PIN_6)
        {
            LowerlimitReached = false;
            UpperlimitReached = false;
            vTaskDelay(250 / portTICK_RATE_MS);
            if ((GPIOPinRead(GPIO_PORTA_BASE, GPIO_PIN_3) & GPIO_PIN_3) != GPIO_PIN_3)
            {
                vTaskDelay(250 / portTICK_RATE_MS);
                if ((GPIOPinRead(GPIO_PORTA_BASE, GPIO_PIN_3) & GPIO_PIN_3) != GPIO_PIN_3)
                {
                    while ((GPIOPinRead(GPIO_PORTA_BASE, GPIO_PIN_3) & GPIO_PIN_3) != GPIO_PIN_3)
                    {
                        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, false);
                        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, GPIO_PIN_3);
                        vTaskDelay(250 / portTICK_RATE_MS);
                    }
                    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, false);
                    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, false);
                    vTaskDelay(250 / portTICK_RATE_MS);
                }
                else
                {
                    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, false);
                    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, GPIO_PIN_3);
                }
            }
            else
            {
                // GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, false);
                // GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, GPIO_PIN_3);
                // vTaskDelay(5000 / portTICK_RATE_MS);
                if (false == LowerlimitReached)
                {
                    // GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2);
                    // GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, false);
                    // vTaskDelay( 2000 / portTICK_RATE_MS );
                    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, false);
                    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, false);
                    GPIOIntClear(GPIO_PORTA_BASE, GPIO_PIN_3);
                }
                // if (true == Jam)
                // {
                //     GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2);
                //     GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, false);
                //     vTaskDelay(1500 / portTICK_RATE_MS);
                //     GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, false);
                //     GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, false);
                //     Jam = false;
                // }
            }
        }
        taskYIELD();
    };
}
/********************************************************************************
 * FUNCTION NAME:       vTaskFunctionDriverUp
 * \param  [in]         void*
 * \param  [out]        void
 *
 *
 ********************************************************************************/
void vTaskFunctionDriverUp(void *pvParameters)
{
    while (1)
    {
        xSemaphoreTake(xSemaphoreDriverUp, portMAX_DELAY);
        // Process interrupt for pin 4 port A
        vTaskDelay(250 / portTICK_RATE_MS);
        LowerlimitReached = false;
        UpperlimitReached = false;
        if ((GPIOPinRead(GPIO_PORTA_BASE, GPIO_PIN_4) & GPIO_PIN_4) != GPIO_PIN_4)
        {
            vTaskDelay(250 / portTICK_RATE_MS);
            if ((GPIOPinRead(GPIO_PORTA_BASE, GPIO_PIN_4) & GPIO_PIN_4) != GPIO_PIN_4)
            {
                while ((GPIOPinRead(GPIO_PORTA_BASE, GPIO_PIN_4) & GPIO_PIN_4) != GPIO_PIN_4)
                {
                    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2);
                    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, false);
                    vTaskDelay(250 / portTICK_RATE_MS);
                }
                GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, false);
                GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, false);
                vTaskDelay(250 / portTICK_RATE_MS);
            }
            else
            {
                GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2);
                GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, false);
            }
        }
        else
        {
            // GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2);
            // GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, false);
            // vTaskDelay(5000 / portTICK_RATE_MS);
            if (false == UpperlimitReached)
            {
                // GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, false);
                // GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, GPIO_PIN_3);
                // vTaskDelay( 2000 / portTICK_RATE_MS );
                GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, false);
                GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, false);
                GPIOIntClear(GPIO_PORTA_BASE, GPIO_PIN_4);
            }
            // if (true == Jam)
            // {
            //     GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, false);
            //     GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, GPIO_PIN_3);
            //     vTaskDelay(1500 / portTICK_RATE_MS);
            //     GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, false);
            //     GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, false);
            //     Jam = false;
            // }
        }
        taskYIELD();
    }
}
/********************************************************************************
 * FUNCTION NAME:       vTaskFunctionDriverDown
 * \param  [in]         void*
 * \param  [out]        void
 *
 *
 ********************************************************************************/
void vTaskFunctionDriverDown(void *pvParameters)
{
    while (1)
    {
        xSemaphoreTake(xSemaphoreDriverDown, portMAX_DELAY);
        // Process interrupt for pin 5 port A
        vTaskDelay(250 / portTICK_RATE_MS);
        LowerlimitReached = false;
        UpperlimitReached = false;

        if ((GPIOPinRead(GPIO_PORTA_BASE, GPIO_PIN_5) & GPIO_PIN_5) != GPIO_PIN_5)
        {
            vTaskDelay(250 / portTICK_RATE_MS);
            if ((GPIOPinRead(GPIO_PORTA_BASE, GPIO_PIN_5) & GPIO_PIN_5) != GPIO_PIN_5)
            {
                while ((GPIOPinRead(GPIO_PORTA_BASE, GPIO_PIN_5) & GPIO_PIN_5) != GPIO_PIN_5)
                {
                    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, false);
                    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, GPIO_PIN_3);
                    vTaskDelay(250 / portTICK_RATE_MS);
                }
                GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, false);
                GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, false);
                vTaskDelay(250 / portTICK_RATE_MS);
            }
            else
            {
                GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, false);
                GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, GPIO_PIN_3);
            }
        }
        else
        {
            // GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, false);
            // GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, GPIO_PIN_3);
            // vTaskDelay(5000 / portTICK_RATE_MS);
            if (false == LowerlimitReached)
            {
                // GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2);
                // GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, false);
                // vTaskDelay( 2000 / portTICK_RATE_MS );
                GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, false);
                GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, false);
                GPIOIntClear(GPIO_PORTA_BASE, GPIO_PIN_5);
            }
            // if (true == Jam)
            // {
            //     GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2);
            //     GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, false);
            //     vTaskDelay(1500 / portTICK_RATE_MS);
            //     GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, false);
            //     GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, false);
            //     Jam = false;
            // }
        }
        taskYIELD();
    }
}
/********************************************************************************
 * FUNCTION NAME:       vTaskFunctionLowerLimit
 * \param  [in]         void*
 * \param  [out]        void
 *
 *
 ********************************************************************************/
void vTaskFunctionLowerLimit(void *pvParameters)
{
    while (1)
    {
        xSemaphoreTake(xSemaphoreLowerLimit, portMAX_DELAY);
        // Process interrupt for pin 6 port F
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, false);
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, false);
        LowerlimitReached = true;
    }
}
/********************************************************************************
 * FUNCTION NAME:       vTaskFunctionUpperLimit
 * \param  [in]         void*
 * \param  [out]        void
 *
 *
 ********************************************************************************/
void vTaskFunctionUpperLimit(void *pvParameters)
{
    while (1)
    {
        xSemaphoreTake(xSemaphoreUpperLimit, portMAX_DELAY);
        // Process interrupt for pin 7 port F
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, false);
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, false);
        UpperlimitReached = true;
    }
}

// TODO added function
void vTaskFunctionJam(void *pvParameters)
{
    while (1)
    {
        xSemaphoreTake(xSemaphoreJam, portMAX_DELAY);
        // Process interrupt for pin 7 port A
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, false);
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, GPIO_PIN_3);
        vTaskDelay(500 / portTICK_RATE_MS);
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, false);
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, false);
        Jam = true;
    }
}
/********************************************************************************
 * FUNCTION NAME:       GPIOPortA_Handler
 * \param  [in]         void
 * \param  [out]        void
 *
 *
 ********************************************************************************/
void GPIOPortA_Handler(void)
{
    static TickType_t lastTickVal = 0U;
    TickType_t currentTickVal = xTaskGetTickCountFromISR();
    uint32_t intStatus = GPIOIntStatus(GPIO_PORTA_BASE, false);
    /// Get rid of bouncing
    GPIOIntClear(GPIO_PORTA_BASE, GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_7);

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
/********************************************************************************
 * FUNCTION NAME:       GPIOPortF_Handler
 * \param  [in]         void
 * \param  [out]        void
 *
 *
 ********************************************************************************/
void GPIOPortF_Handler(void)
{
    static TickType_t lastTickValF = 0U;
    TickType_t currentTickVal = xTaskGetTickCountFromISR();
    uint32_t intStatus = GPIOIntStatus(GPIO_PORTF_BASE, true);

    GPIOIntClear(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4);

    /// Get rid of bouncing
    if ((currentTickVal - lastTickValF) > minTicksReq)
    {
        if (intStatus & GPIO_PIN_1)
        {
            vISRFunctionLowerLimit();
        }

        if (intStatus & GPIO_PIN_0)
        {
            vISRFunctionUpperLimit();
        }
    }

    lastTickValF = currentTickVal;
}
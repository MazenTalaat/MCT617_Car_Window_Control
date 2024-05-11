/********************************************************************************
 * INCLUDES
 ********************************************************************************/
#include "common.h"
#include "systemConfig.h"
#include "interruptHandler.h"
#include "ISR_Functions.h"

/********************************************************************************
 * STATIC VARIABLES
 ********************************************************************************/
static uint8_t lowerlimitReached = false;
static uint8_t upperlimitReached = false;
static uint8_t jamDetected = false;

/********************************************************************************
 * GLOBAL VARIABLES
 ********************************************************************************/

// Declare semaphore handles for each task
SemaphoreHandle_t xSemaphoreDriverUp,
    xSemaphoreDriverDown,
    xSemaphorePassengerUp,
    xSemaphorePassengerDown,
    xSemaphoreLockWindow,
    xSemaphoreUpperLimit,
    xSemaphoreLowerLimit,
    xSemaphoreJamDetected;

// Declare queue handle for jam task
QueueHandle_t xQueueJamDetected;

/********************************************************************************
 * FUNCTION PROTOTYPES
 ********************************************************************************/
void vTaskFunctionDriverUp(void *pvParameters);
void vTaskFunctionDriverDown(void *pvParameters);
void vTaskFunctionPassengerUp(void *pvParameters);
void vTaskFunctionPassengerDown(void *pvParameters);
void vTaskFunctionLowerLimit(void *pvParameters);
void vTaskFunctionUpperLimit(void *pvParameters);
void vTaskFunctionJamDetectedSender(void *pvParameters);
void vTaskFunctionJamDetectedReceiver(void *pvParameters);

int main(void)
{
    // Init the MCU
    CLK_Init();
    GPIO_Init();
    INT_Init();

    // Create the semaphores
    xSemaphorePassengerUp = xSemaphoreCreateBinary();
    xSemaphorePassengerDown = xSemaphoreCreateBinary();
    xSemaphoreDriverUp = xSemaphoreCreateBinary();
    xSemaphoreDriverDown = xSemaphoreCreateBinary();
    xSemaphoreLockWindow = xSemaphoreCreateBinary();
    xSemaphoreLowerLimit = xSemaphoreCreateBinary();
    xSemaphoreUpperLimit = xSemaphoreCreateBinary();
    xSemaphoreJamDetected = xSemaphoreCreateBinary();

    // Create the queue
    xQueueJamDetected = xQueueCreate(3, sizeof(char));

    // Create the tasks
    xTaskCreate(vTaskFunctionDriverUp, "Task Driver Up", configMINIMAL_STACK_SIZE, NULL, 1U, NULL);
    xTaskCreate(vTaskFunctionDriverDown, "Task Driver Down", configMINIMAL_STACK_SIZE, NULL, 1U, NULL);
    xTaskCreate(vTaskFunctionPassengerUp, "Task Passenger Up", configMINIMAL_STACK_SIZE, NULL, 1U, NULL);
    xTaskCreate(vTaskFunctionPassengerDown, "Task Passenger Down", configMINIMAL_STACK_SIZE, NULL, 1U, NULL);
    xTaskCreate(vTaskFunctionLowerLimit, "Task Lower Limit", configMINIMAL_STACK_SIZE, NULL, 1U, NULL);
    xTaskCreate(vTaskFunctionUpperLimit, "Task Upper Limit", configMINIMAL_STACK_SIZE, NULL, 1U, NULL);
    xTaskCreate(vTaskFunctionJamDetectedSender, "Task Jam Detected Sender", configMINIMAL_STACK_SIZE, NULL, 1U, NULL);
    xTaskCreate(vTaskFunctionJamDetectedReceiver, "Task Jam Detected Receiver", configMINIMAL_STACK_SIZE, NULL, 1U, NULL);

    // Start the scheduler
    vTaskStartScheduler();

    // Should never reach here
    for (;;)
        ;
    return 0;
}

/*********************************************************************************************************
** Function name:       vTaskFunctionDriverUp
** Descriptions:        Handles window up functionality from the driver side
** input parameters:    NULL
** output parameters:   none
** Returned value:      none
*********************************************************************************************************/
void vTaskFunctionDriverUp(void *pvParameters)
{
    while (1)
    {
        xSemaphoreTake(xSemaphoreDriverUp, portMAX_DELAY);
        // Process interrupt for pin 4 port A
        vTaskDelay(250 / portTICK_RATE_MS);
        lowerlimitReached = false;
        jamDetected = false;
        // Check if button is pressed
        if ((GPIOPinRead(GPIO_PORTA_BASE, GPIO_PIN_4) & GPIO_PIN_4) != GPIO_PIN_4)
        {
            vTaskDelay(250 / portTICK_RATE_MS);
            // Check if button is pressed for 500ms (250+250)
            if ((GPIOPinRead(GPIO_PORTA_BASE, GPIO_PIN_4) & GPIO_PIN_4) != GPIO_PIN_4)
            {
                // In case of continous press for 500ms keep the motors ON until button release or limit is reached
                while (((GPIOPinRead(GPIO_PORTA_BASE, GPIO_PIN_4) & GPIO_PIN_4) != GPIO_PIN_4) && !upperlimitReached && !jamDetected)
                {
                    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2);
                    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, false);
                }
                GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, false);
                GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, false);
            }
            else
            {
                // In case of single press keep the motors ON until limit is reached
                if (!upperlimitReached)
                {
                    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2);
                    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, false);
                }
            }
        }
        taskYIELD();
    }
}

/*********************************************************************************************************
** Function name:       vTaskFunctionDriverDown
** Descriptions:        Handles window down functionality from the driver side
** input parameters:    NULL
** output parameters:   none
** Returned value:      none
*********************************************************************************************************/
void vTaskFunctionDriverDown(void *pvParameters)
{
    while (1)
    {
        xSemaphoreTake(xSemaphoreDriverDown, portMAX_DELAY);
        // Process interrupt for pin 5 port A
        vTaskDelay(250 / portTICK_RATE_MS);
        upperlimitReached = false;
        jamDetected = false;
        // Check if button is pressed
        if ((GPIOPinRead(GPIO_PORTA_BASE, GPIO_PIN_5) & GPIO_PIN_5) != GPIO_PIN_5)
        {
            // Check if button is pressed for 500ms (250+250)
            vTaskDelay(250 / portTICK_RATE_MS);
            if ((GPIOPinRead(GPIO_PORTA_BASE, GPIO_PIN_5) & GPIO_PIN_5) != GPIO_PIN_5)
            {
                // In case of continous press for 500ms keep the motors ON until button release or limit is reached
                while (((GPIOPinRead(GPIO_PORTA_BASE, GPIO_PIN_5) & GPIO_PIN_5) != GPIO_PIN_5) && !lowerlimitReached)
                {
                    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, false);
                    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, GPIO_PIN_3);
                }
                GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, false);
                GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, false);
            }
            else
            {
                // In case of single press keep the motors ON until limit is reached
                if (!lowerlimitReached)
                {
                    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, false);
                    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, GPIO_PIN_3);
                }
            }
        }
        taskYIELD();
    }
}

/*********************************************************************************************************
** Function name:       vTaskFunctionPassengerUp
** Descriptions:        Handles window up functionality from the passenger side
** input parameters:    NULL
** output parameters:   none
** Returned value:      none
*********************************************************************************************************/
void vTaskFunctionPassengerUp(void *pvParameters)
{
    while (1)
    {
        xSemaphoreTake(xSemaphorePassengerUp, portMAX_DELAY);
        // Process interrupt for pin 2 port A
        // Check if lock window is disabled
        if ((GPIOPinRead(GPIO_PORTA_BASE, GPIO_PIN_6) & GPIO_PIN_6) == GPIO_PIN_6)
        {
            vTaskDelay(250 / portTICK_RATE_MS);
            lowerlimitReached = false;
            jamDetected = false;
            // Check if button is pressed
            if ((GPIOPinRead(GPIO_PORTA_BASE, GPIO_PIN_2) & GPIO_PIN_2) != GPIO_PIN_2)
            {
                // Check if button is pressed for 500ms (250+250)
                vTaskDelay(250 / portTICK_RATE_MS);
                if ((GPIOPinRead(GPIO_PORTA_BASE, GPIO_PIN_2) & GPIO_PIN_2) != GPIO_PIN_2)
                {
                    // In case of continous press for 500ms keep the motors ON until button release or limit is reached
                    while (((GPIOPinRead(GPIO_PORTA_BASE, GPIO_PIN_2) & GPIO_PIN_2) != GPIO_PIN_2) && !upperlimitReached && !jamDetected)
                    {
                        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2);
                        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, false);
                    }
                    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, false);
                    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, false);
                }
                else
                {
                    // In case of single press keep the motors ON until limit is reached
                    if (!upperlimitReached)
                    {
                        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2);
                        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, false);
                    }
                }
            }
        }
        taskYIELD();
    }
}

/*********************************************************************************************************
** Function name:       vTaskFunctionPassengerDown
** Descriptions:        Handles window down functionality from the passenger side
** input parameters:    NULL
** output parameters:   none
** Returned value:      none
*********************************************************************************************************/
void vTaskFunctionPassengerDown(void *pvParameters)
{
    while (1)
    {
        xSemaphoreTake(xSemaphorePassengerDown, portMAX_DELAY);
        // Process interrupt for pin 3 port A
        // Check if lock window is disabled
        if ((GPIOPinRead(GPIO_PORTA_BASE, GPIO_PIN_6) & GPIO_PIN_6) == GPIO_PIN_6)
        {
            vTaskDelay(250 / portTICK_RATE_MS);
            upperlimitReached = false;
            jamDetected = false;
            // Check if button is pressed
            if ((GPIOPinRead(GPIO_PORTA_BASE, GPIO_PIN_3) & GPIO_PIN_3) != GPIO_PIN_3)
            {
                // Check if button is pressed for 500ms (250+250)
                vTaskDelay(250 / portTICK_RATE_MS);
                if ((GPIOPinRead(GPIO_PORTA_BASE, GPIO_PIN_3) & GPIO_PIN_3) != GPIO_PIN_3)
                {
                    // In case of continous press for 500ms keep the motors ON until button release or limit is reached
                    while (((GPIOPinRead(GPIO_PORTA_BASE, GPIO_PIN_3) & GPIO_PIN_3) != GPIO_PIN_3) && !lowerlimitReached)
                    {
                        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, false);
                        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, GPIO_PIN_3);
                    }
                    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, false);
                    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, false);
                }
                else
                {
                    // In case of single press keep the motors ON until limit is reached
                    if (!lowerlimitReached)
                    {
                        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, false);
                        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, GPIO_PIN_3);
                    }
                }
            }
        }
        taskYIELD();
    }
}

/*********************************************************************************************************
** Function name:       vTaskFunctionLowerLimit
** Descriptions:        Handles stopping the window when reaching the limit
** input parameters:    NULL
** output parameters:   none
** Returned value:      none
*********************************************************************************************************/
void vTaskFunctionLowerLimit(void *pvParameters)
{
    // Process interrupt for pin 6 port F
    while (1)
    {
        xSemaphoreTake(xSemaphoreLowerLimit, portMAX_DELAY);
        lowerlimitReached = true;
        upperlimitReached = false;
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, false);
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, false);
    }
}

/*********************************************************************************************************
** Function name:       vTaskFunctionUpperLimit
** Descriptions:        Handles stopping the window when reaching the limit
** input parameters:    NULL
** output parameters:   none
** Returned value:      none
*********************************************************************************************************/
void vTaskFunctionUpperLimit(void *pvParameters)
{
    // Process interrupt for pin 7 port F
    while (1)
    {
        xSemaphoreTake(xSemaphoreUpperLimit, portMAX_DELAY);
        upperlimitReached = true;
        lowerlimitReached = false;
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, false);
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, false);
    }
}

/*********************************************************************************************************
** Function name:       vTaskFunctionJamDetectedSender
** Descriptions:        Sends instructions to handles the jam
** input parameters:    NULL
** output parameters:   none
** Returned value:      none
*********************************************************************************************************/
void vTaskFunctionJamDetectedSender(void *pvParameters)
{
    char cValueToSend;
    // Process interrupt for pin 7 port A
    while (1)
    {
        if (!jamDetected)
        {
            xSemaphoreTake(xSemaphoreJamDetected, portMAX_DELAY);
            jamDetected = true;
            // Send S: stop then D: down then S
            cValueToSend = 'S';
            xQueueSendToBack(xQueueJamDetected, &cValueToSend, 0);
            cValueToSend = 'D';
            xQueueSendToBack(xQueueJamDetected, &cValueToSend, 0);
            cValueToSend = 'S';
            xQueueSendToBack(xQueueJamDetected, &cValueToSend, 0);
        }
        // When finished switch tasks
        taskYIELD();
    }
}

/*********************************************************************************************************
** Function name:       vTaskFunctionJamDetectedReceiver
** Descriptions:        Handles the jam instructions
** input parameters:    NULL
** output parameters:   none
** Returned value:      none
*********************************************************************************************************/
void vTaskFunctionJamDetectedReceiver(void *pvParameters)
{
    char cReceivedValue;
    portBASE_TYPE xStatus;
    while (1)
    {
        // Read the instructions to handle the jam
        xStatus = xQueueReceive(xQueueJamDetected, &cReceivedValue, 0);
        if (xStatus == pdPASS)
        {
            switch (cReceivedValue)
            {
            case 'S':
                // Stop the motor
                GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, false);
                GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, false);
                vTaskDelay(50 / portTICK_RATE_MS);
                break;
            case 'U':
                // Go up
                GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2);
                GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, false);
                vTaskDelay(500 / portTICK_RATE_MS);
                break;
            case 'D':
                // Go down
                GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, false);
                GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, GPIO_PIN_3);
                vTaskDelay(500 / portTICK_RATE_MS);
                break;
            default:
                GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, false);
                GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, false);
                break;
            }
        }
        else
        {
            // When finished switch tasks (queue is empty)
            taskYIELD();
        }
    }
}

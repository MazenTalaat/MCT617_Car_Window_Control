/********************************************************************************
 * INCLUDES
 ********************************************************************************/
#include "systemConfig.h"

/*********************************************************************************************************
** Function name:       CLK_Init
** Descriptions:        Set the system clock to 40MHz
** input parameters:    none
** output parameters:   none
** Returned value:      none
*********************************************************************************************************/
void CLK_Init(void)
{
  // Set the system clock to 40MHz
  SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN);
}

/*********************************************************************************************************
** Function name:       GPIO_Init
** Descriptions:        Config GPIO pins
** input parameters:    none
** output parameters:   none
** Returned value:      none
*********************************************************************************************************/
void GPIO_Init(void)
{
  // Enable the GPIO port that is used for the driver and passenger buttons, jam button and lock switch
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
  // Enable the GPIO port that is used for the motor, limit switches
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

  // Configure the driver and passenger buttons, jam button and lock switch as inputs
  GPIOPinTypeGPIOInput(GPIO_PORTA_BASE, GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7);
  GPIOPadConfigSet(GPIO_PORTA_BASE, GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

  // Configure the limit switch pins as inputs
  GPIOUnlockPin(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_1);
  GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_1);
  GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_1, GPIO_STRENGTH_12MA, GPIO_PIN_TYPE_STD_WPU);

  // Configure the motor pins as outputs
  GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_2 | GPIO_PIN_3);
}

/*********************************************************************************************************
** Function name:       INT_Init
** Descriptions:        Config interrupts
** input parameters:    none
** output parameters:   none
** Returned value:      none
*********************************************************************************************************/
void INT_Init(void)
{
  // Configure the interrupts for the switch pins of Port A
  IntEnable(INT_GPIOA);
  GPIOIntTypeSet(GPIO_PORTA_BASE, GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_7, GPIO_FALLING_EDGE);
  GPIOIntRegister(GPIO_PORTA_BASE, GPIOA_Handler);
  GPIOIntClear(GPIO_PORTA_BASE, GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_7);
  GPIOIntEnable(GPIO_PORTA_BASE, GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_7);
  IntPrioritySet(INT_GPIOA, configMAX_SYSCALL_INTERRUPT_PRIORITY + 1U);

  // Configure the interrupts for the switch pins of Port F
  IntEnable(INT_GPIOF);
  GPIOIntTypeSet(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_1, GPIO_BOTH_EDGES);
  GPIOIntRegister(GPIO_PORTF_BASE, GPIOF_Handler);
  GPIOIntClear(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_1);
  GPIOIntEnable(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_1);
  IntPrioritySet(INT_GPIOF, configMAX_SYSCALL_INTERRUPT_PRIORITY + 1U);
}
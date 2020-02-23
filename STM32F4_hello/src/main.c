/**
  ******************************************************************************
  * @file    main.c
  * @author  Kapil Sharma
  * @version V1.0
  * @date    01-November-2019
  * @brief   Default main function.
  ******************************************************************************
*/

#include "stdio.h"
#include "stdint.h"
#include "string.h"
#include "stm32f4xx.h"
#include "FreeRTOS.h"
#include "task.h"

#include "SEGGER_SYSVIEW.h"


TaskHandle_t xTaskhandle1 = NULL;
TaskHandle_t xTaskhandle2 = NULL;

//Task  Functions prototype
void vTask1_handler(void *params);
void vTask2_handler(void *params);

#ifdef USE_SEMIHOSTING
//used for semihosting
extern void initialise_monitor_handles();
#endif

//Some Macros
#define TRUE 1
#define FALSE 0
#define AVAILABLE TRUE
#define NOT_AVAILABLE FALSE

static void prvSetupHardware(void);
static void prvSetupUart(void);
void PrintMsg(char *msg);

//Global Variable Section
char usr_msg[250];
uint8_t UART_ACCESS_KEY = AVAILABLE;

int main(void)
{
#ifdef USE_SEMIHOSTING
	initialise_monitor_handles();
	printf("This is first RTOS Code\n");
#endif

	//Enable the CYCCNT in DWT_CTRL
	DWT->CTRL |= (1<<0);

	//1.Resets the RCC clock configuration to the default reset state
	RCC_DeInit();

	//2.update the system core clock variable
	SystemCoreClockUpdate();

	prvSetupHardware();

	sprintf(usr_msg, "This  is  hello world ms g\r\n");
	PrintMsg(usr_msg);

	//Start Recording
	SEGGER_SYSVIEW_Conf();
	SEGGER_SYSVIEW_Start();

	//3.Lets  create two task task1 and task2
	xTaskCreate(vTask1_handler, "Task1", configMINIMAL_STACK_SIZE, NULL, 2, xTaskhandle1);
	xTaskCreate(vTask2_handler, "Task2", configMINIMAL_STACK_SIZE, NULL, 2, xTaskhandle2);

	//Start the scheduler
	vTaskStartScheduler();

	for(;;);
}

void vTask1_handler(void *params)
{
	while(1)
	{
		if(UART_ACCESS_KEY == AVAILABLE)
		{
		UART_ACCESS_KEY = NOT_AVAILABLE;
		PrintMsg("Hello World: from Task-1\r\n");
		UART_ACCESS_KEY = AVAILABLE;
		SEGGER_SYSVIEW_Print("Task 1 is running");
		traceISR_EXIT_TO_SCHEDULER();
		taskYIELD();
		}
	}
}

void vTask2_handler(void *params)
{
	while(1)
	{
		if(UART_ACCESS_KEY == AVAILABLE)
		{
		UART_ACCESS_KEY = NOT_AVAILABLE;
		PrintMsg("Hello World: from Task-2\r\n");
		UART_ACCESS_KEY = AVAILABLE;
		SEGGER_SYSVIEW_Print("Task 1 is running");
		traceISR_EXIT_TO_SCHEDULER();
		taskYIELD();
		}
	}
}

static void prvSetupUart(void)
{
	GPIO_InitTypeDef gpio_uart_pins;
	USART_InitTypeDef uart2_init;

	//1.Enable the UART2  and GPIOA peripheral clock
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

	//PA2 is UART2_Txand PA3 is UART2_Rx
	//2. Alternate  function configuration of MCU pin to behave as UART2 Tx and Rx
	//Zeroing each and every member element of the structure
	memset(&gpio_uart_pins, 0,sizeof(gpio_uart_pins));
	gpio_uart_pins.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3;
	gpio_uart_pins.GPIO_Mode = GPIO_Mode_AF;
	gpio_uart_pins.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOA, &gpio_uart_pins);

	//3. AF mode settings for the Pin settings
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource2,GPIO_AF_USART2);		//PA2
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource3,GPIO_AF_USART2);		//PA3

	//4.UART parameter initialization
	//Zeroing each and every member element of the structure
	memset(&uart2_init, 0,sizeof(uart2_init));
	uart2_init.USART_BaudRate = 115200;
	uart2_init.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	uart2_init.USART_Mode = USART_Mode_Tx|USART_Mode_Rx;
	uart2_init.USART_Parity =  USART_Parity_No;
	uart2_init.USART_StopBits = USART_StopBits_1;
	uart2_init.USART_WordLength = USART_WordLength_8b;
	USART_Init(USART2, &uart2_init);

	//5.Enable the UART Peripheral
	USART_Cmd(USART2,ENABLE);
}

static void prvSetupHardware(void)
{
	//Setup Uart2
	prvSetupUart();
}

void PrintMsg(char *msg)
{
	for(uint32_t i=0; i<strlen(msg); i++)
	{
		while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) != SET);
		USART_SendData(USART2, msg[i]);
	}
}





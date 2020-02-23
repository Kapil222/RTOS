/**
  ******************************************************************************
  * @file    main.c
  * @author  Kapil Sharma
  * @version V1.0
  * @date    26-November-2019
  * @brief   Task Delete Code
  ******************************************************************************
*/

#include "stdio.h"
#include "stdint.h"
#include "string.h"
#include "stm32f4xx.h"
#include "FreeRTOS.h"
#include "task.h"


TaskHandle_t xTaskHandle1 = NULL;
TaskHandle_t xTaskHandle2 = NULL;

#define TRUE 1
#define FALSE 0


//Function Prototype
static void prvSetupHardware(void);
void PrintMsg(char *msg);
static void prvSetupUart(void);
void prvSetupGPIO(void);
void rtos_delay(uint32_t delay_in_ms);

//Task Handler prototype
void vtask1_handler(void  *params);
void vtask2_handler(void  *params);

//Global Space for variables
char usr_msg[250];



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

	sprintf(usr_msg, "This  is  Task Delete code\r\n");
	PrintMsg(usr_msg);

	//Start Recording
	SEGGER_SYSVIEW_Conf();
	SEGGER_SYSVIEW_Start();

	//LED-Task Creation
	xTaskCreate(vtask1_handler, "TASK-1", 500, NULL, 1,&xTaskHandle1 );

	//Button-Task Creation
	xTaskCreate(vtask2_handler, "TASK-2", 500, NULL, 2, &xTaskHandle2 );

	//Start the Scheduler
	vTaskStartScheduler();


	for(;;);
}

void vtask1_handler(void  *params)
{
	TickType_t current_tick = 0;
	sprintf(usr_msg, "Task-1 is running\r\n");
	PrintMsg(usr_msg);
	while(1)
	{
		//rtos_delay(200);
		vTaskDelay(200);
		GPIO_ToggleBits(GPIOD, GPIO_Pin_12);
	}
}

void vtask2_handler(void  *params)
{
	TickType_t current_tick = 0;
	sprintf(usr_msg, "Task-2 is running\r\n");
	PrintMsg(usr_msg);
	while(1)
	{
		if(!GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0))
		{
			//Button is not pressed on board
			//toggle led for every 1sec
			rtos_delay(1000);
			GPIO_ToggleBits(GPIOD, GPIO_Pin_12);
		}
		else
		{
			//Button is pressed on the board
			//Task2 delete itself
			sprintf(usr_msg, "Task-2 is getting deleted\r\n");
			PrintMsg(usr_msg);
			vTaskDelete(NULL);
		}
	}
}

static void prvSetupHardware(void)
{
	//Setup Button and LED
	prvSetupGPIO();

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

void rtos_delay(uint32_t delay_in_ms)
{
	uint32_t  current_tick_count = xTaskGetTickCount();
	uint32_t delay_in_ticks = (delay_in_ms * configTICK_RATE_HZ)/1000;
	while(xTaskGetTickCount() < (current_tick_count + delay_in_ticks));
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

void prvSetupGPIO(void)
{
	// Enable peripheral clock for Port D and Port A
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

	//1.Create variable for GPIO Structure
	GPIO_InitTypeDef led_init, button_init;

	//2.Select GPIO Mode
	led_init.GPIO_Mode = GPIO_Mode_OUT;
	led_init.GPIO_OType = GPIO_OType_PP;
	led_init.GPIO_Pin = GPIO_Pin_12;
	led_init.GPIO_Speed = GPIO_Low_Speed;
	led_init.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOD, &led_init);

	button_init.GPIO_Mode = GPIO_Mode_IN;
	button_init.GPIO_OType = GPIO_OType_PP;
	button_init.GPIO_Pin = GPIO_Pin_0;
	button_init.GPIO_Speed = GPIO_Low_Speed;
	button_init.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &button_init);

}


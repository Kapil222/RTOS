/**
  ******************************************************************************
  * @file    main.c
  * @author  Kapil Sharma
  * @version V1.0
  * @date	 28 Nov 2019
  * @brief   Task Delay
  ******************************************************************************
*/

#include<string.h>
#include<stdio.h>
#include<stdint.h>
#include "stm32f4xx.h"

#include "FreeRTOS.h"
#include "task.h"

#define TRUE 1
#define FALSE 0


//function prototypes
static void prvSetupHardware(void);
void printmsg(char *msg);
static void prvSetupUart(void);
void prvSetupGPIO(void);

//tasks prototypes
void vTask1_handler(void *params);
void vTask2_handler(void *params);



//global space for some variable
TaskHandle_t xTaskHandle1=NULL;
TaskHandle_t xTaskHandle2=NULL;

char usr_msg[250]={0};


int main(void)
{
	DWT->CTRL |= (1 << 0);//Enable CYCCNT in DWT_CTRL.

	//1.  Reset the RCC clock configuration to the default reset state.
	//HSI ON, PLL OFF, HSE OFF, system clock = 16MHz, cpu_clock = 16MHz
	RCC_DeInit();

	//2. update the SystemCoreClock variable
	SystemCoreClockUpdate();

	prvSetupHardware();

	//Start Recording
	SEGGER_SYSVIEW_Conf();
	SEGGER_SYSVIEW_Start();

	sprintf(usr_msg,"This is Task Switching Priority Demo\r\n");
	printmsg(usr_msg);


	//lets create task-1
    xTaskCreate(vTask1_handler,"TASK-1",500,NULL,2,&xTaskHandle1);

    //lets create task-2
    xTaskCreate(vTask2_handler,"TASK-2",500,NULL,3,&xTaskHandle2);

    //lets start the scheduler
    vTaskStartScheduler();

	for(;;);
}


void vTask1_handler(void *params)
{
	while(1)
	{
		sprintf(usr_msg, "Status of LED is %d\r\n",  GPIO_ReadOutputDataBit(GPIOD, GPIO_Pin_12));
		printmsg(usr_msg);
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}


void vTask2_handler(void *params)
{
	while(1)
	{
		GPIO_ToggleBits(GPIOD,GPIO_Pin_12);
		vTaskDelay(pdMS_TO_TICKS(1000));
	}

}

static void prvSetupHardware(void)
{
	//Setup Button and LED
	prvSetupGPIO();

	//setup UART2
	prvSetupUart();
}

void printmsg(char *msg)
{
	for(uint32_t i=0; i < strlen(msg); i++)
	{
		while ( USART_GetFlagStatus(USART2,USART_FLAG_TXE) != SET);
		USART_SendData(USART2,msg[i]);
	}

	while ( USART_GetFlagStatus(USART2,USART_FLAG_TC) != SET);

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


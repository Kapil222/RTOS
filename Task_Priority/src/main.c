/**
  ******************************************************************************
  * @file    main.c
  * @author  Kapil Sharma
  * @version V1.0
  * @brief   Task Priority
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

void rtos_delay(uint32_t delay_in_ms);

//global space for some variable
TaskHandle_t xTaskHandle1=NULL;
TaskHandle_t xTaskHandle2=NULL;

char usr_msg[250]={0};
uint8_t switch_prio = FALSE;

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

	UBaseType_t p1,p2;

	sprintf(usr_msg,"Task-1 is running\r\n");
	printmsg(usr_msg);

	sprintf(usr_msg,"Task-1 Priority %ld\r\n",uxTaskPriorityGet(xTaskHandle1));
	printmsg(usr_msg);

	sprintf(usr_msg,"Task-2 Priority %ld\r\n",uxTaskPriorityGet(xTaskHandle2));
	printmsg(usr_msg);

	while(1)
	{
		if( switch_prio )
		{
			switch_prio = FALSE;

			p1 = uxTaskPriorityGet(xTaskHandle1);
			p2 = uxTaskPriorityGet(xTaskHandle2);

			//switch prio
			vTaskPrioritySet(xTaskHandle1,p2);
			vTaskPrioritySet(xTaskHandle2,p1);



		}else
		{
			GPIO_ToggleBits(GPIOA,GPIO_Pin_5);
			rtos_delay(200);
		}

	}
}


void vTask2_handler(void *params)
{

	UBaseType_t p1,p2;

	sprintf(usr_msg,"Task-2 is running\r\n");
	printmsg(usr_msg);

	sprintf(usr_msg,"Task-1 Priority %ld\r\n",uxTaskPriorityGet(xTaskHandle1));
	printmsg(usr_msg);

	sprintf(usr_msg,"Task-2 Priority %ld\r\n",uxTaskPriorityGet(xTaskHandle2));
	printmsg(usr_msg);

	while(1)
	{
		if( switch_prio )
		{
			switch_prio = FALSE;

			p1 = uxTaskPriorityGet(xTaskHandle1);
			p2 = uxTaskPriorityGet(xTaskHandle2);

			//switch prio
			vTaskPrioritySet(xTaskHandle1,p2);
			vTaskPrioritySet(xTaskHandle2,p1);


		}else
		{
			GPIO_ToggleBits(GPIOA,GPIO_Pin_5);
			rtos_delay(1000);
		}

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
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG,  ENABLE);

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

	//interrupt Configuration for the button(PA0)
	//1. System Configuration for EXTI Line(SYSCFG setting)
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOA, EXTI_PinSource0);
	//2.EXTI Line Configuration 0line, falling edge,interrupt mode
	EXTI_InitTypeDef exti_init;
	exti_init.EXTI_Line = EXTI_Line0;
	exti_init.EXTI_Mode = EXTI_Mode_Interrupt;
	exti_init.EXTI_Trigger = EXTI_Trigger_Falling;
	exti_init.EXTI_LineCmd = ENABLE;
	EXTI_Init(&exti_init);

	//3.NVIC Setting(IRQ Setting for the selected EXTI  line 0)
	NVIC_SetPriority(EXTI0_IRQn, 5);
	NVIC_EnableIRQ(EXTI0_IRQn);

}

void EXTI0_IRQHandler(void)
{
	traceISR_ENTER();
	//1. Clear the interrupt pending  bit on EXTI line(0)
	EXTI_ClearITPendingBit(EXTI_Line0);
	traceISR_EXIT();

}

void rtos_delay(uint32_t delay_in_ms)
{
	uint32_t tick_count_local=0;

	//converting ms to ticks
	uint32_t delay_in_ticks = ( delay_in_ms  * configTICK_RATE_HZ ) / 1000;

	tick_count_local = xTaskGetTickCount();
	while ( xTaskGetTickCount() < (tick_count_local+delay_in_ticks ));
}

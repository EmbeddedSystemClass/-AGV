#include "key.h"
#include "taskCTRL.h"
#include "SPI.h"
#include "lcd.h"
#include "stm32f10x_gpio.h"
#include "usart.h"
#include "NVICconfig.h"

int main(void)
{
	SystemInit();
	KeyInit();	  //�����ܽų�ʼ��
	LED_GPIO_Config();//LED�ܽų�ʼ��
	CAN_GPIO_Config();//CAN�ܽų�ʼ��
//	CAN_NVIC_Configuration(); //CAN�жϳ�ʼ��   
	CAN_INIT();//CA��ʼ��Nģ��	
	usartConfig();
	spiInit();
	nvicConfig();
	xTaskCreate((TaskFunction_t)usartLcdTask,
		(const char*)"UsartLcdTask",
		(u16)_USART_LCD_STK,
		(void *)NULL,
		(UBaseType_t)_USART_LCD_PRIO,
		(TaskHandle_t *)&UsartLcdTaskHandle);
	vTaskStartScheduler();
}

void usartLcdTask(void * pvParameter)
{
	vTaskDelay(2000);
	USART_OUT(USART2, "SPG(2)\r\n", 8);
	vTaskDelay(500);
	USART_OUT(USART2, "SPG(3)\r\n", 8);
	vTaskDelay(2000);
	while (1)
	{
	//	USART_OUT(USART2, "DS32(174,168,'10.1V',1);\r\n", 26);
	//	usart485Send("hello\r\n", 7);
		can_tx(10,20);
		vTaskDelay(200);
	}
}



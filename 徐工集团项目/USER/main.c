#include "key.h"
#include "taskCTRL.h"
#include "SPI.h"
#include "lcd.h"
#include "stm32f10x_gpio.h"
#include "usart.h"
#include "NVICconfig.h"
#include "ttllcd.h"
#include "charger.h" 
#include "led.h"
#include "stdio.h"
#include "TIM.h"
#include "irda.h"

#define _PROJECT_AGV 0

xQueueHandle CanMsgQueue;
u8 ChargerStatusBack;
void initShowStrings(void);
void lcdShowElectricity(float e);

//************************************
// FunctionName:  main
// Returns:   int
// Qualifier:��������������Ϊ�칤����AGV�����豸���Ƴ�����Ҫ����2��LCD��������485ͨ�ţ�CANͨ�ţ�
//485�����͸��ģ�����ӣ���������ͨ��,CAN��������ӣ���ɶԳ���Ŀ��ؿ��ơ��������õȹ���
// Parameter: void
//************************************
int main(void)
{
	SystemInit();
	keyInit();	  //�����ܽų�ʼ��
	ledGpioConfig();//LED�ܽų�ʼ��
	canGpioConfig();//CAN�ܽų�ʼ��
	spiInit();
	canInit();//CAN��ʼ��Nģ��	
	usartConfig();
	nvicConfig();
	TIM2_Configuration();

	CanMsgQueue = xQueueCreate(17, sizeof(u32));
#if !_PROJECT_AGV
	xTaskCreate((TaskFunction_t)spiLcdTask, \
		(const char*)"UsartLcdTask", \
		(u16)_SPI_LCD_STK, \
		(void *)NULL, \
		(UBaseType_t)_SPI_LCD_PRIO, \
		(TaskHandle_t *)&SPILcdTaskHandle);
#endif
	xTaskCreate((TaskFunction_t)usartIrdaTask, \
		(const char*)"UsartIrdaTask", \
		(u16)_USART_IRDA_STK, \
		(void *)NULL, \
		(UBaseType_t)_USART_IRDA_PRIO, \
		(TaskHandle_t *)&UsartIrdaTaskHandle);
#if !_PROJECT_AGV
	xTaskCreate((TaskFunction_t)canChargeTask, \
		(const char*)"CanChargeTask", \
		(u16)_CAN_CHARGE_STK, \
		(void *)NULL, \
		(UBaseType_t)_CAN_CHARGE_PRIO, \
		(TaskHandle_t *)&CanChargeTaskHandle);
#endif
#if _PROJECT_AGV
	xTaskCreate((TaskFunction_t)ttlLcdTask, \
		(const char*)"TtlLcdTask", \
		(u16)_TTL_LCD_STK, \
		(void *)NULL, \
		(UBaseType_t)_TTL_LCD_PRIO, \
		(TaskHandle_t *)&TtlLcdTaskHandle);
#endif
	vTaskStartScheduler();
}

//************************************
// FullName:  usartLcdTask
// Returns:   void
// Qualifier:LCD�����������LCD����������ʵ
// Parameter: void * pvParameter
//************************************
void spiLcdTask(void * pvParameter)
{
	u8 showbyte = 0;
	float i = 10.2;
	vTaskDelay(200);
	// 	_TTL_LCD_CLR;
	lcdInit();
	// 	vTaskDelay(500);
	// 	_TTL_LCD_SHOW;
	// 	vTaskDelay(2000);
	initShowStrings();
	while (1)
	{
		sprintf((u8 *)&showbyte, "%x", ChargerStatusBack);
		lcdShowString(X2_Y6, &showbyte);
        //lcdShowNumber(X2_Y5, i);
		lcdShowNumber(X3_Y5, ChargerTimeCount / 10.0);
		lcdShowElectricity(i);
// 		ttlLcdMsgSed(CHARGEVOL, 10);
// 		ttlLcdMsgSed(CHARGECUR, 0);
// 		ttlLcdMsgSed(CHARGETIME, 0);
// 		ttlLcdMsgSed(MACHINESTATUS, 0);
// 		ttlLcdMsgSed(BATTERY, TRUE);
// 		ttlLcdMsgSed(COOL, FALSE);
	}
}

//************************************
// FullName:  usartIrdaTask
// Returns:   void
// Qualifier:���ͨ��RS485����͸��ͨ��
// Parameter: void * pvParameter
//************************************
void usartIrdaTask(void * pvParameter)
{
	u8 Debug_send_buff[6] = { 1,2,3,4,5,6 };
	IRDA_RX_TYPE rcv_Msg_Irda;
	pvParameter = (void *)pvParameter;
	while (1)
	{
		Debug_send_buff[1] = rcvMsgFromIrda();
        if(Debug_send_buff[1] != NO_RX)
        {
            usart485Send(Debug_send_buff, 6);
        }
		//usart485Send((u8 *)"Helloworld\r\n", 12);
		vTaskDelay(20);
	}
}

//************************************
// FullName:  canChargeTask
// Returns:   void
// Qualifier:ͨ��canͨ�ſ��Ƴ�������
// Parameter: void * pvParameter
//************************************
void canChargeTask(void *pvParameter)
{
	u8 led1Flag = 0;

	pvParameter = (void *)pvParameter;
	while (1)
	{
		led1Flag ^= 1;
		LED1(led1Flag);
		ChargerStatusBack = chargerCTRLLoop();
		vTaskDelay(500);
	}
}

//************************************
// FullName:  initShowStrings
// Returns:   void
// Qualifier:12864������ʾ������Ϣ��lables��
// Parameter: void
//************************************
void initShowStrings(void)
{
	lcdShowString(X1_Y1, (u8 *)"����������˼���");
	lcdShowString(X2_Y2, (u8 *)"״̬��");  					//lcdShowString(X2_Y8,(u8 *) "%");
	lcdShowString(X3_Y2, (u8 *)"ʱ�䣺");					//	lcdShowString(X3_Y8, (u8 *)"V");
	lcdShowString(X4_Y2, (u8 *)"������");					//	lcdShowString(X4_Y8,(u8 *) "A");
}
//************************************
// FullName:  lcdShowElectricity
// Returns:   void
// Qualifier:������ʾ���ӵ������������жϳ�硢�ŵ磨����AGV��ʱ��Ϊ�ŵ磬��Ϊ���״̬��
// Parameter: float e
//************************************
void lcdShowElectricity(float e)
{
	//lcdShowNumber(X4_Y5, e);
	lcdShowHex(X4_Y5, getCurr());
	// 	if(e > 0)
	// 	{
	// 		lcdShowString(X4_Y5, (u8 *)"+");	
	// 	}
	// 	else
	// 	{
	// 		lcdShowString(X4_Y5, (u8 *) "-");	
	// 	}

}

void ttlLcdTask(void *pvParameter)
{
	float i = 10.2;
	vTaskDelay(200);
	_TTL_LCD_CLR;
	vTaskDelay(500);
	_TTL_LCD_SHOW;
	vTaskDelay(2000);
	while (1)
	{
		ttlLcdMsgSed(CHARGEVOL, 10);
		ttlLcdMsgSed(CHARGECUR, 0);
		ttlLcdMsgSed(CHARGETIME, 0);
		ttlLcdMsgSed(MACHINESTATUS, 0);
		ttlLcdMsgSed(BATTERY, TRUE);
		ttlLcdMsgSed(COOL, FALSE);
	}
}



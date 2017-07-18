#include "TIM.h"

volatile u32 ChargerTimeCount = 0;

void TIM2_Configuration(void)
{
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	TIM_DeInit(TIM2);
	TIM_TimeBaseStructure.TIM_Period = 1000;	//�Զ���װ�ؼĴ������ڵ�ֵ(����ֵ) 
												/* �ۼ� TIM_Period��Ƶ�ʺ����һ�����»����ж� */
	TIM_TimeBaseStructure.TIM_Prescaler = (7200 - 1);	//ʱ��Ԥ��Ƶ�� 72M/7200      
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; //���ϼ���ģʽ 
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
	TIM_ClearFlag(TIM2, TIM_FLAG_Update);	// �������жϱ�־ 
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
	//    TIM_Cmd(TIM2, ENABLE);	// ����ʱ��    
	//    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2 , DISABLE);	//�ȹرյȴ�ʹ��  
}


void TIM2_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
	{
		TIM_ClearITPendingBit(TIM2, TIM_FLAG_Update);
		ChargerTimeCount++;
	}
}


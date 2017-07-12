#include "key.h"
void Delay(vu32 nCount)
{
	for (; nCount != 0; nCount--);
}
/*�����ܽų�ʼ��*/
void keyInit(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);//ʹ������ʱ��
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);//ʹ������ʱ��

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz; //����������10MHz
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;//��������
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz; //����������10MHz
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; //��������
	GPIO_Init(GPIOA, &GPIO_InitStructure);
}
/*����Ƿ��а�������*/
void  GetKey(void)
{
	if (Bit_RESET == GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_13))
	{
		Delay(1000000);//ȥ����//ȥ����
		if (Bit_RESET == GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_13))
		{
			while (Bit_RESET == GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_13)) { ; }//�ȴ������ͷ�                        
			canMsgTx(0X55, 0X77, 0x01, 0x02);
			LED1(1); LED2(1);
		}
	}

	if (Bit_RESET == GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_12))
	{
		Delay(1000000);//ȥ����//ȥ����
		if (Bit_RESET == GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_12))
		{
			while (Bit_RESET == GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_12)) { ; }//�ȴ������ͷ�                        
			canMsgTx(0X99, 0Xbb, 0x01, 0x02);
			LED1(1); LED2(1);
		}
	}
}


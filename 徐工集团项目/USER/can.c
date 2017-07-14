#include "can.h"
#include "led.h"
#include "stdio.h"

/* ���жϴ������з��� */
__IO uint32_t ret = 0;

/*CAN GPIO ��ʱ������ */
void canGpioConfig(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;	 // ��������
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; // �����������
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_PinRemapConfig(GPIO_Remap1_CAN1, ENABLE);
}

/*	CAN��ʼ�� */
void canInit(void)
{
	CAN_InitTypeDef        CAN_InitStructure;
	CAN_FilterInitTypeDef  CAN_FilterInitStructure;
	CAN_DeInit(CAN1);	//������CAN��ȫ���Ĵ�������Ϊȱʡֵ
	CAN_StructInit(&CAN_InitStructure);//��CAN_InitStruct�е�ÿһ��������ȱʡֵ����
	CAN_InitStructure.CAN_TTCM = DISABLE;//û��ʹ��ʱ�䴥��ģʽ
	CAN_InitStructure.CAN_ABOM = DISABLE;//û��ʹ���Զ����߹���
	CAN_InitStructure.CAN_AWUM = DISABLE;//û��ʹ���Զ�����ģʽ
	CAN_InitStructure.CAN_NART = DISABLE;//û��ʹ�ܷ��Զ��ش�ģʽ
	CAN_InitStructure.CAN_RFLM = DISABLE;//û��ʹ�ܽ���FIFO����ģʽ
	CAN_InitStructure.CAN_TXFP = DISABLE;//û��ʹ�ܷ���FIFO���ȼ�
	CAN_InitStructure.CAN_Mode = CAN_Mode_Normal;//CAN����Ϊ����ģʽ
	CAN_InitStructure.CAN_SJW = CAN_SJW_1tq; //����ͬ����Ծ���1��ʱ�䵥λ
	CAN_InitStructure.CAN_BS1 = CAN_BS1_3tq; //ʱ���1Ϊ3��ʱ�䵥λ
	CAN_InitStructure.CAN_BS2 = CAN_BS2_2tq; //ʱ���2Ϊ2��ʱ�䵥λ
	CAN_InitStructure.CAN_Prescaler = 24;  //ʱ�䵥λ����Ϊ24	
	CAN_Init(CAN1, &CAN_InitStructure);
	//������Ϊ��72M/2/24(1+3+2)=0.25 ��250K
/* CAN filter init */
	CAN_FilterInitStructure.CAN_FilterNumber = 1;//ָ��������Ϊ1
	CAN_FilterInitStructure.CAN_FilterMode = CAN_FilterMode_IdMask;//ָ��������Ϊ��ʶ������λģʽ
	CAN_FilterInitStructure.CAN_FilterScale = CAN_FilterScale_32bit;//������λ��Ϊ32λ
	CAN_FilterInitStructure.CAN_FilterIdHigh = 0x0000;// ��������ʶ���ĸ�16λֵ
	CAN_FilterInitStructure.CAN_FilterIdLow = 0x0000;//	 ��������ʶ���ĵ�16λֵ
	CAN_FilterInitStructure.CAN_FilterMaskIdHigh = 0x0000;//���������α�ʶ���ĸ�16λֵ
	CAN_FilterInitStructure.CAN_FilterMaskIdLow = 0x0000;//	���������α�ʶ���ĵ�16λֵ
	CAN_FilterInitStructure.CAN_FilterFIFOAssignment = CAN_FIFO0;// �趨��ָ���������FIFOΪ0
	CAN_FilterInitStructure.CAN_FilterActivation = ENABLE;// ʹ�ܹ�����
	CAN_FilterInit(&CAN_FilterInitStructure);//	������Ĳ�����ʼ��������
	CAN_ITConfig(CAN1, CAN_IT_FMP0, ENABLE); //ʹ��FIFO0��Ϣ�Һ��ж�
}
/* ���������ֽڵ�����*/
void canMsgTx(u8 Data1, u8 Data2, u8 Data3, u8 Data4)
{
	CanTxMsg TxMessage;
	TxMessage.ExtId = 0x181f02f5  ;	//��׼��ʶ��Ϊ0x00
	TxMessage.IDE = CAN_ID_EXT;//ʹ�ñ�׼��ʶ��
	TxMessage.RTR = CAN_RTR_DATA;//Ϊ����֡
	TxMessage.DLC = 4;	//	��Ϣ�����ݳ���Ϊ2���ֽ�
	TxMessage.Data[0] = Data1; //��һ���ֽ�����
	TxMessage.Data[1] = Data2; //�ڶ����ֽ����� 
	TxMessage.Data[2] = Data3; //�������ֽ�����
	TxMessage.Data[3] = Data4; //���ĸ��ֽ�����
	CAN_Transmit(CAN1, &TxMessage); //��������
}


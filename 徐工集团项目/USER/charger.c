#include "charger.h"
#include "led.h"
extern xQueueHandle CanMsgQueue;
static u32 CloseDelay = 0;
static _CHANGER_STATUS ChangerStatus = CLOSE;

static bool ChangerOverFlag = FALSE;

/*��Ҫ����ѭ��*/
//************************************
// FullName:  changerCTRLLoop
// Returns:   extern u8
// Qualifier:��ɳ���������Ҫ����
// Parameter: void
//************************************
extern u8 changerCTRLLoop(void)
{
	if (isCloseDelay())
	{
		return 0x01;//������ʱ�ر�
	}
	else if(isEmmergency())
	{
		return 0x02;//���¼�ͣ
	}
	else if (!isOnConnect())
	{
		return 0x03;//�޳�������
	}
	else if (isChangerNotGood())
	{
		return 0x04;//�����쳣(���Ӽ��)
	}
	else if (!isBattryVolGood())
	{
		return 0x05;//��ص�ѹ�쳣
	}
	else 
	{
		if (CLOSE == checkChangerStatusOpen())//����δ�򿪣��򿪳���
		{
			setChanger();
			openChanger();
			return 0x06;//���ڴ򿪳��׮
		}
		else if (!isCurGood())//�����Ѿ��򿪣���ѯ����
		{
			closeChanger();
			return 0x07;//�����쳣�ر�
		}
		else if (isOverCharge())
		{
			closeChanger();//�����ɹر�
			return 0x08;
		}
	}
	return 0x09; //���������
}

//************************************
// FullName:  setCloseDelay
// Returns:   void
// Qualifier:������ʱ����ʱʱ��Ϊ3s
// Parameter: void
//************************************
static void setCloseDelay(void)
{
	CloseDelay = 150000;
}

//************************************
// FullName:  isOnConnect
// Returns:   bool
// Qualifier:�ж��Ƿ��Ѿ���AGV�Խӳɹ�
// Parameter: void
//************************************
static bool isOnConnect(void)
{

	return TRUE;
}

//************************************
// FullName:  isCloseDelay
// Returns:   bool
// Qualifier:�ж��Ƿ����ڹػ���ʱ
// Parameter: void
//************************************
static bool isCloseDelay(void)
{
	if (0 == CloseDelay)
	{
		return FALSE;
	}
	else
	{
		CloseDelay--;
		return TRUE;
	}
}

/*�����ѯ����*/
//************************************
// FullName:  isChangerNotGood
// Returns:   bool
// Qualifier:��Ч����
// Parameter: void
//************************************
static bool isChangerNotGood(void)
{
	//���Ͳ�ѯ����
	//��������ȴ�����֤
	u8 i = 0;
	u32 RxMsg = 0;
	canMsgTx(_CONNECT_CMD);
	for ( i = 0; i < 16; i++)
	{
		if (pdFALSE == xQueueReceive(CanMsgQueue, &RxMsg, 50))
		{

			return TRUE;
		}
	}
	return FALSE;
}

//************************************
// FullName:  openChanger
// Returns:   void
// Qualifier:�򿪳������ȴ���������
// Parameter: void
//************************************
static void openChanger(void)
{
	u32 RxMsg = 0;
	canMsgTx(_OPEN_CMD);
	xQueueReceive(CanMsgQueue, &RxMsg, 50);
	if (RxMsg != _OPEN_CMD_BACK)
	{
		return;
	}
	canMsgTx(_OPEN_DATA);
	xQueueReceive(CanMsgQueue, &RxMsg, 50);
	if (RxMsg != _OPEN_DATA_BACK)
	{
		return;
	}
	clrChangerOver();
	ChangerStatus = OPEN;
}

//************************************
// FunctionName:  closeChanger
// Returns:   void
// Qualifier:�رճ�����������ʱ
// Parameter: void
//************************************
static void closeChanger(void)
{
	u32 RxMsg = 0;
	canMsgTx(_CLOSE_CMD);
	xQueueReceive(CanMsgQueue, &RxMsg, 50);
	if (RxMsg != _CLODE_CMD_BACK)
	{
		return;
	}
	canMsgTx(_CLOSE_DATA);
	xQueueReceive(CanMsgQueue, &RxMsg, 50);
	if (RxMsg != _CLOSE_DATA_BACK)
	{
		return;
	}
	setCloseDelay();
	ChangerStatus = CLOSE;
}
//************************************
// FunctionName:  setChanger
// Returns:   void
// Qualifier:�����������õ�ǰ�������
// Parameter: void
//************************************
static void setChanger(void)
{
	//������������ȴ�����
	u32 RxMsg = 0;
	canMsgTx(_SET_VOLATE_CMD);
	xQueueReceive(CanMsgQueue, &RxMsg, 50);
	if (RxMsg != _SET_VOLATE_CMD_BACK)
	{
		return;
	}
	canMsgTx(_SET_VOLATE_DATA);
	xQueueReceive(CanMsgQueue, &RxMsg, 50);
	if (RxMsg != _SET_VOLATE_DATA_BACK)
	{
		return;
	}
	canMsgTx(_SET_CURR_CMD);
	xQueueReceive(CanMsgQueue, &RxMsg, 50);
	if (RxMsg != _SET_CURR_CMD_BACK)
	{
		return;
	}
	canMsgTx(_SET_CURR_DATA);
	xQueueReceive(CanMsgQueue, &RxMsg, 50);
	if (RxMsg != _SET_CURR_DATA_BACK)
	{
		return;
	}

}

//************************************
// FunctionName:  isBattryVolGood
// Returns:   bool
// Qualifier:����ص�ѹ�Ƿ�������Ҫ��
// Parameter: void
//************************************
static bool isBattryVolGood(void)
{
	u32 RxMsg = 0;
	canMsgTx(_CHECK_BATTRY_CMD);
	LED2(0);
	xQueueReceive(CanMsgQueue, &RxMsg, 80);
	if (RxMsg != _CHECK_BATTRY_CMD_BACK)
	{
		LED2(1);
		return FALSE;
	}
	canMsgTx(_CHECK_BATTY_DATA);
	if (pdFALSE == xQueueReceive(CanMsgQueue, &RxMsg, 50))
	{
		return FALSE;
	}
	return TRUE;
}

//************************************
// FunctionName:  isCurGood
// Returns:   bool
// Qualifier:��鵱ǰ��������û���쳣���󣬲����������Ƿ����������Ҫ��
// Parameter: void
//************************************
static bool isCurGood(void)
{
	u32 RxMsg = 0;
	canMsgTx(_CHECK_CURR_CMD);
	xQueueReceive(CanMsgQueue, &RxMsg, 50);
	if (RxMsg != _CHECK_CURR_CMD_BACK)
	{
		return FALSE;
	}
	canMsgTx(_CHECK_CURR_DATA);
	if (pdFALSE == xQueueReceive(CanMsgQueue, &RxMsg, 50))
	{
		return FALSE;
	}
	else
	{
		if (RxMsg < 0xf13100ff)//�趨С��ĳֵʱ������������翪ʼʱ�趨���δ����
		{
			setChangerOver();
		}
		return TRUE;
	}
}

//************************************
// FunctionName:  isOverCharge
// Returns:   bool
// Qualifier:����Ƿ��Ѿ�������
// Parameter: void
//************************************
static bool isOverCharge(void)
{
	return ChangerOverFlag;
}

//************************************
// FunctionName:  isEmmergency
// Returns:   bool
// Qualifier:��⼱ͣ��ť�Ƿ���
// Parameter: void
//************************************
static bool isEmmergency(void)
{
	return FALSE;
}

//************************************
// FunctionName:  checkChangerStatusOpen
// Returns:   _CHANGER_STATUS
// Qualifier:��ѯ��ǰ����״̬
// Parameter: void
//************************************
static _CHANGER_STATUS checkChangerStatusOpen(void)
{
	return ChangerStatus;
}

/* USB�жϺ�CAN�����жϷ������USB��CAN����I/O������ֻ�õ�CAN���жϡ� */
void USB_LP_CAN1_RX0_IRQHandler(void)
{
	CanRxMsg RxMessage;
	u32 RxData = 0;
	portBASE_TYPE xHigherPriorityTaskWoken;
	xHigherPriorityTaskWoken = pdFALSE;
	RxMessage.StdId = 0x00;
	RxMessage.ExtId = 0x00;
	RxMessage.IDE = 0;
	RxMessage.DLC = 0;
	RxMessage.FMI = 0;
	RxMessage.Data[0] = 0x00;
	RxMessage.Data[1] = 0x00;
	RxMessage.Data[2] = 0x00;
	RxMessage.Data[3] = 0x00;
	CAN_Receive(CAN1, CAN_FIFO0, &RxMessage); //����FIFO0�е�����  
	RxData = RxMessage.Data[0] << 24 | RxMessage.Data[1] << 16 | RxMessage.Data[2] << 8 | RxMessage.Data[0];
	if (RxMessage.StdId == 0x181ff502)
	{
		xQueueSendFromISR(CanMsgQueue,&RxData, &xHigherPriorityTaskWoken);
	}
}

//************************************
// FunctionName:  setChangerOver
// Returns:   void
// Qualifier:���øú����趨����������ʱ��������С
// Parameter: void
//************************************
static void setChangerOver(void)
{
	ChangerOverFlag = TRUE;
}

//************************************
// FunctionName:  clrChangerOver
// Returns:   void
// Qualifier:��ʼ��磬����ϴγ���������������γ��ˢ�±�־λ����ʾΪδ������
// Parameter: void
//************************************
static void clrChangerOver(void)
{
	ChangerOverFlag = FALSE;
}




#include "charger.h"

extern xQueueHandle CanMsgQueue;
static u32 CloseDelay = 0;
static _CHANGER_STATUS ChangerStatus = CLOSE;

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
		return 0x04;//�����쳣
	}
	else if (!isBattryVolGood())
	{
		return 0x05;//��ص�ѹ�쳣
	}
	else 
	{
		if (CLOSE == checkChangerStatusOpen())
		{
			setChanger();
			openChanger();
			return 0x06;//���ڴ򿪳��׮
		}
		else if (!isCurGood())
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
	u32 RxMsg = 0;
	canMsgTx(0x01, 0x02, 0x03, 0x04);
	xQueueReceive(CanMsgQueue, &RxMsg, 10);
	if (RxMsg == 0x01)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
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
	canMsgTx(0xf1, 0x02, 0x04, 0x87);
	xQueueReceive(CanMsgQueue, &RxMsg, 50);
	if (RxMsg != 0xf1020487)
	{
		return;
	}
	canMsgTx(0xf1, 0x12, 0x00, 0x00);
	xQueueReceive(CanMsgQueue, &RxMsg, 50);
	if (RxMsg != 0xf1120000)
	{
		return;
	}
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
	canMsgTx(0xf1, 0x02, 0x04, 0x87);
	xQueueReceive(CanMsgQueue, &RxMsg, 50);
	if (RxMsg != 0xf1020487)
	{
		return;
	}
	canMsgTx(0xf1, 0x12, 0x00, 0x01);
	xQueueReceive(CanMsgQueue, &RxMsg, 50);
	if (RxMsg != 0xf1120001)
	{
		return;
	}
	setCloseDelay();
	ChangerStatus = CLOSE;
}
static void setChanger(void)
{
	//������������ȴ�����
	u32 RxMsg = 0;
	canMsgTx(0xf1, 0x02, 0x04, 0x21);
	xQueueReceive(CanMsgQueue, &RxMsg, 50);
	if (RxMsg != 0xf1020421)
	{
		return;
	}
	canMsgTx(0xf1, 0x12, 0x0e, 0xb3);
	xQueueReceive(CanMsgQueue, &RxMsg, 50);
	if (RxMsg != 0xf1120eb3)
	{
		return;
	}
	canMsgTx(0xf1, 0x02, 0x04, 0x23);
	xQueueReceive(CanMsgQueue, &RxMsg, 50);
	if (RxMsg != 0xf1020423)
	{
		return;
	}
	canMsgTx(0xf1, 0x12, 0x0a, 0x00);
	xQueueReceive(CanMsgQueue, &RxMsg, 50);
	if (RxMsg != 0xf1120a00)
	{
		return;
	}

}

static bool isBattryVolGood(void)
{
	u32 RxMsg = 0;
	canMsgTx(0xf1, 0x02, 0x04, 0x31);
	xQueueReceive(CanMsgQueue, &RxMsg, 50);
	if (RxMsg != 0xf1020431)
	{
		return FALSE;
	}
	canMsgTx(0xf1, 0x12, 0x04, 0x31);
	xQueueReceive(CanMsgQueue, &RxMsg, 50);

	return TRUE;
}

static bool isCurGood(void)
{
	u32 RxMsg = 0;
	canMsgTx(0xf1, 0x02, 0x04, 0x30);
	xQueueReceive(CanMsgQueue, &RxMsg, 50);
	if (RxMsg != 0xf1020430)
	{
		return FALSE;
	}
	canMsgTx(0xf1, 0x12, 0x04, 0x30);
	xQueueReceive(CanMsgQueue, &RxMsg, 50);
	return TRUE;
}

static bool isOverCharge(void)
{
	return FALSE;
}

static bool isEmmergency(void)
{
	return FALSE;
}

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



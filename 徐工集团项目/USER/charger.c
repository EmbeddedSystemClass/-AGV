#include "charger.h"
#include "led.h"
#include "key.h"

#define _MSG_RCV_DELAY 100
#define setCloseDelay()\
		 CloseDelay = 200
#define clrChargerOver()\
		ChargerOverFlag = FALSE

extern xQueueHandle CanMsgQueue;
static u32 CloseDelay = 0;
static _CHARGER_STATUS ChargerStatus = CLOSE;
static bool ChargerOverFlag = FALSE;
static u16 rcvCanCount = 0;
static bool ChargerGoodStatus = FALSE;

/*��Ҫ����ѭ��*/
//************************************
// FullName:  changerCTRLLoop
// Returns:   extern u8
// Qualifier:��ɳ���������Ҫ����
// Parameter: void
//************************************
extern u8 chargerCTRLLoop(void)
{
 if (isCloseDelay())
	{
		return 0x01;//������ʱ�ر�
	}
	else if (isEmmergency())
	{
		if (checkChangerStatusOpen() == OPEN)
		{
			closeCharger();
		}
		return 0x02;//���¼�ͣ
	}
	else if (!isOnConnect())
	{
		return 0x03;//�޳�������
	}
	else if (isChargerNotGood())
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
			setCharger();
			openCharger();
			return 0x06;//���ڴ򿪳��׮
		}
		else if (!isCurGood())//�����Ѿ��򿪣���ѯ����
		{
			closeCharger();
			return 0x07;//�����쳣�ر�
		}
		else if (isOverCharge())
		{
			closeCharger();//�����ɹر�
			return 0x08;
		}
	}
	return 0x09; //���������
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
	if(ChargerStatus == OPEN)
	{
		return FALSE;
	}
	else if (0 == CloseDelay)
	{
		return FALSE;
	}
	else
	{
		CloseDelay--;
		vTaskDelay(500);
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
 bool isChargerNotGood(void)
{
	//���Ͳ�ѯ����
	//��������ȴ�����֤
	u8 i = 0;
	u32 RxMsg = 0;
	if (ChargerGoodStatus == TRUE)
	{
		return FALSE;
	}
	canMsgTx(_CONNECT_CMD);
	for (i = 0; i < 16; i++)
	{
		if (pdFALSE == xQueueReceive(CanMsgQueue, &RxMsg, 100))
		{
			ChargerGoodStatus = FALSE;
			return TRUE;
		}
	}
	rcvCanCount = 0;
	ChargerGoodStatus = TRUE;
	return FALSE;
}

//************************************
// FullName:  openChanger
// Returns:   void
// Qualifier:�򿪳������ȴ���������
// Parameter: void
//************************************
static void openCharger(void)
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
	clrChargerOver();
	ChargerStatus = OPEN;
}

//************************************
// FunctionName:  closeChanger
// Returns:   void
// Qualifier:�رճ�����������ʱ
// Parameter: void
//************************************
static void closeCharger(void)
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
	ChargerStatus = CLOSE;
	ChargerGoodStatus = FALSE;
}
//************************************
// FunctionName:  setChanger
// Returns:   void
// Qualifier:�����������õ�ǰ�������
// Parameter: void
//************************************
static void setCharger(void)
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
	xQueueReceive(CanMsgQueue, &RxMsg, 80);
	if (RxMsg != _CHECK_BATTRY_CMD_BACK)
	{
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
		if (RxMsg < 0xf1310100)//�趨С��ĳֵʱ������������翪ʼʱ�趨���δ����
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
	return ChargerOverFlag;
}

//************************************
// FunctionName:  isEmmergency
// Returns:   bool
// Qualifier:��⼱ͣ��ť�Ƿ���
// Parameter: void
//************************************
static bool isEmmergency(void)
{
	if (Bit_RESET == GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_13))
	{
		vTaskDelay(5);
		if (Bit_RESET == GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_13))
		{
			return TRUE;
		}
	}
	return FALSE;
}

//************************************
// FunctionName:  checkChangerStatusOpen
// Returns:   _CHANGER_STATUS
// Qualifier:��ѯ��ǰ����״̬
// Parameter: void
//************************************
static _CHARGER_STATUS checkChangerStatusOpen(void)
{
	return ChargerStatus;
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
	RxData = RxMessage.Data[0] << 24 | RxMessage.Data[1] << 16 | RxMessage.Data[2] << 8 | RxMessage.Data[3];
	if (RxMessage.ExtId == 0x181ff502)
	{
		rcvCanCount ++;
		xQueueSendFromISR(CanMsgQueue, &RxData, &xHigherPriorityTaskWoken);
	}
}

//************************************
// FunctionName:  setChangerOver
// Returns:   void
// Qualifier:���øú����趨��������־����ʱ��������С
// Parameter: void
//************************************
static void setChangerOver(void)
{
	ChargerOverFlag = TRUE;
}


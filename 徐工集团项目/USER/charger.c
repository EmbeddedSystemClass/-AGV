#include "charger.h"
#include "led.h"
#include "key.h"
#include "TIM.h"

#define _MSG_RCV_DELAY 100

//#define setCloseDelay()\
//		 CloseDelay = 200
#define clrChargerOver()\
		ChargerOverFlag = FALSE

#define isButtonPress()\
		Bit_RESET == GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_13)

extern xQueueHandle CanMsgQueue;
extern xQueueHandle CloseTimeQueue;
static u32 CloseDelay = 0;
static _CHARGER_STATUS ChargerStatus = CLOSE;
static bool ChargerOverFlag = FALSE;
static u16 rcvCanCount = 0;
static bool ChargerGoodStatus = FALSE;
static u16 RcvCurr = 0x00;
static u16 RcvVola = 0x00;
static bool AgvSetChargerOpen = FALSE;
static bool AgvConnectFlag = FALSE;


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
			while( 0 != closeCharger());
		}
		return 0x02;//���¼�ͣ
	}
	else if (isChargerNotGood())
	{
		return 0x04;//�����쳣(���Ӽ��)
	}

	else if (!isOnConnect())
	{
		if (checkChangerStatusOpen() == OPEN)
		{
			while (0 != closeCharger());
		}
		return 0x03;//�޳�������
	}
	else if(!AgvSetChargerOpen)
	{
		if (checkChangerStatusOpen()== OPEN)
		{
			while (0 != closeCharger());
		}
		return 0x0a;
	}
	else if (!isBattryVolGood())
	{
		return 0x05;//��ص�ѹ�쳣
	}
	else
	{
		if (CLOSE == checkChangerStatusOpen())//����δ�򿪣��򿪳���
		{
			while(0 != setCharger());
			while(0 != openCharger());
			return 0x06;//���ڴ򿪳��׮
		}
		else if (!isCurGood())//�����Ѿ��򿪣���ѯ����
		{
            closeCharger();
			//while (0 != closeCharger());
			return 0x07;//�����쳣�ر�
		}
		else if (isOverCharge())
		{
			while (0 != closeCharger());//�����ɹر�
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
	return AgvConnectFlag;

	//if (Bit_RESET == GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_8))
	//{
	//	return TRUE;
	//}
	//else
	//{
	//	return FALSE;
	//}
}

//************************************
// FullName:  isCloseDelay
// Returns:   bool
// Qualifier:�ж��Ƿ����ڹػ���ʱ
// Parameter: void
//************************************
static bool isCloseDelay(void)
{
	if (ChargerStatus == OPEN)
	{
		return FALSE;
	}
	else if (0 == ChargerCloseCount)
	{
		STOP_CLOSE;
		return FALSE;
	}
	else
	{
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
	canMsgTx(0,_CONNECT_CMD);
	while(pdTRUE == xQueueReceive(CanMsgQueue, &RxMsg, 100))
	{
		i++;
	}
	if(i == 0)
	{
		ChargerGoodStatus = FALSE;
		return TRUE;
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
static s8 openCharger(void)
{
	u32 RxMsg = 0;
	canMsgTx(0,_OPEN_CMD);
	xQueueReceive(CanMsgQueue, &RxMsg, 50);
	if (RxMsg != _OPEN_CMD_BACK)
	{
		return -1;
	}
	vTaskDelay(10);
	canMsgTx(0,_OPEN_DATA);
	xQueueReceive(CanMsgQueue, &RxMsg, 50);
	if (RxMsg != _OPEN_DATA_BACK)
	{
		return -2;
	}
	clrChargerOver();
	ChargerStatus = OPEN;
	START_TIME;
	return 0;
}

//************************************
// FunctionName:  closeChanger
// Returns:   void
// Qualifier:�رճ�����������ʱ
// Parameter: void
//************************************
static s8 closeCharger(void)
{
	u32 RxMsg = 0;
	canMsgTx(0,_CLOSE_CMD);
	xQueueReceive(CanMsgQueue, &RxMsg, 50);
	if (RxMsg != _CLODE_CMD_BACK)
	{
		return -1;
	}
	vTaskDelay(10);
	canMsgTx(0,_CLOSE_DATA);
	xQueueReceive(CanMsgQueue, &RxMsg, 50);
	if (RxMsg != _CLOSE_DATA_BACK)
	{
		return -2;
	}
//	setCloseDelay();
	ChargerStatus = CLOSE;
	STOP_TIME;
	START_CLOSE;
	agvOpenResetCmd();
	return 0;
}
//************************************
// FunctionName:  setChanger
// Returns:   void
// Qualifier:�����������õ�ǰ�������
// Parameter: void
//************************************
static s8 setCharger(void)
{
	//������������ȴ�����
	u32 RxMsg = 0;
	canMsgTx(0,_SET_VOLATE_CMD);
	xQueueReceive(CanMsgQueue, &RxMsg, 50);
	if (RxMsg != _SET_VOLATE_CMD_BACK)
	{
		return -1;
	}
	vTaskDelay(10);
	canMsgTx(0,_SET_VOLATE_DATA);
	xQueueReceive(CanMsgQueue, &RxMsg, 50);
	if (RxMsg != _SET_VOLATE_DATA_BACK)
	{
		return -2;
	}
	vTaskDelay(10);
	canMsgTx(0,_SET_CURR_CMD);
	xQueueReceive(CanMsgQueue, &RxMsg, 50);
	if (RxMsg != _SET_CURR_CMD_BACK)
	{
		return -3;
	}
	vTaskDelay(10);
	canMsgTx(0,_SET_CURR_DATA);
	xQueueReceive(CanMsgQueue, &RxMsg, 50);
	if (RxMsg != _SET_CURR_DATA_BACK)
	{
		return -4;
	}
	return 0;
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
	canMsgTx(0,_CHECK_BATTRY_CMD);
	xQueueReceive(CanMsgQueue, &RxMsg, 80);
	if (RxMsg != _CHECK_BATTRY_CMD_BACK)
	{
		return FALSE;
	}
	vTaskDelay(10);
	canMsgTx(0,_CHECK_BATTY_DATA);
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
	canMsgTx(0,_CHECK_CURR_CMD);
	xQueueReceive(CanMsgQueue, &RxMsg, 50);
	if (RxMsg != _CHECK_CURR_CMD_BACK)
	{
		return FALSE;
	}
	vTaskDelay(10);
	canMsgTx(0,_CHECK_CURR_DATA);
	if (pdFALSE == xQueueReceive(CanMsgQueue, &RxMsg, 50))
	{
		return FALSE;
	}
	else
	{
		RcvCurr = (RxMsg & 0xffff);
		if (ChargerTimeCount < 60)
		{
			return TRUE;
		}
		if (RxMsg < 0xf13100E0)//�趨С��ĳֵʱ������������翪ʼʱ�趨���δ����
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
	if (isButtonPress())
	{
		vTaskDelay(5);
		if (isButtonPress())
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
	if (RxMessage.ExtId == 0x181ff502 || RxMessage.ExtId == 0x181ff512 || RxMessage.ExtId == 0x181ff522)
	{
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

extern u16 getCurr(void)
{
	return RcvCurr;
}

extern void agvOpenSetCmd(void)
{
	AgvSetChargerOpen = TRUE;
}
extern void agvOpenResetCmd(void)
{
	AgvSetChargerOpen = FALSE;
}

extern void agvConnectSetCmd(void)
{
	AgvConnectFlag = TRUE;
}
extern void agvConnectResetCmd(void)
{
	AgvConnectFlag = FALSE;
}


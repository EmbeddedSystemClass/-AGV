#include "charger.h"
#include "led.h"
#include "key.h"
#include "TIM.h"

#define _CLOSE_CUR 0x100ul//����������

#define _MSG_RCV_DELAY 100

#define clrChargerOver()\
		ChargerOverFlag = FALSE

#define isButtonPress()\
		Bit_RESET == GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_13)

extern xQueueHandle CanMsgQueue;
extern xQueueHandle CloseTimeQueue;
static _CHARGER_STATUS_TYPE ChargerStatus = CLOSE;
static bool ChargerOverFlag = FALSE;
static bool ChargerGoodStatus = FALSE;
static CHARGER_MOUDLE_TYPE MoudleGoodFlag = {FALSE, FALSE };
static CHARGER_MOUDLE_TYPE MoudleOpenFlag = {FALSE, FALSE };
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
	u8 controlBack = 0;
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
		controlBack = controlStrtagy();
		if (controlBack)
		{
			return controlBack;
		}
	}
	return 0x09; //���������
}

static s8 controlStrtagy(void)
{
	if (CLOSE == checkChangerStatusOpen())//����δ�򿪣��򿪳���
	{
		while (0 != setCharger());
		while (0 != openCharger());
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
	else if ((ChargerTimeCount >= 600) && (ChargerTimeCount % 600 == 0))
	{
		if (MoudleOpenFlag.module0 == TRUE)
		{
			closeMoudle(module0);
			MoudleOpenFlag.module0 = FALSE;
			openMdoule(module2);
			MoudleOpenFlag.module2 = TRUE;
		}
		else if (MoudleOpenFlag.module2 == TRUE)
		{
			closeMoudle(module2);
			MoudleOpenFlag.module2 = FALSE;
			openMdoule(module0);
			MoudleOpenFlag.module0 = TRUE;
		}
	}
	return 0x00;
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

static bool isModuleConnect(_CHANGER_MODULE moudle)
{
	u8 i = 0;
    u32 RxMsg = 0;
	sendCmdToCharger(moudle, _CONNECT_CMD);
	while (pdTRUE == xQueueReceive(CanMsgQueue, &RxMsg, 100))
	{
		i++;
	}
	if (i == 0)
	{
		return FALSE;
	}
	else
	{
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
	if (ChargerGoodStatus == TRUE)
	{
		return FALSE;
	}

	if (isModuleConnect(module0))//�ж�ģ��1�Ƿ���������
	{
		MoudleGoodFlag.module0 = TRUE;
	}
	if (isModuleConnect(module2))//�ж�ģ��2�Ƿ���������
	{
		MoudleGoodFlag.module2 = TRUE;
	}
	if ((MoudleGoodFlag.module0 | MoudleGoodFlag.module2) == FALSE)
	{
		ChargerGoodStatus = FALSE;
		return TRUE;
	}
	ChargerGoodStatus = TRUE;
	return FALSE;
}

static s8 openMdoule(_CHANGER_MODULE module)
{
	u32 RxMsg = 0;
	canMsgTx(module, _OPEN_CMD);
	xQueueReceive(CanMsgQueue, &RxMsg, 50);
	if (RxMsg != _OPEN_CMD_BACK)
	{
		return -1;
	}
	vTaskDelay(10);
	canMsgTx(module, _OPEN_DATA);
	xQueueReceive(CanMsgQueue, &RxMsg, 50);
	if (RxMsg != _OPEN_DATA_BACK)
	{
		return -2;
	}
    return 0;
}

//************************************
// FullName:  openChanger
// Returns:   void
// Qualifier:�򿪳������ȴ���������
// Parameter: void
//************************************
static s8 openCharger(void)
{
	if (MoudleGoodFlag.module0 == TRUE)
	{
		openMdoule(module0);
		MoudleOpenFlag.module0 = TRUE;
	}
	if (MoudleGoodFlag.module2 == TRUE)
	{
		openMdoule(module2);
		MoudleOpenFlag.module2 = TRUE;
	}

	clrChargerOver();
	ChargerStatus = OPEN;
	START_TIME;
	return 0;
}

static s8 closeMoudle(_CHANGER_MODULE module)
{
	u32 RxMsg = 0;
	canMsgTx(module, _CLOSE_CMD);
	xQueueReceive(CanMsgQueue, &RxMsg, 50);
	if (RxMsg != _CLODE_CMD_BACK)
	{
		return -1;
	}
	vTaskDelay(10);
	canMsgTx(module, _CLOSE_DATA);
	xQueueReceive(CanMsgQueue, &RxMsg, 50);
	if (RxMsg != _CLOSE_DATA_BACK)
	{
		return -2;
	}
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
	if (MoudleOpenFlag.module0 == TRUE)
	{
		if (0 == closeMoudle(module0))
		{
			MoudleOpenFlag.module0 = FALSE;
		}
	}
	if (MoudleOpenFlag.module2 == TRUE)
	{
		if (0 == closeMoudle(module2))
		{
			MoudleOpenFlag.module2 = FALSE;
		}
		MoudleOpenFlag.module2 = FALSE;
	}
	if ((MoudleOpenFlag.module0 | MoudleOpenFlag.module2) == FALSE)
	{
		ChargerStatus = CLOSE;
	}
    RcvCurr = 0;
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
	canMsgTx(2, _SET_VOLATE_CMD);
	xQueueReceive(CanMsgQueue, &RxMsg, 50);
	if (RxMsg != _SET_VOLATE_CMD_BACK)
	{
		return -1;
	}
	vTaskDelay(10);
	canMsgTx(2, _SET_VOLATE_DATA);
	xQueueReceive(CanMsgQueue, &RxMsg, 50);
	if (RxMsg != _SET_VOLATE_DATA_BACK)
	{
		return -2;
	}
	vTaskDelay(10);
	canMsgTx(2, _SET_CURR_CMD);
	xQueueReceive(CanMsgQueue, &RxMsg, 50);
	if (RxMsg != _SET_CURR_CMD_BACK)
	{
		return -3;
	}
	vTaskDelay(10);
	canMsgTx(2, _SET_CURR_DATA);
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
	u32 Cur0 = 0;
	u32 Cur2 = 0;
	canMsgTx(module0,_CHECK_CURR_CMD);
	xQueueReceive(CanMsgQueue, &RxMsg, 50);
	if (RxMsg != _CHECK_CURR_CMD_BACK)
	{
		return FALSE;
	}
	vTaskDelay(10);
	canMsgTx(module0,_CHECK_CURR_DATA);
	if (pdFALSE == xQueueReceive(CanMsgQueue, &Cur0, 50))
	{
		return FALSE;
	}

	canMsgTx(module2, _CHECK_CURR_CMD);
	xQueueReceive(CanMsgQueue, &RxMsg, 50);
	if (RxMsg != _CHECK_CURR_CMD_BACK)
	{
		return FALSE;
	}
	vTaskDelay(10);
	canMsgTx(module2, _CHECK_CURR_DATA);
	if (pdFALSE == xQueueReceive(CanMsgQueue, &Cur2, 50))
	{
		return FALSE;
	}
	RcvCurr = (Cur0 & 0xffff) + (Cur2 & 0xffff);
	if (ChargerTimeCount < 60)
	{
		return TRUE;
	}
	if (RcvCurr < _CLOSE_CUR)//�趨С��ĳֵʱ������������翪ʼʱ�趨���δ����
	{
		setChangerOver();
	}
	return TRUE;
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
static _CHARGER_STATUS_TYPE checkChangerStatusOpen(void)
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


static void sendCmdToCharger(u8 moudle, u8 data1, u8 data2, u8 data3, u8 data4)
{
	switch (moudle)
	{
	case 0:
		canMsgTx(0, data1, data2, data3, data4);
		break;
	case 2:
		canMsgTx(2, data1, data2, data3, data4);
		break;
	case 3:
		canMsgTx(0, data1, data2, data3, data4);
		vTaskDelay(10);
		canMsgTx(2, data1, data2, data3, data4);
		break;
	default:
		break;
	}
}



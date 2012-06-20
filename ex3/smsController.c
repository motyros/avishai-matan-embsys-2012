#include "TX/tx_api.h"
#include "input_panel/input_panel.h"
#include "network/network.h"
#include "smsController.h"
#include "smsView.h"
#include "smsModel.h"
#include "timer/timer.h"
#include "time.h"
#include "common_defs.h"

#define DEVICE_ID "06664120"
#define KEY_PAD_TIMER_DURATION (150/TX_TICK_MS)
#define NETWORK_SLEEP_PERIOD (100/TX_TICK_MS)
#define NETWORK_PERIODIC_SEND_DURATION (100/TX_TICK_MS)
#define KEY_PAD_THREAD_STACK_SIZE (1024)
#define NETWORK_SEND_THREAD_STACK_SIZE (1024)
#define NETWORK_RECEIVE_THREAD_STACK_SIZE (1024)
#define NETWORK_SEND_THREAD_STACK_SIZE (1024)

#define KEY_PAD_PRIORITY (1)
#define NETWORK_PRIORITY (1)
#define ARRAY_CHAR_LEN(a) (sizeof(a)/sizeof(CHAR))

typedef enum
{
	SEND_PROBE = 1,
	SEND_PROBE_ACK = 2,
} NetworkSendFlags;

//**** FWD Declaration ****\\

void handleNumberScreen(button but);
void handleEditScreen(button but);
void controllerPacketArrived();

//**************Threads global variables****************//
TX_THREAD gKeyPadThread;
CHAR gKeyPadThreadStack[KEY_PAD_THREAD_STACK_SIZE];
TX_EVENT_FLAGS_GROUP gKeyPadEventFlags;

TX_THREAD gNetworkReceiveThread;
CHAR gNetworkReceiveThreadStack[NETWORK_RECEIVE_THREAD_STACK_SIZE];
TX_EVENT_FLAGS_GROUP gNetworkReceiveEventFlags;

TX_THREAD gNetworkSendThread;
CHAR gNetworkSendThreadStack[NETWORK_SEND_THREAD_STACK_SIZE];
TX_EVENT_FLAGS_GROUP gNetworkSendEventFlags;
//******************************************************//

TX_MUTEX gProbeAckMutex;

bool gNeedToAckDeliverCounter = false;
SMS_PROBE gSmsProbe;
TX_TIMER gPeriodicSendTimer;


const CHAR gButton1[] = {'.',',','?','1'};
const CHAR gButton2[] = {'a','b','c','2'};
const CHAR gButton3[] = {'d','e','f','3'};
const CHAR gButton4[] = {'g','h','i','4'};
const CHAR gButton5[] = {'j','k','l','5'};
const CHAR gButton6[] = {'m','n','o','6'};
const CHAR gButton7[] = {'p','q','r','s','7'};
const CHAR gButton8[] = {'t','u','v','8'};
const CHAR gButton9[] = {'w','x','y','z','9'};
const CHAR gButton0[] = {' ','0'};
int gInEditContinuosNum;
int gInEditRecipientIdLen;

TX_TIMER gContinuousButtonPressTimer;
//the button that latest pressed
button gPressedButton;

//UINT gTimerExpNumber = 0;

#define PROBE_MESSAGE_SIZE 8
#define PROBE_MESSAGE_MAX_SIZE 22

TX_MUTEX gProbeAckMutex;


SMS_PROBE gSmsProbe;

void twoDigitIntToStr(UINT n,char* str)
{
	*str++ = (n/10) + '0';
	*str = (n % 10) + '0';
}
void fillTimeStampBuffer(CHAR* buf)
{
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);

	twoDigitIntToStr((tm.tm_year + 1900) % 100,buf++);
	twoDigitIntToStr(tm.tm_mon + 1,buf++);
	twoDigitIntToStr(tm.tm_mday,buf++);
	twoDigitIntToStr(tm.tm_hour,buf++);
	twoDigitIntToStr(tm.tm_min,buf++);
	twoDigitIntToStr(tm.tm_sec,buf++);
	//since we are GMT+2, and the units are 15 minutes, we need 4*2 units
	twoDigitIntToStr(4*2,buf);
}

char gProbeBuf[PROBE_MESSAGE_MAX_SIZE];

/*
 *   this the periodic probe timer callback
 */
void networkSendPeriodicProbe(ULONG v)
{

	//get the lock on the var which indicates if we need to ack or just probe
	//signal the thread to send probe
	tx_event_flags_set(&gNetworkSendEventFlags,SEND_PROBE,TX_OR);
}


void networkSendThreadMainFunc(ULONG v)
{
	ULONG actualFlag;

	while(true)
	{
		//wait for a flag wake the thread up
		tx_event_flags_get(&gNetworkSendEventFlags,SEND_PROBE|SEND_PROBE_ACK,TX_OR_CLEAR,&actualFlag,TX_WAIT_FOREVER);

		UINT bufLen = 0;

		embsys_fill_probe(gProbeBuf,&gSmsProbe,actualFlag & SEND_PROBE_ACK,&bufLen);

		network_send_packet_start((uint8_t*)gProbeBuf,bufLen,bufLen);
	}
}

//buffer for a packet that latest received
uint8_t gPacketReceivedBuffer[NETWORK_MAXIMUM_TRANSMISSION_UNIT];



void buttonPressedCB(button b)
{
	gPressedButton = b;
	//wake up the keyPad thread
	tx_event_flags_set(&gKeyPadEventFlags,1,TX_OR);
}

/*
 * call back when a packet was received.
 */
void networkReceiveCB(uint8_t buffer[], uint32_t size, uint32_t length)
{

	DBG_ASSERT(size>=length);
	DBG_ASSERT(length<=NETWORK_MAXIMUM_TRANSMISSION_UNIT);

	tx_timer_deactivate(&gPeriodicSendTimer);

	//copy the packet to the global
	memcpy(gPacketReceivedBuffer,buffer,length);

	//wake up the networkReceive thread
	tx_event_flags_set(&gNetworkReceiveEventFlags,1,TX_OR);
}

void keyPadThreadMainFunc(ULONG v)
{
	ULONG actualFlag;

	while(true)
	{
		//wait for a flag wake the thread up
		tx_event_flags_get(&gKeyPadEventFlags,1,TX_AND_CLEAR,&actualFlag,TX_WAIT_FOREVER);

		//handle the button that pressed
		controllerButtonPressed(gPressedButton);
	}
}

void networkReceiveThreadMainFun(ULONG v)
{
	ULONG actualFlag;
	while(true)
	{
		//wait for a flag wake the thread up
		tx_event_flags_get(&gNetworkReceiveEventFlags,1,TX_AND_CLEAR,&actualFlag,TX_WAIT_FOREVER);

		//handle the packet that arrived
		controllerPacketArrived();

		tx_timer_activate(&gPeriodicSendTimer);
	}
}
void disableContinuousButtonPress(ULONG v)
{
	modelSetIsContinuousButtonPress(false);
}

/*
 * call back when transmission done
 */
void txDone(const uint8_t *buffer, uint32_t size)
{

}


/*
 * call back when a packet was dropped during receiving.
 */
void rxPacketDrop(packet_dropped_reason_t)
{

}

/*
 * call back when a packet was dropped during transmission.
 */
void txPacketDrop(transmit_error_reason_t,
                                           uint8_t *buffer,
                                           uint32_t size,
                                           uint32_t length )
{

}

#define NET_BUF_SIZE 4

desc_t gTranBuf[NET_BUF_SIZE];
desc_t gRecBuf[NET_BUF_SIZE];
uint8_t gRecData[NET_BUF_SIZE*NETWORK_MAXIMUM_TRANSMISSION_UNIT];

result_t networkInit()
{
	result_t result;
	int i;

	/*
	 * init network: init a full struct for the transmit_buffer and Receive_buffer,
	 * 				call backs for the network: transmitted_cb, received_cb, dropped_cb, error_cb
	 *
	 */
	network_call_back_t callBacks;
	network_init_params_t initParams;

	callBacks.dropped_cb = rxPacketDrop;
	callBacks.recieved_cb = networkReceiveCB;
	callBacks.transmit_error_cb = txPacketDrop;
	callBacks.transmitted_cb = txDone;

	initParams.list_call_backs = callBacks;

    for(i = 0 ; i < NET_BUF_SIZE ; ++i)
    {
    	gRecBuf[i].pBuffer = (uint32_t)(gRecData+i*NETWORK_MAXIMUM_TRANSMISSION_UNIT);
    	gRecBuf[i].buff_size = NETWORK_MAXIMUM_TRANSMISSION_UNIT;
    }

    initParams.transmit_buffer = gTranBuf;
    initParams.size_t_buffer = NET_BUF_SIZE;
    initParams.recieve_buffer = gRecBuf;
    initParams.size_r_buffer = NET_BUF_SIZE;

    network_set_operating_mode(NETWORK_OPERATING_MODE_SMSC);
	return network_init(&initParams);
}

TX_STATUS controllerInit()
{
	gInEditRecipientIdLen = 0;
	gInEditContinuosNum = 0;
	/* create three threads
	 *
	 * 1. will listed to the keypad
	 * 2. will listed to the network: received_cb
	 * 3. will send data to the network: transmitted_cb, error_cb, dropped_cb
	 */

	TX_STATUS status;


	/***********create the timers****************/
	status = tx_timer_create(	&gContinuousButtonPressTimer,
								"Continuous Button Press Timer",
								disableContinuousButtonPress,
								0,
								KEY_PAD_TIMER_DURATION,
								0,
								TX_NO_ACTIVATE);
	status = tx_timer_create(	&gPeriodicSendTimer,
								"Periodic send timer",
								networkSendPeriodicProbe,
								0,
								NETWORK_PERIODIC_SEND_DURATION*5,
								NETWORK_PERIODIC_SEND_DURATION,
								TX_AUTO_ACTIVATE);


	/***********create the threads****************/
	status = tx_thread_create(	&gKeyPadThread,
								"Keypad Thread",
								keyPadThreadMainFunc,
								0,
								gKeyPadThreadStack,
								KEY_PAD_THREAD_STACK_SIZE,
								KEY_PAD_PRIORITY,
								KEY_PAD_PRIORITY,
								TX_NO_TIME_SLICE, TX_AUTO_START
								);

	status = tx_thread_create(	&gNetworkReceiveThread,
								"Network Receive Thread",
								networkReceiveThreadMainFun,
								0,
								gNetworkReceiveThreadStack,
								NETWORK_RECEIVE_THREAD_STACK_SIZE,
								NETWORK_PRIORITY,
								NETWORK_PRIORITY,
								TX_NO_TIME_SLICE, TX_AUTO_START
								);

	status = tx_thread_create(	&gNetworkSendThread,
								"Network Send Thread",
								networkSendThreadMainFunc,
								0,
								gNetworkSendThreadStack,
								NETWORK_SEND_THREAD_STACK_SIZE,
								NETWORK_PRIORITY,
								NETWORK_PRIORITY,
								TX_NO_TIME_SLICE, TX_AUTO_START
								);

	/***********create the event flags****************/
	status = tx_event_flags_create(&gKeyPadEventFlags,"Keypad Event Flags");

	status = tx_event_flags_create(&gNetworkReceiveEventFlags,"Network Rec Event Flags");

	status = tx_event_flags_create(&gNetworkSendEventFlags,"Network Send Event Flags");

	/***********create the mutex****************/
	status = tx_mutex_create(&gProbeAckMutex,"Probe Ack Mutex",TX_INHERIT);

	memcpy(gSmsProbe.device_id,DEVICE_ID,strlen(DEVICE_ID));






	//TODO handle status != TX_SUCCESS

	result_t result;
	//init the key pad
	result = ip_init(buttonPressedCB);

	if(result != OPERATION_SUCCESS)
	{
		return result;
	}

	ip_enable();


	/* timer - for periodically send PROBE or SMS_SUBMIT
	 * timer - for turning off continues button press after x ms
	 *
	 */

	/*
	 *
	 *
	 *
	 */
	modelAcquireLock();

	modelSetCurrentScreenType(MESSAGE_LISTING_SCREEN);
	//update the first sms on screen
	modelSetFirstSmsOnScreen(modelGetFirstSms());
	//update the selected sms
	modelSetSelectedSms(modelGetFirstSms());

	modelReleaseLock();

	networkInit();

	return TX_SUCCESS;
}

void secureSetFirstSmsOnScreen(const SmsLinkNodePtr pSms)
{
	modelAcquireLock();
	modelSetFirstSmsOnScreen(pSms);
	modelReleaseLock();
}

void deleteSmsFromScreen(const SmsLinkNodePtr pFirstSms,const SmsLinkNodePtr pSelectedSms)
{
	//we are deleting the first sms in the screen
	if (viewIsFirstRowSelected())
	{
		//if this is the only sms
		if (modelGetSmsDbSize() == 1)
		{
			modelSetFirstSmsOnScreen(NULL);
			modelSetSelectedSms(NULL);
		}
		else
		{
			modelSetFirstSmsOnScreen(pFirstSms->pNext);
		}
	}

	//do it only if after the delete db is not empty
	if (modelGetSmsDbSize() > 1)
	{
		modelSetSelectedSms(pSelectedSms->pNext);
	}
//	else
//	{
//		modelSetFirstSmsOnScreen(pFirstSms->pNext);
//	}
}

void deleteSms(const SmsLinkNodePtr pFirstSms,const SmsLinkNodePtr pSelectedSms)
{
	deleteSmsFromScreen(pFirstSms,pSelectedSms);
	modelRemoveSmsFromDb(pSelectedSms);
}

void handleListingScreen(button b)
{
	//modelAcquireLock();

	SmsLinkNodePtr pSelectedSms = modelGetSelectedSms();
	SmsLinkNodePtr pFirstSms = modelGetFirstSmsOnScreen();

	bool refreshView = false;

	switch (b)
	{
	case BUTTON_2: //up

		//there are no messages to display
		if (pSelectedSms == NULL)
		{
			break;
		}

		/* if first row is selected and we are moving up:
		 * 1. if we have more sms than the screen height, first line remain selected
		 *    and the first sms will be the predecessor
		 * 2. if all sms fit the screen, select the last sms
		 */
		if (viewIsFirstRowSelected())
		{
			if (modelGetSmsDbSize() > viewGetScreenHeight(MESSAGE_LISTING_SCREEN))
			{
				modelSetFirstSmsOnScreen(pFirstSms->pPrev);
			}
			viewSetRefreshScreen();
		}
		modelSetSelectedSms(pSelectedSms->pPrev);
		refreshView = true;
		break;
	case BUTTON_8: //down
		//there are no message to display
		if (pSelectedSms == NULL)
		{
			break;
		}
		if (viewIsLastRowSelected())
		{
			if (modelGetSmsDbSize() > viewGetScreenHeight(MESSAGE_LISTING_SCREEN))
			{
				modelSetFirstSmsOnScreen(pFirstSms->pNext);
			}
			viewSetRefreshScreen();
		}
		modelSetSelectedSms(pSelectedSms->pNext);
		refreshView = true;
		break;

	case BUTTON_STAR: //enter new message screen
		//TODO erase all fields of inEditSms
		//if we don't have room for new sms, don't let write new one
		if (modelGetSmsDbSize() == MAX_NUM_SMS)
		{
			break;
		}
		modelSetCurrentScreenType(MESSAGE_EDIT_SCREEN);
		viewSetRefreshScreen();
		refreshView = true;
		break;
	case BUTTON_NUMBER_SIGN: //delete message
		if (pSelectedSms == NULL) //no message to delete
		{
			break;
		}
		deleteSms(pFirstSms,pSelectedSms);
		viewSetRefreshScreen();
		refreshView = true;
		break;
	case BUTTON_OK: //display message
		modelSetCurrentScreenType(MESSAGE_DISPLAY_SCREEN);
		viewSetRefreshScreen();
		refreshView = true;
		break;
	}

	//modelReleaseLock();
	//signal the view to refresh itself
	if (refreshView)
	{
		viewSignal();
	}

}

void handleDisplayScreen(button b)
{
	bool needToRefershGui = false;
	switch (b)
	{
	case BUTTON_STAR: //go back to sms listing screen
		modelSetCurrentScreenType(MESSAGE_LISTING_SCREEN);
		needToRefershGui = true;
		break;
	case BUTTON_NUMBER_SIGN: //delete current sms
		modelSetCurrentScreenType(MESSAGE_LISTING_SCREEN);
		SmsLinkNodePtr pSelectedSms = modelGetSelectedSms();
		SmsLinkNodePtr pFirstSms = modelGetFirstSmsOnScreen();

		deleteSms(pFirstSms,pSelectedSms);
		//deleteSmsFromScreen(pFirstSms,pSelectedSms);
		//modelRemoveSmsFromDb(pSelectedSms);
		needToRefershGui = true;
		break;
	}

	if (needToRefershGui)
	{
		viewSetRefreshScreen();
		viewSignal();
	}

}
//TODO delete this
TX_STATUS gDbgStatus;

//decide what to do according the cuurent screen, current state etc
void controllerButtonPressed(const button b)
{

	if (modelAcquireLock() != TX_SUCCESS)
	{
		return;
	}
	switch (modelGetCurentScreenType())
	{
	case MESSAGE_DISPLAY_SCREEN:
		handleDisplayScreen(b);
		break;
	case MESSAGE_EDIT_SCREEN:
		handleEditScreen(b);
		break;
	case MESSAGE_LISTING_SCREEN:
		handleListingScreen(b);
		break;
	case MESSAGE_NUMBER_SCREEN:
		handleNumberScreen(b);
		break;
	}

//	//secured set the buttons fields
//	modelAcquireLock();
	modelSetLastButton(b);
	modelSetIsContinuousButtonPress(true);
//	modelReleaseLock();

	gDbgStatus = tx_timer_deactivate(&gContinuousButtonPressTimer);
	//TODO
	gDbgStatus++;
	gDbgStatus = tx_timer_change(&gContinuousButtonPressTimer,KEY_PAD_TIMER_DURATION,0);
	gDbgStatus++;
	gDbgStatus = tx_timer_activate(&gContinuousButtonPressTimer);
	gDbgStatus++;

	modelReleaseLock();
}

CHAR getNumberFromButton(button but)
{
	int counter = 0;
	while (but != 0)
	{
		but = but >> 1;
		counter++;
	}

	return '0' + (counter-1)%11;

//	switch (but)
//	{
//	case BUTTON_1:
//		retun;
//		break;
//	case BUTTON_2:
//		handleEditNewChar(gButton2);
//		break;
//	case BUTTON_3:
//		handleEditNewChar(gButton3);
//		break;
//	case BUTTON_4:
//		handleEditNewChar(gButton4);
//		break;
//	case BUTTON_5:
//		handleEditNewChar(gButton5);
//		break;
//	case BUTTON_6:
//		handleEditNewChar(gButton6);
//		break;
//	case BUTTON_7:
//		handleEditNewChar(gButton7);
//		break;
//	case BUTTON_8:
//		handleEditNewChar(gButton8);
//		break;
//	case BUTTON_9:
//		handleEditNewChar(gButton9);
//		break;
//	case BUTTON_0:
//		handleEditNewChar(gButton0);
//		break;
//	}

}

/*
 *
 */
void updateModelFields()
{
//	modelAcquireLock();
	//if it is the first sms, update the relevant fields
	if (modelGetSmsDbSize() == 1)
	{
		modelSetFirstSmsOnScreen(modelGetFirstSms());
		modelSetSelectedSms(modelGetFirstSms());
	}
//	modelReleaseLock();
}

char gSendBuf[NETWORK_MAXIMUM_TRANSMISSION_UNIT];

/*
 * send the edited sms
 */
void sendEditSms()
{
	SMS_SUBMIT* smsToSend = modelGetInEditSms();
	unsigned dataLen;
	memset(smsToSend->device_id,0,ID_MAX_LENGTH);
	memcpy(smsToSend->device_id,DEVICE_ID,strlen(DEVICE_ID));
	//TODO make sure device_id and rec id will be zeroed before working with them
	embsys_fill_submit(gSendBuf,smsToSend,&dataLen);

	network_send_packet_start((uint8_t*)gSendBuf,dataLen,dataLen);

	//add the sms to the linked list
	modelAddSmsToDb(smsToSend, OUTGOING_MESSAGE);

	//update the relevant fields
	updateModelFields();
}

//void handleNewNumber(char num)
//{
//	if(gInEditContinuosNum >= ID_MAX_LENGTH)
//	{
//		inEditSms->recipient_id[gInEditRecipientIdLen-1] = num;
//	}
//	else
//	{
//		inEditSms->recipient_id[gInEditRecipientIdLen] = num;
//	}
//}

void handleNumberScreen(button but)
{
	SMS_SUBMIT* inEditSms = modelGetInEditSms();

	switch (but)
	{
	case BUTTON_OK:
		sendEditSms();
		modelSetCurrentScreenType(MESSAGE_LISTING_SCREEN);
		viewSetRefreshScreen();
		viewSignal();
		return;
	case BUTTON_STAR:
		//cancel the message
		inEditSms->data_length = 0;
		//update the gui
		modelSetCurrentScreenType(MESSAGE_LISTING_SCREEN);
		viewSetRefreshScreen();
		viewSignal();
		return;
	case BUTTON_NUMBER_SIGN:
		if (gInEditRecipientIdLen == 0)
		{
			return;
		}

		gInEditRecipientIdLen--;
		inEditSms->recipient_id[gInEditRecipientIdLen] = ' ';
		viewSignal();
		return;

		//TODO
//	case BUTTON_1:
//		handleNewNumber('1');
//		break;
//	case BUTTON_2:
//		handleEditNewChar(gButton2);
//		break;
//	case BUTTON_3:
//		handleEditNewChar(gButton3);
//		break;
//	case BUTTON_4:
//		handleEditNewChar(gButton4);
//		break;
//	case BUTTON_5:
//		handleEditNewChar(gButton5);
//		break;
//	case BUTTON_6:
//		handleEditNewChar(gButton6);
//		break;
//	case BUTTON_7:
//		handleEditNewChar(gButton7);
//		break;
//	case BUTTON_8:
//		handleEditNewChar(gButton8);
//		break;
//	case BUTTON_9:
//		handleEditNewChar(gButton9);
//		break;
//	case BUTTON_0:
//		handleEditNewChar(gButton0);
//		break;
	}

	if(gInEditRecipientIdLen == ID_MAX_LENGTH)
	{
		inEditSms->recipient_id[gInEditRecipientIdLen-1] = getNumberFromButton(but);
	}
	else
	{
		inEditSms->recipient_id[gInEditRecipientIdLen] = getNumberFromButton(but);
		gInEditRecipientIdLen++;
	}

	//update the screen
	viewSignal();

}

void handleEditNewChar(const char buttonX[])
{
	SMS_SUBMIT* inEditSms = modelGetInEditSms();
	if(gInEditContinuosNum > 0 || inEditSms->data_length == DATA_MAX_LENGTH)
	{
		int numChar = gInEditContinuosNum % ARRAY_CHAR_LEN(buttonX);
		inEditSms->data[inEditSms->data_length-1] = buttonX[numChar];
	}
	else
	{
		inEditSms->data[inEditSms->data_length] = buttonX[0];
		inEditSms->data_length++;
	}

	//update the screen
	viewSignal();
}

void handleEditScreen(button but)
{
	if(but!=modelGetLastButton() || !modelIsContinuousButtonPress())
	{
		gInEditContinuosNum = 0;
	}
	else
	{
		gInEditContinuosNum++;
	}
	SMS_SUBMIT* inEditSms = modelGetInEditSms();
	switch (but)
	{
	case BUTTON_OK:
		gInEditRecipientIdLen = 0;
		//clean the recipient_id
		memset(inEditSms->recipient_id ,0, ID_MAX_LENGTH);
		modelSetCurrentScreenType(MESSAGE_NUMBER_SCREEN);
		viewSetRefreshScreen();
		viewSignal();
		break;
	case BUTTON_STAR:
		//reset the message
		inEditSms->data_length = 0;
		modelSetCurrentScreenType(MESSAGE_LISTING_SCREEN);
		viewSetRefreshScreen();
		viewSignal();
		break;
	case BUTTON_NUMBER_SIGN:
		if(inEditSms->data_length > 0)
		{
			inEditSms->data_length--;
			viewSignal();
		}
		break;
	case BUTTON_1:
		handleEditNewChar(gButton1);
		break;
	case BUTTON_2:
		handleEditNewChar(gButton2);
		break;
	case BUTTON_3:
		handleEditNewChar(gButton3);
		break;
	case BUTTON_4:
		handleEditNewChar(gButton4);
		break;
	case BUTTON_5:
		handleEditNewChar(gButton5);
		break;
	case BUTTON_6:
		handleEditNewChar(gButton6);
		break;
	case BUTTON_7:
		handleEditNewChar(gButton7);
		break;
	case BUTTON_8:
		handleEditNewChar(gButton8);
		break;
	case BUTTON_9:
		handleEditNewChar(gButton9);
		break;
	case BUTTON_0:
		handleEditNewChar(gButton0);
		break;
	}
}



/*
 * let the controller know that packet has arrived
 */
void controllerPacketArrived()
{
	EMBSYS_STATUS status;

	//try to parse to submit ack;
	SMS_SUBMIT_ACK submitAck;

	do
	{
		status = embsys_parse_submit_ack((char*)gPacketReceivedBuffer,&submitAck);
		if(status == SUCCESS) break;

		//try to parse to deliver sms
		SMS_DELIVER deliverSms;
		status = embsys_parse_deliver((char*)gPacketReceivedBuffer,&deliverSms);
		if(status != SUCCESS) break;

		//in case it is a delivered sms, add it to the DB
		if(modelAcquireLock() != TX_SUCCESS)
		{
			break;
		}

		//add the delivered sms to the liked list
		modelAddSmsToDb(&deliverSms,INCOMMING_MESSAGE);

		//update the relevant fields
		updateModelFields();

		//release the lock
		modelReleaseLock();

		//copy the sender id, for the probe ack
		memcpy(gSmsProbe.sender_id,deliverSms.sender_id,ID_MAX_LENGTH);
		memcpy(gSmsProbe.timestamp,deliverSms.timestamp,TIMESTAMP_MAX_LENGTH);

		//gNeedToAckDeliverCounter = true;

		//signal the send thread to send probe akc
		tx_event_flags_set(&gNetworkSendEventFlags,SEND_PROBE_ACK,TX_OR);

		//in case the screen is on the message listing, refresh the screen
		if(modelGetCurentScreenType() == MESSAGE_LISTING_SCREEN)
		{
			viewSetRefreshScreen();
			viewSignal();
		}
	}while(false);

}


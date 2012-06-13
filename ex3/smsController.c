#include "TX/tx_api.h"
#include "input_panel/input_panel.h"
#include "smsController.h"
#include "smsView.h"
#include "smsModel.h"


#define KEY_PAD_THREAD_STACK_SIZE (1024)
#define KEY_PAD_PRIORITY (1)
#define ARRAY_CHAR_LEN(a) (sizeof(a)/sizeof(CHAR))

TX_THREAD gKeyPadThread;
CHAR gKeyPadThreadStack[KEY_PAD_THREAD_STACK_SIZE];


TX_EVENT_FLAGS_GROUP gKeyPadEventFlags;
button gCurrentButton;

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

void buttonPressedCB(button b)
{
	//wake up the keyPad thread
}

void keyPadThreadMainFunc(ULONG v)
{
	while(true)
	{
		//wait for event flags
	}
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

	ip_init(buttonPressedCB);
	/*
	 * init network: init a full struct for the transmit_buffer and recieve_buffer,
	 * 				call backs for the network: transmitted_cb, received_cb, dropped_cb, error_cb
	 *
	 * init keypad: call back for the keypad
	 * timer - for periodically send PROBE or SMS_SUBMIT
	 * timer - for turning off continues button press after x ms
	 *
	 */

	/*
	 *
	 *
	 *
	 */

	modelSetCurrentScreenType(MESSAGE_LISTING_SCREEN);

	modelSetFirstSmsOnScreen(modelGetFirstSms());
	modelSetSelectedSms(modelGetFirstSms());

	return TX_SUCCESS;
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
		}
		else
		{
			modelSetFirstSmsOnScreen(pFirstSms->pNext);
		}
	}
	else
	{
		modelSetFirstSmsOnScreen(pFirstSms->pNext);
	}
}

void handleListingScreen(button b)
{
	SmsLinkNodePtr pSelectedSms = modelGetSelectedSms();
	SmsLinkNodePtr pFirstSms = modelGetFirstSmsOnScreen();

	switch (b)
	{
	case BUTTON_2: //up

		//there are no messages to display
		if (pFirstSms == NULL)
		{
			return;
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
		break;
	case BUTTON_8: //down
		//there are no message to display
		if (pFirstSms == NULL)
		{
			return;
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
		break;

	case BUTTON_STAR: //enter new message screen
		//if we don't have room for new sms, don't let write new one
		if (modelGetSmsDbSize() == MAX_NUM_SMS)
		{
			break;
		}
		modelSetCurrentScreenType(MESSAGE_EDIT_SCREEN);
		viewSetRefreshScreen();
		break;
	case BUTTON_NUMBER_SIGN: //delete message
		deleteSmsFromScreen(pFirstSms,pSelectedSms);
		modelDeleteSmsFromDb(pSelectedSms);
		viewSetRefreshScreen();
		break;
	case BUTTON_OK: //display message
		modelSetCurrentScreenType(MESSAGE_DISPLAY_SCREEN);
		viewSetRefreshScreen();
		break;
	default: return;
	}

	//signal the view to refresh itself
	viewSignal();

}

void handleDisplayScreen(button b)
{
	switch (b)
	{
	case BUTTON_STAR: //go back to sms listing screen
		modelSetCurrentScreenType(MESSAGE_LISTING_SCREEN);
		break;
	case BUTTON_NUMBER_SIGN: //delete current sms
		SmsLinkNodePtr pSelectedSms = modelGetSelectedSms();
		SmsLinkNodePtr pFirstSms = modelGetFirstSmsOnScreen();

		deleteSmsFromScreen(pFirstSms,pSelectedSms);
		modelDeleteSmsFromDb(pSelectedSms);
		break;
	default: return;
	}

	viewSetRefreshScreen();

}

//decide what to do according the cuurent screen, current state etc
void controllerButtonPressed(const button b)
{
	switch (modelGetCurentScreenType())
	{
	case MESSAGE_DISPLAY_SCREEN:
		handleDisplayScreen(b);
		break;
	case MESSAGE_EDIT_SCREEN:
		//handleEditScreen(b);
		break;
	case MESSAGE_LISTING_SCREEN:
		handleListingScreen(b);
		break;
	case MESSAGE_NUMBER_SCREEN:
		//handleNumberScreen(b);
		break;
	}
	modelSetLastButton(b);
	modelSetIsContinuousButtonPress(true);
	//TODO - create a timer that will set this var to false
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


void handleNumberScreen(button but)
{
	SMS_SUBMIT* inEditSms = modelGetInEditSms();

	switch (but)
	{
	case BUTTON_OK:
		//TODO: send the sms with network
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
		return;
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
		memset(inEditSms->recipient_id ,' ', ID_MAX_LENGTH);
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
		}
		viewSignal();
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


//pass the info
void controllerPacketArrived(const uint8_t* pBuffer,const uint32_t size)
{
}


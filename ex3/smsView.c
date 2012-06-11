#include "smsView.h"
#include "smsModel.h"
#include "TX/tx_api.h"
#include "embsys_sms_protocol.h"

#define SCREEN_HEIGHT 18
#define SCREEN_WIDTH 12
#define BUTTOM

TX_EVENT_FLAGS_GROUP gGuiRefershEventFlags;
TX_EVENT_FLAGS_GROUP gLcdIdleEventFlags;

#define MESSAGE_LISTING_SCREEN_BUTTOM 	"New   Delete"
#define MESSAGE_DISPLAY_SCREEN_BUTTOM 	"Back  Delete"
#define MESSAGE_EDIT_SCREEN_BUTTOM    	"Back  Delete"
#define MESSAGE_NUMBER_SCREEN_BUTTOM	"Back        "
#define BLANK_LINE						"            "

#define INT_TO_CH(n) ((n) + '0')

//we want to refresh the screen at the first time
bool gViewRefreshScreen = true;


UINT init()
{
	UINT status;

	do
	{
		status = tx_event_flags_create(&gGuiEventFlags,"Gui Event Flags");

		if (status != TX_SUCCESS)
		{
			break;
		}

		status = tx_event_flags_create(&gLcdIdleEventFlags,"Lcd Idle Event Flags");

		if (status != TX_SUCCESS)
		{
			break;
		}

		//create GUI thread


	}while(false);

	return status;

}

void deinit()
{
	tx_event_flags_delete(&gGuiEventFlags);
	tx_event_flags_delete(&gLcdIdleEventFlags);

}

void viewSetRefreshScreen()
{
	gViewRefreshScreen = true;
}

int viewGetScreenHeight(const screen_type screen)
{

	switch(screen)
	{
		case MESSAGE_LISTING_SCREEN:
		case MESSAGE_EDIT_SCREEN:
		case MESSAGE_NUMBER_SCREEN:
			return SCREEN_HEIGHT-1;
			break;
		case MESSAGE_DISPLAY_SCREEN:
			return  SCREEN_HEIGHT- 3;
	}
	
	DBG_ASSERT(false);
	return SCREEN_HEIGHT;
}

/*
void viewSetGuiThreadEventsFlag(TX_EVENT_FLAGS_GROUP *pGuiThreadEventsFlag)
{
	DBG_ASSERT(pGuiThreadEventsFlag!=NULL);
	gViewGuiThreadEventsFlag = pGuiThreadEventsFlag;
}
*/

void lcdDone()
{
	tx_event_flags_set(&gLcdIdleEventFlags,1,TX_OR);
}
void viewSignal()
{
	tx_event_flags_set(&gGuiRefershEventFlags,1,TX_OR);
}

void blockingSetLcdLine(uint8_t row_number, bool selected, char const line[], uint8_t length)
{
	ULONG actualFlag;
	lcd_set_row(row_number,selected,line,length);
	tx_event_flags_get(&gLcdIdleEventFlags,1,TX_AND_CLEAR,&actualFlag,TX_WAIT_FOREVER);
}


/*
 *  this function gets an sms message from our DB, serial number and line buffer
 *  and fill the line with the relevant data:
 *  [0-99][number][I/O]
 */
void setMessageListingLineInfo(SmsLinkNodePtr pSms,int serialNumber,CHAR* pLine)
{
	int i = 0;
	int leftDigit = serialNumber/10;
	int rightDigit = serialNumber%10;
	char* p;
	SMS_DELIVER* pInSms;
	SMS_SUBMIT* pOutSms;
	CHAR* pStartLine = pLine;

	//
	if (leftDigit != 0)
	{
		*pLine++ = INT_TO_CH(leftDigit);
	}
	*pLine++ = INT_TO_CH(rightDigit);

	if (pSms->type == INCOMMING_MESSAGE)
	{
		pInSms = (SMS_DELIVER*)pSms->pSMS;
		for(i = 0 ; i < ID_MAX_LENGTH ; ++i)
		{
			*pLine++ = pInSms->sender_id[i];
		}
		*pLine++ = 'I';
	}
	else
	{
		pOutSms = (SMS_SUBMIT*)pSms->pSMS;
		for(i = 0 ; i < ID_MAX_LENGTH ; ++i)
		{
			*pLine++ = pOutSms->device_id[i];
		}
		*pLine++ = 'O'
	}

	i = (int)(pLine-pStartLine);

	while(i++ < SCREEN_WIDTH)
	{
		*pLine++ = ' ';
	}
}
void renderMessageListingScreen()
{
	int i,smsSerialNumber;
	SmsLinkNodePtr pMessage,pSelectedMessage;
	CHAR line[SCREEN_WIDTH];

	pMessage = modelGetFirstSmsOnScreen();
	pSelectedMessage = modelGetSelectedSms();
	smsSerialNumber = modelGetSmsSerialNumber(pMessage);

	if (gViewRefreshScreen)
	{




		for(i = 0 ; i < SCREEN_HEIGHT-1 ; ++i)
		{
			if (!pMessage)
			{
				blockingSetLcdLine(i,false,BLANK_LINE,SCREEN_WIDTH);
			}
			else
			{
				setMessageListingLineInfo(pMessage,smsSerialNumber,&line);
				blockingSetLcdLine(i,(pMessage==pSelectedMessage),line,SCREEN_WIDTH);
				pMesaage = pMessage->pNext;
			}
		}

		blockingSetLcdLine(SCREEN_HEIGHT-1,true,MESSAGE_LISTING_SCREEN,SCREEN_WIDTH);

		gViewRefreshScreen = false;
	}
	else
	{
		i = 0;

		while(pMessage++ != pSelectedMessage)
		{
			i++;
		}

		//i will contain the number of the selected line
		//need to render line above, selected and line below

	}
}

void renderMessageEditScreen()
{

}

void renderMessageNumberScreen()
{

}

void renderMessageDisplayScreen()
{

}

void viewRefresh()
{
	screen_type currentScreen = getCurentScreenType();
	//according the current screen, refresh and current operation refresh the display
	switch(currentScreen)
	{
		case MESSAGE_LISTING_SCREEN:
			renderMessageListingScreen();
			break;
		case MESSAGE_EDIT_SCREEN:
			renderMessageEditScreen();
			break;
		case MESSAGE_NUMBER_SCREEN:
			renderMessageNumberScreen();
			break;
		case MESSAGE_DISPLAY_SCREEN:
			break;

	}
}


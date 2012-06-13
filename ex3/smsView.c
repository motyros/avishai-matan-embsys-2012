#include "smsView.h"
#include "smsModel.h"
#include "TX/tx_api.h"
#include "embsys_sms_protocol.h"
#include <string.h>
#include "LCD/LCD.h"
#include "common_defs.h"

#define SCREEN_HEIGHT 18
#define SCREEN_WIDTH 12
#define BOTTOM

TX_EVENT_FLAGS_GROUP gGuiRefershEventFlags;
TX_EVENT_FLAGS_GROUP gLcdIdleEventFlags;

#define MESSAGE_LISTING_SCREEN_BOTTOM 	"New   Delete"
#define MESSAGE_DISPLAY_SCREEN_BOTTOM 	"Back  Delete"
#define MESSAGE_EDIT_SCREEN_BOTTOM    	"Back  Delete"
#define MESSAGE_NUMBER_SCREEN_BOTTOM	"Back        "
#define BLANK_LINE						"            "

#define INT_TO_CH(n) ((n) + '0')

#define GUI_THREAD_STACK_SIZE 1024
#define GUI_THREAD_PRIORITY 1

char gGuiThreadStack[GUI_THREAD_STACK_SIZE];

TX_THREAD gGuiThread;

//we want to refresh the screen at the first time
bool gViewRefreshScreen = true;

INT gSelectedLineIndex = 0;
/*
 * FWD declaration of functions
 */
void viewRefresh();
void lcdDone();

/*
 * this is the gui main loop. basically the thread sleeps until the LCD need to
 * be refreshed
 */
void guiThreadMainLoopFunc(ULONG v)
{
	ULONG actualFlag;

	while(true)
	{
		viewRefresh();
		tx_event_flags_get(&gGuiRefershEventFlags,1,TX_AND_CLEAR,&actualFlag,TX_WAIT_FOREVER);
	}
}



/*
 * initiation of the view
 */
UINT viewInit()
{
	UINT status;
	result_t apiStatus;

	do
	{
		apiStatus = lcd_init(lcdDone);

		if (apiStatus != OPERATION_SUCCESS)
		{
			status = TX_PTR_ERROR;
			break;
		}

		status = tx_event_flags_create(&gGuiRefershEventFlags,"Gui Event Flags");

		if (status != TX_SUCCESS)
		{
			break;
		}

		status = tx_event_flags_create(&gLcdIdleEventFlags,"Lcd Idle Event Flags");

		if (status != TX_SUCCESS)
		{
			break;
		}

		status = tx_thread_create(	&gGuiThread,
									"Gui Thread",
									guiThreadMainLoopFunc,
									0,
									gGuiThreadStack,
									GUI_THREAD_STACK_SIZE,
									GUI_THREAD_PRIORITY,
									GUI_THREAD_PRIORITY,
									TX_NO_TIME_SLICE, TX_AUTO_START
									);


	}while(false);

	return status;

}

void deinit()
{
	tx_event_flags_delete(&gGuiRefershEventFlags);
	tx_event_flags_delete(&gLcdIdleEventFlags);
	tx_thread_delete(&gGuiThread);

}

/*
 * callback when LCD operation done
 */
void lcdDone()
{
	//signal the thread that operation done
	tx_event_flags_set(&gLcdIdleEventFlags,1,TX_OR);
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

	SMS_DELIVER* pInSms;
	SMS_SUBMIT* pOutSms;

	*pLine++ = INT_TO_CH(leftDigit);
	*pLine++ = INT_TO_CH(rightDigit);
	*pLine++ = ' ';

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
		*pLine++ = 'O';
	}

}

void renderMessageListingScreen()
{
	int i,smsSerialNumber;
	SmsLinkNodePtr pMessage,pSelectedMessage;
	CHAR line[SCREEN_WIDTH];

	//get the message that appear at the first line
	pMessage = modelGetFirstSmsOnScreen();

	//get the selected message
	pSelectedMessage = modelGetSelectedSms();

	//get the serial number of the first message
	smsSerialNumber = modelGetSmsSerialNumber(pMessage);


	//cehck if we need to render the whole screen
	if (gViewRefreshScreen)
	{
		//render every line
		for(i = 0 ; i < SCREEN_HEIGHT-1 ; ++i)
		{
			//if we reached the end of the messages, just print blank lines
			if (!pMessage)
			{
				blockingSetLcdLine(i,false,BLANK_LINE,SCREEN_WIDTH);
			}
			else
			{
				//prepare the line to print
				setMessageListingLineInfo(pMessage,smsSerialNumber+i,line);

				//print the line
				blockingSetLcdLine(i,(pMessage==pSelectedMessage),line,SCREEN_WIDTH);

				pMessage = pMessage->pNext;
			}

			if (pMessage==pSelectedMessage)
			{
				gSelectedLineIndex = i;
			}
		}

		//print the bottom of the screen
		blockingSetLcdLine(SCREEN_HEIGHT-1,true,MESSAGE_LISTING_SCREEN_BOTTOM,SCREEN_WIDTH);

		gViewRefreshScreen = false;
	}
	//we only need to render specific lines
	else
	{
		//find the offset of the selected message, this is also the line of the selected message
		i = 0;
		while(pMessage++ != pSelectedMessage)
		{
			i++;
		}

		/* check what was the last button been pressed*/

		//up button - print the message after the selected message
		if (modelGetLastButton() == BUTTON_2)
		{
			gSelectedLineIndex--;
			setMessageListingLineInfo(pMessage->pNext,smsSerialNumber+i+1,line);
			blockingSetLcdLine(i+1,false,line,SCREEN_WIDTH);

		}
		//down button - print the message before the selected message
		else
		{
			gSelectedLineIndex++;
			setMessageListingLineInfo(pMessage->pPrev,smsSerialNumber+i-1,line);
			blockingSetLcdLine(i-1,false,line,SCREEN_WIDTH);
		}
		//any other case will be considered as "refresh screen" (message deletion and message receive)


		//print the selected message
		setMessageListingLineInfo(pMessage,smsSerialNumber+i,line);
		//print the line
		blockingSetLcdLine(i,true,line,SCREEN_WIDTH);
	}
}

void renderMessageEditScreen()
{
	UINT i;
	if (gViewRefreshScreen)
	{
		//clear the whole screen
		for(i = 0 ; i < SCREEN_HEIGHT-1 ; ++i)
		{
			blockingSetLcdLine(i,false,BLANK_LINE,SCREEN_WIDTH);
		}
		//display the bottom line
		blockingSetLcdLine(SCREEN_HEIGHT-1,true,MESSAGE_EDIT_SCREEN_BOTTOM,SCREEN_WIDTH);

		gViewRefreshScreen = false;
	}
	else
	{
		SMS_SUBMIT* pMessage = modelGetInEditSms();

		//get the index of the last row
		if (pMessage->data_length == 0)
		{
			i = 0;
		}
		else
		{
			i = (pMessage->data_length-1)/SCREEN_WIDTH;
		}

		//print the last row (add spaces if needed)
		UINT lastRowLen = pMessage->data_length % SCREEN_WIDTH;
		CHAR line[SCREEN_WIDTH];
		memset(line,' ',SCREEN_WIDTH);
		memcpy(line,pMessage->data + SCREEN_WIDTH*i,lastRowLen);
		blockingSetLcdLine(i,false,line,SCREEN_WIDTH);

		//if the last button was "delete char" an we moved to the last position at the previouse
		//line, we should clear the next line from the one char it still has
		if (lastRowLen == SCREEN_WIDTH && modelGetLastButton() == BUTTON_NUMBER_SIGN)
		{
			blockingSetLcdLine(i+1,false,BLANK_LINE,SCREEN_WIDTH);
		}
	}
}

void renderMessageNumberScreen()
{
	SMS_SUBMIT* pMessage = modelGetInEditSms();
	CHAR line[SCREEN_WIDTH];
	UINT i;

	//set and display the number currently typed
	memcpy(line,pMessage->device_id,ID_MAX_LENGTH);
	memset(line+ID_MAX_LENGTH,' ',SCREEN_WIDTH-ID_MAX_LENGTH);

	blockingSetLcdLine(0,false,line,SCREEN_WIDTH);

	//in case of refresh, clear the rest of the screen and draw the bottom line
	if (gViewRefreshScreen)
	{
		for(i = 1 ; i < SCREEN_WIDTH-1 ; ++i)
		{
			blockingSetLcdLine(i,false,BLANK_LINE,SCREEN_WIDTH);
		}
		blockingSetLcdLine(SCREEN_HEIGHT-1,true,MESSAGE_NUMBER_SCREEN_BOTTOM,SCREEN_WIDTH);

		gViewRefreshScreen = false;
	}
}

void renderMessageDisplayScreen()
{
	//since the whole message fits the screen, the only option to render this screen
	//is if we enter this screen, and in this case, we need full refresh
	if (gViewRefreshScreen)
	{

		SmsLinkNodePtr pMessage = modelGetSelectedSms();

		UINT dataLength;
		CHAR* pMessageBuffer;
		CHAR line[SCREEN_WIDTH];

		memset(line,' ',SCREEN_WIDTH);

		if (pMessage->type == INCOMMING_MESSAGE)
		{
			SMS_DELIVER* pInMsg = (SMS_DELIVER*)pMessage->pSMS;

			dataLength =pInMsg->data_length;
			pMessageBuffer =pInMsg->data;
			memcpy(line,"F:",2);
			memcpy(line+2,pInMsg->sender_id,ID_MAX_LENGTH);
		}
		else
		{
			SMS_SUBMIT* pOutMsg = (SMS_SUBMIT*)pMessage->pSMS;
			dataLength =pOutMsg->data_length;
			pMessageBuffer =pOutMsg->data;
			memcpy(line,"T:",2);
			memcpy(line+2,pOutMsg->recipient_id,ID_MAX_LENGTH);
		}

		blockingSetLcdLine(0,true,line,SCREEN_WIDTH);

		//should print timestamp
		blockingSetLcdLine(1,true,BLANK_LINE,SCREEN_WIDTH);

		UINT fullLines = dataLength/SCREEN_WIDTH;
		UINT i,lineIndex;



		//print message lines that takes full line
		for(i = 0,lineIndex=2 ; i < fullLines ; ++i,++lineIndex)
		{
			blockingSetLcdLine(lineIndex,false,pMessageBuffer,SCREEN_WIDTH);
			pMessageBuffer+=SCREEN_WIDTH;
		}

		//concatenate the remaining of the message to spaces and print the line
		UINT charsRemain =  dataLength %  SCREEN_WIDTH;

		if (charsRemain > 0)
		{
			memset(line,' ',SCREEN_WIDTH);
			memcpy(line,pMessageBuffer,charsRemain);
			blockingSetLcdLine(lineIndex++,false,line,SCREEN_WIDTH);
		}

		//print an empty lines until the end of the screen
		for(;lineIndex<SCREEN_HEIGHT-1;++lineIndex)
		{
			blockingSetLcdLine(lineIndex,false,BLANK_LINE,SCREEN_WIDTH);
		}

		//print the
		blockingSetLcdLine(SCREEN_HEIGHT-1,true,MESSAGE_DISPLAY_SCREEN_BOTTOM,SCREEN_WIDTH);

		gViewRefreshScreen = false;
	}
}

void viewRefresh()
{
	screen_type currentScreen = modelGetCurentScreenType();
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
			renderMessageDisplayScreen();
			break;

	}
}

bool viewIsFirstRowSelected()
{
	return gSelectedLineIndex==0;
}

bool viewIsLastRowSelected()
{
	return gSelectedLineIndex==(SCREEN_HEIGHT-1);
}


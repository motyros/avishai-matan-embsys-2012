#include "smsModel.h"
#include "embsys_sms_protocol.h"

typedef struct
{
	SmsLinkNodePtr 	pHead;
	SmsLinkNodePtr 	pTail;
	int 			size;
}SmsLinkedList;

SmsLinkedList gSmsDb;

screen_type gCurrentScreen;

SmsLinkNodePtr gpSelectedSms;

SmsLinkNodePtr gpFirstSmsOnScreen;

button gLastPreddedButton;

bool gIsContinuousButtonPress;

SMS_SUBMIT gInEditSms;

//int gRowCursorIndex;
//int gColumnCursorIndex;

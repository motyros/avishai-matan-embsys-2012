#include "smsModel.h"

typedef struct
{
	SmsLinkPtr 	pHead;
	SmsLinkPtr 	pTail;
	int 		size;
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

/* list off all sms (incomming and outgoing */

typedef enum
{
    MESSAGE_LISTING_SCREEN	= 0,
    MESSAGE_DISPLAY_SCREEN	= 1,
    MESSAGE_EDIT_SCREEN		= 2,
    MESSAGE_NUMBER_SCREEN	= 3,
} screen_type;

typedef enum
{
    INCOMMING_MESSAGE		= 0,
    OUTGOING_MESSAGE		= 1,	
} message_type;

typedef struct
{
	//serial number
	//number
	//text
	//textlength
	//timestamp
}SMS,*SMSPtr;

typedef struct
{
	SMSLinkPtr 	pNext;
	SMSLinkPtr 	pPrev;
	SMSPtr 		pSMS;
}SMSLinkNode,*SMSLinkNodePtr;

SMSLinkNode gSMSList;

screen_type gCurrentScreen;
SMSPtr	    gpSelectedSms;

button gLastPreddedButton;
bool gIsContinuousButtonPress;







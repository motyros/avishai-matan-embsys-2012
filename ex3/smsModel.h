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

/*
typedef struct _SMS_DELIVER {

  char sender_id[ID_MAX_LENGTH];
  char timestamp[TIMESTAMP_MAX_LENGTH];
  unsigned data_length;  
  char data[DATA_MAX_LENGTH];

}SMS_DELIVER;

typedef struct _SMS_SUBMIT {

  char device_id[ID_MAX_LENGTH];
  char msg_reference;
  char recipient_id[ID_MAX_LENGTH];  
  unsigned data_length;  
  char data[DATA_MAX_LENGTH];

}SMS_SUBMIT;
*/

#define

typedef struct
{
	SmsLinkPtr 	pNext;
	SmsLinkPtr 	pPrev;
	message_type	type;
	void* 		pSMS;
}SmsLinkNode,*SmsLinkNodePtr;

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

//int gRowCursorIndex;
//int gColumnCursorIndex;

SMS_SUBMIT gInEditSms;

//init all dast, create memory pools
smsModelInit();

screen_type getCurentScreenType();

button getLastButton();
bool isContinuousButtonPress();
void addSmsToDb(void* pSms,const message_type type);
void removeSmsFromDb(const SmsLinkNodePtr pSms);
int getSmsSerialNumber(const SmsLinkNodePtr pSms);
SMS_SUBMIT* getInEditSms();












#ifndef __SMS_MODEL_H__
#define __SMS_MODEL_H__

/* list off all sms (incomming and outgoing */

#include "common_defs.h"

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

typedef struct
{
	SmsLinkPtr 	pNext;
	SmsLinkPtr 	pPrev;
	message_type	type;
	void* 		pSMS;
}SmsLinkNode,*SmsLinkNodePtr;


/*
 *
 */
modelInit();

screen_type getCurentScreenType();

button getLastButton();
bool isContinuousButtonPress();
void addSmsToDb(void* pSms,const message_type type);
void removeSmsFromDb(const SmsLinkNodePtr pSms);
int getSmsSerialNumber(const SmsLinkNodePtr pSms);
SMS_SUBMIT* getInEditSms();



#endif












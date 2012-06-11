#ifndef __SMS_MODEL_H__
#define __SMS_MODEL_H__

/* list off all sms (incomming and outgoing */

#include "common_defs.h"
#include "TX/tx_api.h"
#include "input_panel/input_panel.h"
#include "embsys_sms_protocol.h"

#define MAX_NUM_SMS 100
#define SMS_BLOCK_SIZE sizeof(SMS_SUBMIT)
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

/*this struct is recursive */
typedef struct
{
	SmsLinkNodePtr	pNext;
	SmsLinkNodePtr	pPrev;
	message_type	type;
	void*			pSMS;
}SmsLinkNode,*SmsLinkNodePtr;


/*
 * initialize the structs
 */
UINT modelInit();

/*
 * get the enum of the current been displayed screen
 */
screen_type modelGetCurentScreenType();

/*
 * get the last pressed button
 */
button modelGetLastButton();

/*
 * get true if time since last button pressed is short
 * to be considered as continuous button press
 */
bool modelIsContinuousButtonPress();

/*
 * add an sms to sms db
 */
UINT modelAddSmsToDb(void* pSms,const message_type type);

/*
 * remove sms from sms db
 */
UINT modelRemoveSmsFromDb(const SmsLinkNodePtr pSms);

/*
 * get the serial number of an sms
 */
int modelGetSmsSerialNumber(const SmsLinkNodePtr pSms);

/*
 * get the sms that been edit right now
 */
SMS_SUBMIT* modelGetInEditSms();


SmsLinkNodePtr modelGetFirstSmsOnScreen();

SmsLinkNodePtr modelGetSelectedSms();
#endif












#ifndef __SMS_MODEL_H__
#define __SMS_MODEL_H__

/* list off all sms (incomming and outgoing */

#include "common_defs.h"
#include "TX/tx_api.h"
#include "input_panel/input_panel.h"
#include "embsys_sms_protocol.h"

typedef enum
{
    INCOMMING_MESSAGE		= 0,
    OUTGOING_MESSAGE		= 1,	
} message_type;

#define MAX_NUM_SMS (100)
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

typedef struct SmsLinkNode SmsLinkNode;
typedef struct SmsLinkNode* SmsLinkNodePtr;

struct SmsLinkNode
{
	SmsLinkNodePtr	pNext;
	SmsLinkNodePtr	pPrev;
	message_type	type;
	void*			pSMS;
};


/*
 * initialize the structs
 */
UINT modelInit();

/*
 * get the enum of the current been displayed screen
 */
screen_type modelGetCurentScreenType();

/*
 * set the enum of the current been displayed screen
 */
void modelSetCurrentScreenType(screen_type screen);

/*
 * get the last pressed button
 */
button modelGetLastButton();

/*
 * set the last pressed button
 */
 void modelSetLastButton(const button);

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
void modelSetFirstSmsOnScreen(const SmsLinkNodePtr pSms);

SmsLinkNodePtr modelGetSelectedSms();
void modelSetSelectedSms(const SmsLinkNodePtr pSms);

/*
 * set the if next button press will be considered as continuous
 */
void modelSetIsContinuousButtonPress(bool status);

/*
 * get is continuous button press
 */
bool modelIsContinuousButtonPress();

void modelDeleteSmsFromDb(SmsLinkNodePtr pSms);

UINT modelGetSmsDbSize();

SmsLinkNodePtr modelGetFirstSms();

#endif












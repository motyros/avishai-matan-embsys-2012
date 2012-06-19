#ifndef __SMS_MODEL_H__
#define __SMS_MODEL_H__

/* list off all sms (incomming and outgoing */

#include "common_defs.h"
#include "TX/tx_api.h"
#include "input_panel/input_panel.h"
#include "embsys_sms_protocol.h"

typedef enum
{
    INCOMMING_MESSAGE           = 0,
    OUTGOING_MESSAGE            = 1,    
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
        SmsLinkNodePtr  pNext;
        SmsLinkNodePtr  pPrev;
        message_type    type;
        void*                   pSMS;
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
 void modelSetLastButton(const button b);


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

/*
 * get the first sms on the screen
 */
SmsLinkNodePtr modelGetFirstSmsOnScreen();

/*
 * set the first sms on screen
 */
void modelSetFirstSmsOnScreen(const SmsLinkNodePtr pSms);

/*
 * get the selected sms
 */
SmsLinkNodePtr modelGetSelectedSms();

/*
 * set the selected sms
 */
void modelSetSelectedSms(const SmsLinkNodePtr pSms);

/*
 * set the if next button press will be considered as continuous
 */
void modelSetIsContinuousButtonPress(bool status);

/*
 * get is continuous button press
 */
bool modelIsContinuousButtonPress();

/*
 * get the size of the sms linked list data-base
 */
UINT modelGetSmsDbSize();

/*
 * get the first sms of the sms linked list
 */
SmsLinkNodePtr modelGetFirstSms();

/*
 * try to get the lock (mutex) on the sms data-base 
 */
TX_STATUS modelAcquireLock();

/*
 * try to release the lock (mutex) on the sms data-base
 */
TX_STATUS modelReleaseLock();

#endif



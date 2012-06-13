#include "common_defs.h"
#include "TX/tx_api.h"
#include "smsController.h"
#include "smsModel.h"
#include "smsView.h"
#include "smsController.h"
#include "timer/timer.h"
#include <string.h>
SMS_SUBMIT gOutgoingSms;
SMS_DELIVER gIncommingSms;

void addMessagesToDB()
{

	memcpy(gOutgoingSms.data,"Hi How Are You",14);
	memcpy(gOutgoingSms.device_id,"00000000",8);
	memcpy(gOutgoingSms.recipient_id,"11111111",8);
	gOutgoingSms.data_length = 14;

	modelAddSmsToDb(&gOutgoingSms,OUTGOING_MESSAGE);


	memcpy(gIncommingSms.data,"Fine, thanks :)",15);
	memcpy(gIncommingSms.sender_id,"11111111",8);
	gIncommingSms.data_length = 15;

	modelAddSmsToDb(&gIncommingSms,INCOMMING_MESSAGE);
}
//TX will call this function after system init
void tx_application_define(void *first) 
{
	TX_STATUS status;

	status = modelInit();
	DBG_ASSERT(status == TX_SUCCESS);

	status = viewInit();
	DBG_ASSERT(status == TX_SUCCESS);

	//status = controllerInit();
	//DBG_ASSERT(status == TX_SUCCESS);

	arm_tx_timer(5);

	//addMessagesToDB();

	//create some threads in running mode - after this function ends, TX will scedule them to run
	_enable();
}

void main()
{
    //entry point of TX
	tx_kernel_enter();
}


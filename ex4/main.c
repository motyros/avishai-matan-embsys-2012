#include "common_defs.h"
#include "TX/tx_api.h"
#include "smsController.h"
#include "smsModel.h"
#include "smsView.h"
#include "timer/timer.h"


//TX will call this function after system init
void tx_application_define(void *first) 
{
	TX_STATUS status;

	//init the MVC components. each component creates its own threads
	status = modelInit();
	DBG_ASSERT(status == TX_SUCCESS);

	status = viewInit();
	DBG_ASSERT(status == TX_SUCCESS);

	status = controllerInit();
	DBG_ASSERT(status == TX_SUCCESS);

	//set initial values to the timer tx using
	timer_arm_tx_timer(TX_TICK_MS);

	//enable interrupts
	_enable();
}

void main()
{
    //entry point of TX
	tx_kernel_enter();
}


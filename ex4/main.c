#include "common_defs.h"
#include "TX/tx_api.h"
//#include "smsController.h"
//#include "smsModel.h"
//#include "smsView.h"
#include "timer/timer.h"
#include "flash/flash.h"
#include <string.h>

char gThreadStack[1024];
TX_THREAD gThread;


void threadMainLoopFunc(ULONG v)
{
	uint8_t str[64];
	int i;
        result_t fStatus;
	for(i = 0 ; i < 63 ; ++i)
	{
		str[i] = 'a';
	}
	str[i] = 'b';
	
	fStatus = flash_write(1024*64-1,1,str);
	
	memset(str,0x00,64);
	
	fStatus = flash_read(1024*64-1,1,str);
	
	i = 5;
	i++;
}

static void flash_data_recieve_cb(uint8_t const *buffer, uint32_t size)
{
	int a = 0;
	a = a + 1;
}

static void flash_request_done_cb(void)
{
	int a = 0;
	a = a + 1;
}

//TX will call this function after system init
void tx_application_define(void *first) 
{
	TX_STATUS status;
	result_t fStatus;
	//init the MVC components. each component creates its own threads
	//status = modelInit();
	//DBG_ASSERT(status == TX_SUCCESS);

	//status = viewInit();
	//DBG_ASSERT(status == TX_SUCCESS);

	//status = controllerInit();
	//DBG_ASSERT(status == TX_SUCCESS);

	//set initial values to the timer tx using
	fStatus = flash_init(flash_data_recieve_cb,flash_request_done_cb);
	
        

	status = tx_thread_create(	&gThread,
					"Thread",
					threadMainLoopFunc,
					0,
					gThreadStack,
                                        1024,
					0,//GUI_THREAD_PRIORITY,
					0,//GUI_THREAD_PRIORITY,
					TX_NO_TIME_SLICE, TX_AUTO_START
					);

	timer_arm_tx_timer(TX_TICK_MS);

	//enable interrupts
	_enable();
}

void main()
{
    //entry point of TX
	tx_kernel_enter();
}


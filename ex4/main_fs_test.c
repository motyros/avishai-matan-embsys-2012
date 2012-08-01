#include "common_defs.h"
#include "TX/tx_api.h"
//#include "smsController.h"
//#include "smsModel.h"
//#include "smsView.h"
#include "timer/timer.h"
#include "flash/flash.h"
#include "fs/fs.h"
#include <string.h>

char gThreadStack[1024];
TX_THREAD gThread;

bool gOpDone = false;

void num2str(uint16_t num,char* str)
{
	char temp[5];
	int j,i = 0;

	for(i = 0 ; i < 5 ; ++i)
	{
		temp[i] = num%10 + '0';

		num = num / 10;
	}


	for(j = 0 ; j < 5 ; ++j)
	{
		str[j] = str[5 - j - 1];
	}
}

void threadMainLoopFunc(ULONG v)
{
	FS_SETTINGS settings;
	FS_STATUS status;

	char data[512];
	unsigned len = 512,i;
	char filename[5];
	settings.block_count = 16;
	status = fs_init(settings);
	

	for(i = 0 ; i < 1000 ; ++i)
	{
		num2str(i,filename);
		fs_write(filename,0,data);
	}
	status = SUCCESS;
	
}

static void flash_data_recieve_cb(uint8_t const *buffer, uint32_t size)
{
	int a = 0;
	a = a + 1;
	gOpDone = true;
}

static void flash_request_done_cb(void)
{
	int a = 0;
	a = a + 1;
	gOpDone = true;
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
	//fStatus = flash_init(flash_data_recieve_cb,flash_request_done_cb);
	
        

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


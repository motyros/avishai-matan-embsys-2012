#include "common_defs.h"
#include "TX/tx_api.h"
#include "smsController.h"
#include "smsModel.h"
#include "smsView.h"
#include "timer/timer.h"

#define MAIN_INIT_THREAD_STACK_SIZE (1024)
#define MAIN_INIT_PRIORITY (1)
//keypad Thread
TX_THREAD gMainInitThread;
CHAR gMainInitThreadStack[MAIN_INIT_THREAD_STACK_SIZE];
void mainInitThreadMainFunc(ULONG v);

//TX will call this function after system init
void tx_application_define(void *first) 
{
	TX_STATUS status;

    status = tx_thread_create(      &gMainInitThread,
                                                            "Main Init Thread",
                                                            mainInitThreadMainFunc,
                                                            0,
                                                            gMainInitThreadStack,
                                                            MAIN_INIT_THREAD_STACK_SIZE,
                                                            MAIN_INIT_PRIORITY,
                                                            MAIN_INIT_PRIORITY,
                                                            TX_NO_TIME_SLICE, TX_AUTO_START
                                                            );
    DBG_ASSERT(status == TX_SUCCESS);


    //set initial values to the timer tx using
    timer_arm_tx_timer(TX_TICK_MS);

    //enable interrupts
    _enable();
}

/*
 * the main function for the init main thread
 */
void mainInitThreadMainFunc(ULONG v)
{

		TX_STATUS status;

    	//init the MVC components. each component creates its own threads
    	status = modelInit();
    	DBG_ASSERT(status == TX_SUCCESS);

    	status = viewInit();
    	DBG_ASSERT(status == TX_SUCCESS);

    	status = controllerInit();
    	DBG_ASSERT(status == TX_SUCCESS);
}

void main()
{
    //entry point of TX
	tx_kernel_enter();
}


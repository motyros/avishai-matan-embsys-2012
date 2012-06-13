#include "TX/tx_api.h"
#include "input_panel/input_panel.h"
#include "smsController.h"
#include "smsView.h"
#include "smsModel.h"


#define KEY_PAD_THREAD_STACK_SIZE 1024
#define KEY_PAD_PRIORITY 1

TX_THREAD gKeyPadThread;
CHAR gKeyPadThreadStack[KEY_PAD_THREAD_STACK_SIZE];


TX_EVENT_FLAGS_GROUP gKeyPadEventFlags;
button gCurrentButton;

void buttonPressedCB(button b)
{

}

void keyPadThreadMainFunc(ULONG v)
{
	while(true)
	{
		//wait for event flags
	}
}

TX_STATUS controllerInit()
{
	/* create three threads
	 *
	 * 1. will listed to the keypad
	 * 2. will listed to the network: received_cb
	 * 3. will send data to the network: transmitted_cb, error_cb, dropped_cb
	 */

	TX_STATUS status;

	status = tx_thread_create(	&gKeyPadThread,
								"Keypad Thread",
								keyPadThreadMainFunc,
								0,
								gKeyPadThreadStack,
								KEY_PAD_THREAD_STACK_SIZE,
								KEY_PAD_PRIORITY,
								KEY_PAD_PRIORITY,
								TX_NO_TIME_SLICE, TX_AUTO_START
								);

	ip_init(buttonPressedCB);
	/*
	 * init network: init a full struct for the transmit_buffer and recieve_buffer,
	 * 				call backs for the network: transmitted_cb, received_cb, dropped_cb, error_cb
	 *
	 * init keypad: call back for the keypad
	 * timer - for periodically send PROBE or SMS_SUBMIT
	 * timer - for turning off continues button press after x ms
	 *
	 */

	/*
	 *
	 *
	 *
	 */

	modelSetCurrentScreenType(MESSAGE_LISTING_SCREEN);
	//TODO set the selected messages and the fist message in line

}


//decide what to do according the cuurent screen, current state etc
void controllerButtonPressed(const button b)
{
	switch (modelGetCurentScreenType())
	{
	case MESSAGE_DISPLAY_SCREEN:
		handleDisplayScreen(b);
		break;
	case MESSAGE_EDIT_SCREEN:
		handleEditScreen(b);
		break;
	case MESSAGE_LISTING_SCREEN:
		handleListingScreen(b);
		break;
	case MESSAGE_NUMBER_SCREEN:
		handleNumberScreen(b);
		break;
	}
}

//pass the info
void controllerPacketArrived(const uint8_t* pBuffer,const uint32_t size)
{
}


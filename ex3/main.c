#include "TX/tx_api.h"
#include "smsController.h"

//main will contain ALL callbacks. those callbacks will update some globals (such as the button that been pressed)
//and will signal the events flag of the relevant thread

button gPressedButton;

void buttonThreadEntry(...)
{
	while(true)
	{
		//wait for events flagg
		controllerButtonPressed(gPressedButton);
	}
}
//TX will call this function after system init
void tx_application_define(void *first) 
{
	//create some threads in running mode - after this function ends, TX will scedule them to run
	_enable();
}

void main()
{
    //entry point of TX
	tx_kernel_enter();
}


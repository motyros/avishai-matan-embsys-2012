#include "TX/tx_api.h"

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

#include "timer/timer.h"
#include "input_panel/input_panel.h"
#include "flash/flash.h"
#include "LCD/LCD.h"
#include "network/network.h"

int gCounter = 0;
uint32_t gA = 0;
uint32_t gB = 0;
uint32_t gC = 0;
void lcdWrite(uint32_t row,bool sel,char* line,uint32_t len)
{
    result_t result = NOT_READY;
    while (result == NOT_READY)
    {
        result  = lcd_set_row(row, sel, line, len);
    }

}
void incCounter()
{
    gCounter++; 
}

bool gFlashOpDone = false;

button gButton;
void flushComplete()
{
    //gFlashOpDone = true;
}

void buttonPressed(button b)
{
    gButton = b;
}



void flashOpComplete(void)
{
	char line[] = {'S','o','m','e','*','t','h','i','n','g'};
	lcdWrite(1,gFlashOpDone , line, 10);
    gFlashOpDone = true;

}

void flashReadDone(uint8_t const *buffer, uint32_t size)
{
	lcdWrite(1,gFlashOpDone , (char*)buffer, 12);
    gFlashOpDone = true;
}

#define BUF_SIZE 70

result_t flashTest()
{
	uint8_t buf[BUF_SIZE];// = {'1','2','1','2','1','2','1','2','1','2'};
	uint8_t buf2[BUF_SIZE];

	uint32_t i = 0;
	result_t res;

	for (i = 0 ; i < BUF_SIZE-1 ; ++i)
	{
		buf[i] = 't';
	}
	buf[BUF_SIZE-1] = 'z';

	flash_init(flashReadDone,flashOpComplete);
    gFlashOpDone = false;
    flash_bulk_erase_start();
    while(!gFlashOpDone);
    gFlashOpDone = false;
	res = flash_write_start(9,BUF_SIZE,(uint8_t*)&buf);
	while(!gFlashOpDone);
    while(!flash_is_ready());
	res = flash_read(9,BUF_SIZE,(uint8_t*)&buf2);
    while(!flash_is_ready());
    res = flash_read(9,BUF_SIZE,(uint8_t*)&buf2);
    while(!flash_is_ready());

	//if(res==OPERATION_SUCCESS) lcdWrite(2,gFlashOpDone,(char*) buf2,12);
	//res = flash_read(9,BUF_SIZE,(uint8_t*)&buf2);
	//if(res==OPERATION_SUCCESS) lcdWrite(3,gFlashOpDone,(char*) buf2,12);
	//res = flash_write(1000,BUF_SIZE,(uint8_t*)&buf2);
	//gFlashOpDone = false;
	//res = flash_read_start(1000,BUF_SIZE);
//	while(!gFlashOpDone);
    //while(!flash_is_ready());

	return res;
}

void lcdTest()
{
//    char line[] = {'S','o','m','e','_','t','h','i','n','g'};
//    lcdWrite(4,line,10);
    char line1[] = {'e','r','t','Q','!',' ','3','e','$','%','t',')'};
    lcdWrite(5, false, line1, 12);
    char line2[] = {'d','o',' ','i','t','!'};
    lcdWrite(17, false, line2, 6);
}

int gTimer0Counter = 0;
int gTimer1Counter = 0;

void timer0cb()
{
	gTimer0Counter++;
}

void timer1cb()
{
	gTimer1Counter++;
}

void timerTest()
{
	timer0_register(10,true,timer0cb);
	timer1_register(5,false,timer1cb);
	int i = 0;

	//make sure gTimer0Counter = 1 and gTimer1Counter keep increasing
	while (i < 1000)
	{
		i++;
		if (i>100)
		{
			i = 0;
		}
	}
}
#define NET_BUF_SIZE 32
network_init_params_t networkInit;
desc_t tranBuf[NET_BUF_SIZE];
desc_t recBuf[NET_BUF_SIZE];
void networkTest()
{
    networkInit.transmit_buffer = tranBuf;
    networkInit.size_t_buffer = NET_BUF_SIZE;
    networkInit.recieve_buffer = recBuf;
    networkInit.size_r_buffer = NET_BUF_SIZE;
    uint8_t buf[NET_BUF_SIZE];



    buf[0]='a',buf[1]='b',buf[2]='c';


    network_init(&networkInit);
    network_set_operating_mode(NETWORK_OPERATING_MODE_NORMAL);

    network_send_packet_start((uint8_t*)buf,NET_BUF_SIZE,3);

    return;
}

void main()
{
    _enable();
    networkTest();
    //verified - working!
	//timerTest();
    
    //lcd_init(flushComplete);
    //lcdTest();
    //flashTest();
    while(true)
    {
        //counter
        gA = _lr(0x21);
        //control
        gB = _lr(0x22);
        //limit
        gC = _lr(0x23);
        gA = gA+1;
    }
}

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
void zeroAllFlash()
{
	uint8_t buf[MAX_REQUEST_BUFFER_SIZE];
	result_t res;
	int i;

	for(i = 0 ; i < MAX_REQUEST_BUFFER_SIZE ; ++i)
	{
		buf[i] = 0x0;
	}

	res = flash_bulk_erase_start();
	while(!flash_is_ready());

	res = flash_write(MAX_REQUEST_BUFFER_SIZE*128-5,MAX_REQUEST_BUFFER_SIZE,buf);

	for(i = 0 ; i < 127 ; ++i)
	{
		res = flash_write(MAX_REQUEST_BUFFER_SIZE*i,MAX_REQUEST_BUFFER_SIZE,buf);
		i = (i+1)-1;
	}

}
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

	zeroAllFlash();

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


void txDone(const uint8_t *buffer, uint32_t size)
{

}

int lcdNum = 0;
/*
 * call back when a packet was received.
 */
void rxDone(uint8_t buffer[], uint32_t size, uint32_t length)
{
	lcd_set_row(lcdNum++,false,(char*)buffer,length);
}

/*
 * call back when a packet was dropped during receiving.
 */
void rxPacketDrop(packet_dropped_reason_t)
{

}

/*
 * call back when a packet was dropped during transmission.
 */
void txPacketDrop(transmit_error_reason_t,
                                           uint8_t *buffer,
                                           uint32_t size,
                                           uint32_t length )
{

}

#define NET_BUF_SIZE 5
network_init_params_t networkInit;
desc_t tranBuf[NET_BUF_SIZE];
desc_t recBuf[NET_BUF_SIZE];
uint8_t recData[NET_BUF_SIZE*161];
void networkTest()
{
	network_call_back_t cbs;

	cbs.dropped_cb = rxPacketDrop;
	cbs.recieved_cb = rxDone;
	cbs.transmit_error_cb = txPacketDrop;
	cbs.transmitted_cb = txDone;

	int i;
    for(i = 0 ; i<NET_BUF_SIZE ; ++i)
    {
    	recBuf[i].pBuffer = (uint32_t)(recData+i*161);
    	recBuf[i].buff_size = 161;

    }

    networkInit.transmit_buffer = tranBuf;
    networkInit.size_t_buffer = NET_BUF_SIZE;
    networkInit.recieve_buffer = recBuf;
    networkInit.size_r_buffer = NET_BUF_SIZE;
    networkInit.list_call_backs = cbs;
    uint8_t buf[NET_BUF_SIZE];



    buf[0]='a',buf[1]='b',buf[2]='c';


    network_init(&networkInit);
    network_set_operating_mode(NETWORK_OPERATING_MODE_NORMAL);

    result_t res;
    for(i = 0 ; i < 10 ; ++i)
    {
    	buf[0] = i+33;
    	res = network_send_packet_start((uint8_t*)buf,NET_BUF_SIZE,3);
    	buf[0] = i+33;
    }

    return;
}

int rowNum = 0;
int charNum = 0;
bool sel = false;
#define rowLen 12
char row[rowLen] = {' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '};
void lcdComplete(){}
char buttonToChar(button but)
{
    switch(but)
    {
    case BUTTON_OK: return '&';
    case BUTTON_1: return '1';
    case BUTTON_2: return '2';
    case BUTTON_3: return '3';
    case BUTTON_4: return '4';
    case BUTTON_5: return '5';
    case BUTTON_6: return '6';
    case BUTTON_7: return '7';
    case BUTTON_8: return '8';
    case BUTTON_9: return '9';
    case BUTTON_STAR: return '*';
    case BUTTON_0: return '0';
    case BUTTON_NUMBER_SIGN: return '#';
    }
    return '%';
}
void buttonPressed1(button but)
{
    if(charNum>=12)
    {
        int i=0;
        charNum = 0;
        rowNum++;
        if(rowNum>=18)
        {
            rowNum=0;
        }
        for(i=0;i<rowLen;i++)
        {
            row[i] = ' ';
        }
    }


    if(but == BUTTON_STAR) sel = sel ? false : true;

    row[charNum++] = buttonToChar(but);
    lcd_set_row(rowNum,sel,row,rowLen);
    //if(but == BUTTON_OK)
}

testType()
{

    ip_init(buttonPressed1);
    ip_enable();
    lcd_init(lcdComplete);
    //lcd_set_row(rowNum,sel,row,1);
}

void main()
{
    _enable();
    lcd_init(flushComplete);
    //networkTest();
    //verified - working!
	//timerTest();
    

    //lcdTest();
    flashTest();
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

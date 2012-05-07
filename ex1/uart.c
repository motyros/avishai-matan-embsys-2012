#include "uart.h"
#include "defs.h"

#define UART_BASE_ADDR 0x100000

#define UART_MAX_BAUD_RATE 115200
#define UART_BAUD_RATE 19200


//pack the struct with no internal spaces
#pragma pack(1)
typedef struct
{
       
	union
	{
		UINT8 THR;
		UINT8 RBR;
		UINT8 DLL;
	}REG0;
	
	union
	{
		UINT8 IER; //not in use
		UINT8 DLH;
	}REG1;
	
	union
	{
		UINT8 IIR; //not in use
		UINT8 FCR; //not in use
	}REG2;
	
	union
	{
		UINT8 data;
		struct
		{
			UINT8 wordLength	: 2;
			UINT8 stop		: 1;
			UINT8 parity		: 3;
			UINT8 breakSignal	: 1;
			UINT8 dlab		: 1;	
		} bits;
	} LCR;


	UINT8 MCR; //not in use
	
	union
	{
		UINT8 data;
		struct
		{
			UINT8 dataReady		:1;
			UINT8 overrunError	:1;
			UINT8 parityError	:1;
			UINT8 framingError	:1;
			UINT8 breakSignal	:1;
			UINT8 thrIsEmpty	:1;
			UINT8 thsEmptyAndIdle	:1;
			UINT8 fifoError		:1;	
		} bits;
	} LSR;

	UINT8 MSR; //not in use
	UINT8 SR;  //not in use

} volatile UartRegs;
#pragma pack()


//cost the UART_BASE_ADDR to the UartRegs struct 
UartRegs *gpUart = (UartRegs*)(UART_BASE_ADDR);

/*
 * Sets the raud range to the given rate
 */
void uartSetBaudRate(const UINT16 rate)
{
	//finding the divisorLatch
	UINT16 divisorLatch = UART_MAX_BAUD_RATE / rate;
	
	//turn on the DLAB bit
	gpUart->LCR.bits.dlab = 1;
	//set the DLL and the DLH registers
	gpUart->REG0.DLL = divisorLatch & 0xFF;
	gpUart->REG1.DLH = divisorLatch>>8;
	//turn off the DLAB bit
	gpUart->LCR.bits.dlab = 0;
}

/*
 * Initilize the Uart
 */
void uartInit()
{

	int i;
	UINT8 *pData = (UINT8*)gpUart;

	//zero all registers except first register
        //otherwize, the uart will send 0x00.
	for(i = 1 ; i < sizeof(UartRegs) ; ++i)
	{
            pData[i] = 0x0;
	}
 	
	//set baud rate according the specification
	uartSetBaudRate(UART_BAUD_RATE);

	//set communication parameters (LCR)
	gpUart->LCR.bits.wordLength = 0x3;
}

/*
 * Return "TURE" (in defs.h) if the bit in LSR for dataReady, is on.
 * retun "FALSE" otherwise.
 */
BOOL uartReadyToRead()
{
	return gpUart->LSR.bits.dataReady;
}

/*
 * Return the byte that stores in the RBR register.
 */
UINT8 uartReadByte()
{
	return gpUart->REG0.RBR;
}

/*
 * Return "TURE" (in defs.h) if the bit in LSR for thrIsEmpty, is on.
 * retun "FALSE" otherwise.
 */
BOOL uartReadyToWrite()
{
	return gpUart->LSR.bits.thrIsEmpty;
}

/*
 * Writes the byte to the THR register.
 */
void uartWriteByte(const UINT8 byte)
{
	gpUart->REG0.THR = byte;
}



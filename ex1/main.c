#include "defs.h"
#include "7segments.h"
#include "clock.h"
#include "uart.h"
#include "cyclicBuffer.h"

UINT8 gInterval;
UINT32 gLastClockReading;
UINT8 gCounter;

CyclicBuffer gReadBuffer;
CyclicBuffer gWriteBuffer;

#define COUNTER_INIT_VAL 0
#define INTERVAL_INIT_VAL 100
#define PROTOCOL_MSG_SIZE 4



void updateCounter()
{
	++gCounter;
	segmentsSetNumber(gCounter);
}

void checkClock()
{
	UINT32 currentClock = clockGetTime();

	//check if the clock started another cycle or not
	if 	( 	
		//no clock overlap
		((currentClock > gLastClockReading) && (currentClock - gLastClockReading >= gInterval)) ||
		//clock overlaps
		((gLastClockReading > currentClock) && (currentClock + (MAX_UINT32 - gLastClockReading) >= gInterval)) 

		)
	{
		updateCounter();
		gLastClockReading = currentClock;
	}
}

UINT8 asciiToDigit(char digit)
{
	if (digit >= '0' && digit <= '9')
	{
		return digit-'0';
	}

	if (digit >= 'a' && digit <= 'f')
	{
		return 10 + digit-'a';
	}

	if (digit >= 'A' && digit <= 'F')
	{
		return 10 + digit-'A';
	} 
	
	return STATUS_ERROR;
	
}

char digitToAscii(UINT8 digit)
{
	if (digit <= 0x9)
	{
		return digit+'0';
	}

	if (digit >= 0xa && digit <= 0xf)
	{
		return digit -10 +'A';
	}
	
	return STATUS_ERROR;
	
}
void handleProtocolMessage()
{
	UINT8 commandByte = cyclicBufferGet(&gReadBuffer);
	UINT8 variableByte = cyclicBufferGet(&gReadBuffer);
	UINT8 valueDigit1 = asciiToDigit(cyclicBufferGet(&gReadBuffer));
	UINT8 valueDigit2 = asciiToDigit(cyclicBufferGet(&gReadBuffer));

	STATUS status = STATUS_ERROR;
	do
	{
		if (valueDigit1 == STATUS_ERROR || valueDigit2 == STATUS_ERROR)
		{
			break;
		}

		UINT8 returnVal;
		UINT8 valueWord = valueDigit1;
		valueWord = (valueWord << 4) +  valueDigit2;

		if (commandByte == 'r' && variableByte == 't' && (valueWord == 0x0))
		{
			returnVal = gInterval;
		}
		else if (commandByte == 'r' && variableByte == 'c' && (valueWord == 0x0))
		{
			returnVal = gCounter;
		}
		else if (commandByte == 'w' && variableByte == 't')
		{
			gInterval = valueWord;
			returnVal = 0x0;
		}
		else if (commandByte == 'w' && variableByte == 'c')
		{
			gCounter = valueWord;
			returnVal = 0x0;
		}
		else
		{
			break;
		}
		//write back the chars as upper case
		cyclicBufferPut(&gWriteBuffer,commandByte-32);
		cyclicBufferPut(&gWriteBuffer,variableByte-32);

		//write back the 4 MSB
		cyclicBufferPut(&gWriteBuffer, digitToAscii(returnVal>>4));
		//write back the 4 LSB
		cyclicBufferPut(&gWriteBuffer, digitToAscii(returnVal & 0x0F));
		status = STATUS_SUCCESS;
		

	}while(FALSE);
	
	if (status == STATUS_ERROR)
	{
		cyclicBufferPut(&gWriteBuffer,'Z');	
	}
	
	return;
	
}

void checkUart()
{
	if (uartReadyToRead())
	{
		//1. read
		cyclicBufferPut(&gReadBuffer,uartReadByte());
		//2. if got 4 bytes, process
		if (gReadBuffer.size == PROTOCOL_MSG_SIZE)
		{
			handleProtocolMessage();	
		}
	}

	if (gWriteBuffer.size > 0 && uartReadyToWrite())
	{
		uartWriteByte(cyclicBufferGet(&gWriteBuffer));		
	}
}

void initDrivers()
{
	clockInit();
	segmentsInit();
	uartInit();
}

/*
 * init all programs component
 */
void init()
{
	initDrivers();
		
	//set init values
	gLastClockReading = clockGetTime();
	gInterval = INTERVAL_INIT_VAL;
	gCounter = COUNTER_INIT_VAL;

	segmentsSetNumber(gCounter);

}
void main(void)
{
	//init drivers and components
	init();

	while(TRUE)
	{
		//check if interval passed and the counter need to be update
		checkClock();
		//check if a request has arrived and handle it
		checkUart();
	
	}
}

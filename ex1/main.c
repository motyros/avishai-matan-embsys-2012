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

//define init values
#define COUNTER_INIT_VAL 0
#define INTERVAL_INIT_VAL 100
//define the size of a messeage
#define PROTOCOL_MSG_SIZE 4


/*
 * Updates the counter and increase it by one,
 * and it updates the 7-segments to the new counter.
 */
void updateCounter()
{
	++gCounter;
	segmentsSetNumber(gCounter);
}

/*
 * Checks if the colck has moved forward (modolo max UINT32) for gInterval,
 * and updates the couter and the 7-segments device.
 */
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

/*
 * Converts the given char to a hex digit.
 * Return STATUS_SUCCESS on legal hex digit, and STATUS_ERROR otherwise.
 */
STATUS asciiToDigit(char digit,UINT8 *pDigit)
{
	
    do
    {
        if (digit >= '0' && digit <= '9')
	{
		*pDigit =  digit-'0';
                break;
	}

	if (digit >= 'a' && digit <= 'f')
	{
		*pDigit =  10 + digit-'a';
                break;
	}

	if (digit >= 'A' && digit <= 'F')
	{
		*pDigit =  10 + digit-'A';
                break;
	} 
	
	return STATUS_ERROR;
    }while(FALSE);

    return STATUS_SUCCESS;
	
}

/*
 * Convert the given digit to a hex char.
 * the method assumes that the UINT digit is not bigger then 0xf, the biggest digit in hex.
 */
char digitToAscii(UINT8 digit)
{
    
    DBG_ASSERT(digit <= 0xf);

    if (digit <= 0x9)
    {
	return  digit+'0';
    }

    return digit -10 +'A';
}

/*
 * Handles aמ incomming message in the Uart protocol.
 */
void handleProtocolMessage()
{
  //gets the command char and the variable char fromthe buffer
	UINT8 commandByte = cyclicBufferGet(&gReadBuffer);
	UINT8 variableByte = cyclicBufferGet(&gReadBuffer);
	UINT8 valueDigit1,valueDigit2;

	STATUS status = STATUS_ERROR;

	do
	{
     //checks if the given value us a legal hex digit
		if (asciiToDigit(cyclicBufferGet(&gReadBuffer),&valueDigit1) != STATUS_SUCCESS)
		{
			break;
		}

		if (asciiToDigit(cyclicBufferGet(&gReadBuffer),&valueDigit2) != STATUS_SUCCESS)
		{
			break;
		}
    
	  UINT8 returnVal;
		UINT8 valueWord = valueDigit1;
		valueWord = (valueWord << 4) +  valueDigit2;
      
      //"rt00"
		if (commandByte == 'r' && variableByte == 't' && (valueWord == 0x0))
		{
			returnVal = gInterval;
		}
      //"rc00"
		else if (commandByte == 'r' && variableByte == 'c' && (valueWord == 0x0))
		{
			returnVal = gCounter;
		}
      //"wtVV"
		else if (commandByte == 'w' && variableByte == 't')
		{
			gInterval = valueWord;
			returnVal = 0x0;
		}
      //"wcVV"
		else if (commandByte == 'w' && variableByte == 'c')
		{
			gCounter = valueWord;
			returnVal = 0x0;
		}
		else
		{
      //Illegal command / variable / value
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

/*
 * Handles the Uart protocol.
 */
void checkUart()
{
  //checks if ready to read
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

  //checks if ready to write
	if (gWriteBuffer.size > 0 && uartReadyToWrite())
	{
		uartWriteByte(cyclicBufferGet(&gWriteBuffer));		
	}
}

/*
 * Initilizes the drivers of all devices.
 */
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
        cyclicBufferInit(&gReadBuffer);
        cyclicBufferInit(&gWriteBuffer);
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

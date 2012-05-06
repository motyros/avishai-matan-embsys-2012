#include "clock.h"
#include "defs.h"

#define CLOCK_ADDR 0x100

/*
 * Initilize the clock to the time "0"
 */
void clockInit()
{
	clockSetTime(0);
}

/*
 * Sets the time to the given UINT32 number
 */
void clockSetTime(const UINT32 time)
{
	_sr(time,CLOCK_ADDR);
}

/*
 * Gets the time of the clock (UINT32)
 */
UINT32 clockGetTime()
{
	return _lr(CLOCK_ADDR);
}

#ifndef __CLOCK_H__
#define __CLOCK_H__

#include "defs.h"

/*
 * Initilize the clock to the time "0"
 */
void clockInit();

/*
 * Sets the time to the given UINT32 number
 */
void clockSetTime(const UINT32 time);

/*
 * Gets the time of the clock (UINT32)
 */
UINT32 clockGetTime();

#endif


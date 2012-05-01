#ifndef __7SEGMENTS_H__
#define __7SEGMENTS_H__

#include "defs.h"

/*
 * driver's init function
 */
void segmentsInit();

/*
 * this function get a number and displays number%100 
 * on the two seven segments screen
 */
void segmentsSetNumber(const INT8 number); 

#endif


#ifndef __CYCLIC_BUFFER_H__
#define __CYCLIC_BUFFER_H__

#include "defs.h"

#define BUFFER_SIZE 512

typedef struct
{
	INT16	pos;
	UINT16  size;		
	UINT8	data[BUFFER_SIZE];
}CyclicBuffer;

/*
 * Puts the given UINT8 data into the given cyclic buffer.
 * the method assumes that the buffer in not full.
 */
void cyclicBufferPut(CyclicBuffer * const pBuffer,const UINT8 data);

/*
 * Gets and pops the data in a FIFO, from the given cyclic buffer.
 * the method assumes that the buffer in not empty.
 */
UINT8 cyclicBufferGet(CyclicBuffer * const pBuffer);

/*
 * Initilize the given cyclic buffer an empty buffer.
 */
void cyclicBufferInit(CyclicBuffer * const pBuffer);

#endif

#ifndef __CYCLIC_BUFFER_H__
#define __CYCLIC_BUFFER_H__

#include "defs.h"

#define BUFFER_SIZE 5

typedef struct
{
	INT16	pos;
	UINT16  size;		
	UINT8	data[BUFFER_SIZE];
}CyclicBuffer;

void cyclicBufferPut(CyclicBuffer * const pBuffer,const UINT8 data);
UINT8 cyclicBufferGet(CyclicBuffer * const pBuffer);
void cyclicBufferInit(CyclicBuffer * const pBuffer);

#endif

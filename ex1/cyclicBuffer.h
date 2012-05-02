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

void cyclicBufferPut(CyclicBuffer *pBuffer,const UINT8 data);
UINT8 cyclicBufferGet(CyclicBuffer *pBuffer);

#endif

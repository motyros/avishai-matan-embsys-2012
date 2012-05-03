#include "cyclicBuffer.h"
#include "defs.h"

void cyclicBufferPut(CyclicBuffer * const pBuffer,const UINT8 data)
{
        DBG_ASSERT(pBuffer != NULL);
	DBG_ASSERT(pBuffer->size < BUFFER_SIZE); 
	
	pBuffer->data[pBuffer->pos] = data;
	pBuffer->size++;
	pBuffer->pos = (pBuffer->pos+1)%BUFFER_SIZE; 
}

UINT8 cyclicBufferGet(CyclicBuffer * const pBuffer)
{
	
        DBG_ASSERT(pBuffer != NULL);
	DBG_ASSERT(pBuffer->size > 0);

	UINT8 result = pBuffer->data[(pBuffer->pos - pBuffer->size + BUFFER_SIZE) % BUFFER_SIZE];
	pBuffer->size--;

	return result;
}

void cyclicBufferInit(CyclicBuffer * const pBuffer)
{
	pBuffer->size = 0;
	pBuffer->pos = 0;
}

#include "cyclicBuffer.h"
#include "defs.h"

/*
 * Puts the given UINT8 data into the given cyclic buffer.
 * the method assumes that the buffer in not full.
 */
void cyclicBufferPut(CyclicBuffer * const pBuffer,const UINT8 data)
{
  //DBG check that the pointer is not null
  DBG_ASSERT(pBuffer != NULL);
  //DBG check that the buffer has space for the data
	DBG_ASSERT(pBuffer->size < BUFFER_SIZE);
   
   //insert the data and increase the size by one.
   pBuffer->data[pBuffer->pos] = data;
   pBuffer->size++;
   //update the position for the next insert. 
   //(by increasing the position by one, modolo the buffer size)
   pBuffer->pos = (pBuffer->pos+1)%BUFFER_SIZE; 
}

/*
 * Gets and pops the data in a FIFO, from the given cyclic buffer.
 * the method assumes that the buffer in not empty.
 */
UINT8 cyclicBufferGet(CyclicBuffer * const pBuffer)
{
  //DBG check that the pointer is not null
  DBG_ASSERT(pBuffer != NULL);
  //DBG check that the buffer has data to get and pop
	DBG_ASSERT(pBuffer->size > 0);

  //the data in located at the point of position minos the size of the buffer, modolo buffer size.
  //we overcome the negative outcomes by adding the buffer suze to the result, before doing the modolo.
	UINT8 result = pBuffer->data[(pBuffer->pos - pBuffer->size + BUFFER_SIZE) % BUFFER_SIZE];
	pBuffer->size--;

	return result;
}

/*
 * Initilize the given cyclic buffer an empty buffer.
 */
void cyclicBufferInit(CyclicBuffer * const pBuffer)
{
	pBuffer->size = 0;
	pBuffer->pos = 0;
}

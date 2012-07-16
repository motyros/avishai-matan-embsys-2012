#include "flash.h"
#include <string.h>
#include <stdio.h>



char flash[FLASH_SIZE];
bool flashInUse = false;

void ensureFlashExist()
{
   FILE* f = fopen("d:\\development\\embsys\\ex4\\flash.img","r");
   char a = '\0';
   if (f == NULL)
   {
      f = fopen("d:\\development\\embsys\\ex4\\flash.img","w+");
      fseek(f,64*1024-1,SEEK_SET);
      fwrite(&a,1,1,f);
      fclose(f);
   }
   else
   {
      fclose(f);
   }
}
result_t flash_init( void (*flash_data_recieve_cb)(uint8_t const *buffer, uint32_t size),
		     void (*flash_request_done_cb)(void))
{
   FILE* f;
   ensureFlashExist();
   f = fopen("d:\\development\\embsys\\ex4\\flash.img","r");
   fread(flash,64*1024,1,f);
   fclose(f);

   return OPERATION_SUCCESS;
}


/**********************************************************************
 *
 * Function:    flash_is_ready
 *
 * Descriptor:
 *
 * Notes:
 *
 * Return:      true in case the FW ready to accept a new request.
 *
 ***********************************************************************/
bool flash_is_ready(void)
{
   return flashInUse;
}


/**********************************************************************
 *
 * Function:    flash_read_start
 *
 * Descriptor:  Read "size" bytes start from address "start_address".
 *              This function is non-blocking.
 *
 * Notes:       size = # bytes to read - must be > 0
 *
 * Return:      OPERATION_SUCCESS: Read request done successfully.
 *              INVALID_ARGUMENTS: One of the arguments is invalid.
 *              NOT_READY:         The device is not ready for a new request.
 *
 ***********************************************************************/
char internalBuffer[MAX_REQUEST_BUFFER_SIZE];

result_t flash_read_start(uint16_t start_address, uint16_t size)
{
   if (flashInUse)
   {
      return NOT_READY;
   }

   flashInUse = true;
   memcpy(internalBuffer,&flash[start_address],size);
   flashInUse = false;

   return OPERATION_SUCCESS;
}


/**********************************************************************
 *
 * Function:    flash_read
 *
 * Descriptor:  Same as flash_read_start except the data received in the supplied
 *              buffer.
 *              This function is blocking.

 * Notes:       size = # bytes to read - must be > 0
 *
 * Return:      OPERATION_SUCCESS: Read request done successfully.
 *              INVALID_ARGUMENTS: One of the arguments is invalid.
 *              NOT_READY:         The device is not ready for a new request.
 *              NULL_POINTER:      One of the arguments points to NULL
 *
 ***********************************************************************/
result_t flash_read(uint16_t start_address, uint16_t size, uint8_t buffer[])
{
   if (flashInUse)
   {
      return NOT_READY;
   }

   flashInUse = true;
   memcpy(buffer,&flash[start_address],size);
   flashInUse = false;

   return OPERATION_SUCCESS;
}


/**********************************************************************
 *
 * Function:    flash_write_start
 *
 * Descriptor:  Write "size" bytes start from address "start_address".
 *              This function is non-blocking.
 *
 * Notes:       size = # bytes to write - must be > 0.
 *              The given buffer can be used immediately when the function returned.
 *
 * Return:      OPERATION_SUCCESS: Write request done successfully.
 *              INVALID_ARGUMENTS: One of the arguments is invalid.
 *              NOT_READY:         The device is not ready for a new request.
 *              NULL_POINTER:      One of the arguments points to NULL
 *
 ***********************************************************************/
result_t flash_write_start(uint16_t start_address, uint16_t size, const uint8_t buffer[])
{
   uint16_t i;

   if (flashInUse)
   {
      return NOT_READY;
   }

   flashInUse = true;
   for(i = 0 ; i < size ; ++i)
   {
      flash[start_address + i ] &= buffer[i];
   }
   flashInUse = false;

   return OPERATION_SUCCESS;
   
}


/**********************************************************************
 *
 * Function:    flash_write
 *
 * Descriptor:  Same as flash_write_start.
 *              This function is blocking.
 *
 * Notes:       size = # bytes to write - must be > 0.
 *              The given buffer can be used immediately when the function returned.
 *
 * Return:      OPERATION_SUCCESS: Write request done successfully.
 *              INVALID_ARGUMENTS: One of the arguments is invalid.
 *              NOT_READY:         The device is not ready for a new request.
 *              NULL_POINTER:      One of the arguments points to NULL
 *
 ***********************************************************************/
result_t flash_write(uint16_t start_address, uint16_t size, const uint8_t buffer[])
{
   uint16_t i;
   FILE* f;

   if (flashInUse)
   {
      return NOT_READY;
   }

   flashInUse = true;
   for(i = 0 ; i < size ; ++i)
   {
      flash[start_address + i ] &= buffer[i]; //nor flash can turn off bits only
   }
   f = fopen("d:\\development\\embsys\\ex4\\flash.img","r+");
   fseek(f,start_address,SEEK_SET);
   fwrite(buffer,1,size,f);
   fclose(f);
   flashInUse = false;

   return OPERATION_SUCCESS;
}


/**********************************************************************
 *
 * Function:    flash_bulk_erase_start
 *
 * Descriptor:  Erase all the content that find on the flash device.
 *              This function is non-blocking.
 *
 * Notes:
 *
 * Return:      OPERATION_SUCCESS: The request done successfully.
 *              NOT_READY:         The device is not ready for a new request.
 *
 ***********************************************************************/
result_t flash_bulk_erase_start(void)
{
   FILE* f;

   if (flashInUse)
   {
      return NOT_READY;
   }

   flashInUse = true;
   memset(flash,0xFF,FLASH_SIZE);
   f = fopen("d:\\development\\embsys\\ex4\\flash.img","r+");
   fwrite(flash,1,sizeof(flash),f);
   fclose(f);
   flashInUse = false;

   return OPERATION_SUCCESS;
}


/**********************************************************************
 *
 * Function:    flash_block_erase_start
 *
 * Descriptor:  Erase the content that find on the block that find on address
 *              "start_address".
 *              This function is non-blocking.
 *
 * Notes:
 *
 * Return:      OPERATION_SUCCESS: The request done successfully.
 *              NOT_READY:         The device is not ready for a new request.
 *
 ***********************************************************************/
result_t flash_block_erase_start(uint16_t start_address)
{
   FILE* f;

   if (flashInUse)
   {
      return NOT_READY;
   }

   flashInUse = true;
   start_address = (start_address/BLOCK_SIZE)*BLOCK_SIZE;
   memset(&flash[start_address],0xFF,BLOCK_SIZE);
   f = fopen("d:\\development\\embsys\\ex4\\flash.img","r+");
   fseek(f,start_address,SEEK_SET);
   fwrite(&flash[start_address],1,BLOCK_SIZE,f);
   fclose(f);
   flashInUse = false;

   return OPERATION_SUCCESS;
}
#include "embsys_sms_protocol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*

  Description:
    Fill the buffer with an SMS_PROBE / SMS_PROBE_ACK message fields

  Arguments:
    buf - a pointer to a buffer to fill / parse
    msg_fields - a pointer to a struct to get / put the fields of the message
    is_ack - a value other then NULL indicates this is an SMS_PROBE_ACK 
             and the appropriate fields in the struct are applicable
    len - the actual used size of the supllied buffer

  Return value:
  

*/
#define SWAP_BYTE_NIBBLE(b) (((b)&0xf)*0x10 + ((b)>>4))

/*unsigned fillDateAndTime(char *buf)
{

   struct tm *ttime;
   time_t tm = time(0);
   

   ttime = localtime(&tm);
   
   *buf++= SWAP_BYTE_NIBBLE((ttime->tm_year+1900)%100);
   *buf++= SWAP_BYTE_NIBBLE(ttime->tm_mon);
   *buf++= SWAP_BYTE_NIBBLE(ttime->tm_mday);
   *buf++= SWAP_BYTE_NIBBLE(ttime->tm_hour);
   *buf++= SWAP_BYTE_NIBBLE(ttime->tm_min);
   *buf++= SWAP_BYTE_NIBBLE(ttime->tm_sec);
   
   //our timezone GMT+2
   *buf++= SWAP_BYTE_NIBBLE(2*4);
   
   return 7;

}*/
unsigned deviceIdSize(char *pBuff)
{
   unsigned size = 0;
   while(size < ID_MAX_LENGTH && *pBuff != (char)NULL)
   {
      ++size;
   }

   return size;
}

unsigned fillNibbleSwappedStrNumericalData(char* buf,char *pData,unsigned size)
{
   unsigned i;
   for(i = 0 ; i < size ; i+=2)
   {
      if (i+1 == size)
      {
         *buf++ = (*pData++ - '0') + 0xF0;
      }
      else
      {
         *buf++ = (*pData++ - '0') + (*pData++ - '0')*0x10;      
      }
   }

   return i;
}



EMBSYS_STATUS embsys_fill_probe(char *buf, SMS_PROBE *msg_fields, char is_ack, unsigned *len)
{
   unsigned size;
   char *origBuf = buf;

   if (is_ack == (char)NULL)
   {
      *buf++ = 0x02;
   }
   else
   {
      *buf++ = 0x12;
   }
   *buf++ = size = deviceIdSize(msg_fields->device_id);
   *buf++ = 0xc9;
   buf += fillNibbleSwappedStrNumericalData(buf,msg_fields->device_id,size);

   *len = 3 + (size + size%2==0?0:1)/2;

   if (is_ack != (char)NULL)
   {
      //next 7 bits will contain timestamp
      buf += fillNibbleSwappedStrNumericalData(buf,msg_fields->timestamp,TIMESTAMP_MAX_LENGTH);

      *buf++ = size = deviceIdSize(msg_fields->sender_id);
      *buf++ = 0xc9;
      buf += fillNibbleSwappedStrNumericalData(buf,msg_fields->sender_id,size);

      *len += (7 + 2 + (size + size%2==0?0:1)/2);

   }

   return SUCCESS;
}

char bottomNBits(char byte,unsigned n)
{
   return ((1<<n)-1) & byte;
}

unsigned encodeAs7bits1(char* buf,char* text,unsigned size)
{
   unsigned borrow = 1,lastBorrow = 0,i;

   char* pOrigText = text;
   for(i = 0 ; i < size ; ++i)
   {

      *text++ = buf[i] >> lastBorrow | ((i+1)==size?0:(bottomNBits(buf[i+1],borrow)<<(8-borrow)));

      lastBorrow = borrow++;
      
      if (borrow == 8)
      {
         borrow = 1;
         lastBorrow = 0;
         ++i;
      }
   }

   return (unsigned)(text-pOrigText);


}

unsigned encodeAs7bits(char* buf,char* text,unsigned size)
{
   unsigned borrow = 1;
   int i;
   char* pOrigText = text;

   for(i = 0 ; i < size ; ++i)
   {
    
      *text++ = bottomNBits((buf[i] >> (borrow-1)),8-borrow) | ((i+1)==size?0:(bottomNBits(buf[i+1],borrow)<<(8-borrow)));

     
      borrow++;

      if (borrow == 8)
      {
         borrow = 1;
     
         ++i;
      }
   }

   return (unsigned)(text-pOrigText);


}

unsigned decode7bits(char* buf,char* text,unsigned size)
{
   /*unsigned borrow = 1,lastBorrow = 0,i;
   char temp1,temp2,temp3;

   for(i = 0 ; i < size ; ++i)
   {
      temp1 = bottomNBits(buf[i],8-borrow)<<lastBorrow;
      temp2 = ((lastBorrow==0)?0:(bottomNBits(buf[i-1]>>(8-lastBorrow),lastBorrow)));
      temp3 = temp1 | temp2;
      printf("%c",temp3);
      *text++ = bottomNBits(buf[i],8-borrow)<<(borrow-1) | ((i==0)?0:(buf[i-1]>>(8-borrow)));

      lastBorrow = borrow++;
      
   if (borrow == 8)
   {
      borrow = 1;
      lastBorrow = 0;
      --i;
    }

   }
   */

   unsigned borrow = 0,lastBorrow = 0,i=0,chars;
   char temp0,temp1,temp2,temp3;
   chars = 0;

   for(i = 0,chars = 0 ; chars < size ; ++i,++chars)
   {
      temp0 = bottomNBits(buf[i],7-borrow);
      temp1 = bottomNBits(buf[i],7-borrow)<<borrow;
      temp2 = ((i==0)?0:bottomNBits((buf[i-1]>>(8-borrow)),borrow));
      temp3 = temp1 | temp2;
      printf("%c",temp3);
      *text++ = (bottomNBits(buf[i],7-borrow)<<borrow) | ((i==0)?0:bottomNBits((buf[i-1]>>(8-borrow)),borrow));

      lastBorrow = borrow++;
      
      if (borrow == 8)
      {
         borrow = 0;
         lastBorrow = 0;
         --i;
       }

     

   }
   


   


   return 0;
}
/*

  Description:
    Fill the buffer with an SMS_SUBMIT message fields

  Arguments:
    buf - a pointer to a buffer to fill / parse
    msg_fields - a pointer to a struct to get / put the fields of the message
    len - the actual used size of the supllied buffer

  Return value:
  

*/
EMBSYS_STATUS embsys_fill_submit(char *buf, SMS_SUBMIT *msg_fields, unsigned *len)
{
   unsigned senderSize,recipientSize;
   *buf++ = 0x11;
   *buf++ = senderSize = deviceIdSize(msg_fields->device_id);
   *buf++ = 0xc9;
   buf+= fillNibbleSwappedStrNumericalData(buf,msg_fields->device_id,senderSize);
   
   *buf++ = msg_fields->msg_reference;

   *buf++ = recipientSize = deviceIdSize(msg_fields->recipient_id);
   *buf++ = 0xc9;
   buf+= fillNibbleSwappedStrNumericalData(buf,msg_fields->recipient_id,recipientSize);

   *buf++ = 0x0;
   *buf++ = 0x0;
   *buf++ = 0x3b;

   *buf++ = msg_fields->data_length;




   return SUCCESS;
}

/*

  Description:
    Parse the buffer as an SMS_SUBMIT_ACK message

  Arguments:
    buf - a pointer to a buffer to fill / parse
    msg_fields - a pointer to a struct to get / put the fields of the message

  Return value:
  

*/
EMBSYS_STATUS embsys_parse_submit_ack(char *buf, SMS_SUBMIT_ACK *msg_fields)
{
   
   return SUCCESS;
}


/*

  Description:
    Parse the buffer as an SMS_DELIVER message

  Arguments:
    buf - a pointer to a buffer to fill / parse
    msg_fields - a pointer to a struct to get / put the fields of the message

  Return value:
  

*/
EMBSYS_STATUS embsys_parse_deliver(char *buf, SMS_DELIVER *msg_fields)
{
   return SUCCESS;
}

int main()
{
   char buffer[100];
   char enc[100];

   SMS_PROBE p;
   char t = bottomNBits(115,4);

   memcpy(buffer,"hellohello",10);
   encodeAs7bits(buffer,enc,10);
   decode7bits(enc,buffer,10);

   memcpy(p.device_id,"12345678",9);



   return 0;
}
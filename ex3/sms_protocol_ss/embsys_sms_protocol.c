#include "embsys_sms_protocol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


#define SWAP_BYTE_NIBBLE(b) (((b)&0xf)*0x10 + ((b)>>4))

unsigned deviceIdSize(char *pBuff)
{
   unsigned size = 0;
   while(size < ID_MAX_LENGTH && *pBuff != (char)NULL)
   {
      ++size;
   }

   return size;
}

unsigned convStrToNibbleSwaped(char* str,char *swappedNibble,unsigned strSize)
{
   unsigned i;
   char* origSwappedNibble = swappedNibble;
   char n1,n2;
   for(i = 0 ; i < strSize ; i+=2)
   {
      if (i+1 == strSize)
      {
         *swappedNibble++ = (*str++ - '0') | 0xF0;
      }
      else
      {
         n1 =  (*str++ - '0');
         n2 = (*str++ - '0');
         
         *swappedNibble++ = (n2<<4) | n1;
      }
   }

   return (unsigned)(swappedNibble-origSwappedNibble);
}

unsigned convNibbleSwapedToString(char* nibble,char* str,unsigned strSize)
{
   unsigned i;
   char* origNibble = nibble;

   for(i = 0 ; i < strSize ; i+=2)
   {
      *str++ = (*nibble & 0xf) + '0';
      if (i+1 < strSize)
      {
         *str++ = ((*nibble >> 4) & 0xf) + '0';
      }
      nibble++;
   }

   return (unsigned)(nibble - origNibble);
}

char bottomNBits(char byte,unsigned n)
{
   return ((1<<n)-1) & byte;
}

/*
 * encoded 8bits text stream to 7 bits string
 * size - length of 8 bits string
 * return value - size of the encoded text in bytes
 */
unsigned encodeAs7bitStr(char* str,char* encodedStr,unsigned size)
{
   unsigned borrow = 1,i;
   
   char* pOrigText = encodedStr;

   for(i = 0 ; i < size ; ++i)
   {
    
      *encodedStr++ = bottomNBits((str[i] >> (borrow-1)),8-borrow) | ((i+1)==size?0:(bottomNBits(str[i+1],borrow)<<(8-borrow)));

     
      borrow++;

      if (borrow == 8)
      {
         borrow = 1;
     
         ++i;
      }
   }

   return (unsigned)(encodedStr-pOrigText);


}

/*
 * decode 7 bits string to 8 bits string
 * size - number of encoded chars
 * return value - the size of the encoded str in bytes
 *
 */
/*unsigned decodeTo8bitStr(char* encodedStr,char* str,unsigned size)
{
   

   unsigned borrow = 0,lastBorrow = 0,i=0,chars;

   chars = 0;

   for(i = 0,chars = 0 ; chars < size ; ++i,++chars)
   {
      
      *str++ = (bottomNBits(encodedStr[i],7-borrow)<<borrow) | ((i==0)?0:bottomNBits((str[i-1]>>(8-borrow)),borrow));

      borrow++;
      
      if (borrow == 8)
      {
         borrow = 0;
         --i;
      }
   }

   return i;
}
*/

unsigned decodeTo8bitStr(char* decodedStr,char* str,unsigned size)
{
   unsigned borrow = 0,i=0,chars=0;


   for(i = 0,chars = 0 ; chars < size ; ++i,++chars)
   {
     
      *str++ = (bottomNBits(decodedStr[i],7-borrow)<<borrow) | ((i==0)?0:bottomNBits((decodedStr[i-1]>>(8-borrow)),borrow));

      borrow++;
      
      if (borrow == 8)
      {
         borrow = 0;

         --i;
      }
   }
   return i;
}

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
   buf += convStrToNibbleSwaped(msg_fields->device_id,buf,size);


   *len = (3 + (size + (size%2==0?0:1))/2);

   if (is_ack != (char)NULL)
   {
      //next 7 bits will contain timestamp
      buf += convStrToNibbleSwaped(msg_fields->timestamp,buf,TIMESTAMP_MAX_LENGTH);

      *buf++ = size = deviceIdSize(msg_fields->sender_id);
      *buf++ = 0xc9;
      buf += convStrToNibbleSwaped(msg_fields->sender_id,buf,size);

      *len += (7 + 2 + (size + (size%2==0?0:1))/2);

   }

   return SUCCESS;
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
   char* origBuf = buf;

   //msg identifier
   *buf++ = 0x11;
   //length of the sender's number
   *buf++ = senderSize = deviceIdSize(msg_fields->device_id);
   //type of sender's address
   *buf++ = 0xc9;
   //sender's actual number
   buf+= convStrToNibbleSwaped(msg_fields->device_id,buf,senderSize);
   
   //message reference
   *buf++ = msg_fields->msg_reference;

   //recipient number's size
   *buf++ = recipientSize = deviceIdSize(msg_fields->recipient_id);
   //type of recipient's address
   *buf++ = 0xc9;
   //recipient actual number
   buf+= convStrToNibbleSwaped(msg_fields->recipient_id,buf,recipientSize);

   //protocol identifier
   *buf++ = 0x0;
   //data coding scheme
   *buf++ = 0x0;
   //validity period
   *buf++ = 0x3b;
   //user data len
   *buf++ = msg_fields->data_length;

   buf += encodeAs7bitStr(msg_fields->data,buf,msg_fields->data_length);

   *len = (unsigned)(buf - origBuf);
   
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
   unsigned numberLength;
   do
   {
      if ((unsigned char)*buf++ != 0x07)
      {
         break;
      }

      msg_fields->msg_reference = *buf++;
      numberLength = *buf++;

      if ((unsigned char)*buf++ != 0xc9)
      {
         break;
      }
      convNibbleSwapedToString(buf,msg_fields->recipient_id,numberLength);
      
      return SUCCESS;

   }while(0);

   return FAIL;
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
   unsigned numberLength;

   do
   {
      if ((unsigned char)*buf++ != 0x04)
      {
         break;
      }

      numberLength = *buf++;

      if ( (unsigned char)*buf++ != 0xc9)
      {
         break;
      }

      buf += convNibbleSwapedToString(buf,msg_fields->sender_id,numberLength);

      if (*buf++ != 0x0)
      {
         break;
      }

      if (*buf++ != 0x0)
      {
         break;
      }

      buf += convNibbleSwapedToString(buf,msg_fields->timestamp,TIMESTAMP_MAX_LENGTH);

      msg_fields->data_length = *buf++;

      decodeTo8bitStr(buf,msg_fields->data,msg_fields->data_length);

      return SUCCESS;
   }while(0);

   return FAIL;
   
}

void testSmsProbe()
{
   //make sure #define ID_MAX_LENGTH  = 9!!!
   char res[] = {0x02,0x09,0xc9,0x21,0x43,0x65,0x87,0xf9};

   char buf[100];
   unsigned size;
   SMS_PROBE m;

   memcpy(m.device_id,"123456789",9);

   embsys_fill_probe((char*)buf,&m,(char)NULL,&size);

   if (size == 8 && memcmp(res,buf,8) == 0)
   {
      printf("%s\n","testSmsProbe - passed");
   }
   else
   {
        printf("%s\n","testSmsProbe - failed");
   }
   //buff = 02 09 c9 21 43 65 87 f9
   //size = 8

   return;

}

void testSmsProbeAck()
{
   //make sure #define ID_MAX_LENGTH  = 9!!!
   char res[] = {0x12,0x09,0xc9,0x21,0x43,0x65,0x87,0xf9,0x99,0x30,0x92,0x51,0x61,0x95,0x80,0x09,0xc9,0x21,0x43,0x65,0x87,0xf9};

   char buf[100];
   unsigned size;
   SMS_PROBE m;

   memcpy(m.device_id,"123456789",9);
   memcpy(m.sender_id,"123456789",9);
   memcpy(m.timestamp,"99032915165908",14);

   embsys_fill_probe((char*)buf,&m,1,&size);

   if (size == 22 && memcmp(res,buf,22) == 0)
   {
      printf("%s\n","testSmsProbe - passed");
   }
   else
   {
        printf("%s\n","testSmsProbe - failed");
   }
}

void testSmsDeliver()
{
   char blob[] = {0x04,0x09,0xc9,0x21,0x43,0x65,0x87,0xf9,0x00,0x00,0x99,0x30,0x92,0x51,0x61,0x95,0x80,0x0a,0xe8,0x32,0x9b,0xfd,0x46,0x97,0xd9,0xec,0x37};
   SMS_DELIVER m;
   EMBSYS_STATUS s = embsys_parse_deliver(blob,&m);

   //verify fields
   return;
}

void testSmsSumbit()
{
   char blob1[] = {0x11,0x09,0xc9,0x21,0x43,0x65,0x87,0xf9,0x00,0x09,0xc9,0x21,0x43,0x65,0x87,0xf9,0x00,0x00,0x3b,0x0a,0xe8,0x32,0x9b,0xfd,0x46,0x97,0xd9,0xec,0x37};
   char blob2[29];
   SMS_SUBMIT m;
   unsigned len;
   EMBSYS_STATUS s;

   memcpy(m.data,"hellohello",10);
   m.data_length = 10;
   m.msg_reference = 0;
   memcpy(m.device_id,"123456789",9);
   memcpy(m.recipient_id,"123456789",9);
   
   
   s = embsys_fill_submit(blob2,&m,&len);

   //verify fields

   return;
}

void testSmsSubmitAck()
{
   char blob[] = {0x07,0x00,0x09,0xc9,0x21,0x43,0x65,0x87,0xf9};
   SMS_SUBMIT_ACK m;

   embsys_parse_submit_ack(blob,&m);

   return;
}
int main()
{
   /*char msg[100];
   char enc[100];
   char msg1[100];

   unsigned size;

   memcpy(msg,"hellohello",10);
   size = encodeAs7bitStr(msg,enc,10);
   size = decodeTo8bitStr(enc,msg1,10);
   */

   testSmsSubmitAck();
   /*
   char stream[] = {'a','b'};
   char num[9],num2[9];
   char buf[100];
   unsigned size;
   memcpy(num,"123456789",9);
   size = convStrToNibbleSwaped(num,buf,9);
   size = convNibbleSwapedToString(buf,num2,9);
   */



   return 0;
}
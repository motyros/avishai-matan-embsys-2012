#include "embsys_sms_protocol_mine.h"
#include <string.h>


#define SWAP_BYTE_NIBBLE(b) (((b)&0xf)*0x10 + ((b)>>4))

//calculate the length of the device id string
unsigned deviceIdSize(char *pBuff)
{
   unsigned size = 0;
   while(size < ID_MAX_LENGTH && pBuff[size] != (char)NULL)
   {
      ++size;
   }

   return size;
}

//converts string number to nibble swapped bytes representation  with trailing F's if needed
//return the number of bytes we used to represent this string
unsigned convStrToNibbleSwaped(char* str,char *swappedNibble,unsigned strSize)
{
   unsigned i;
   char* origSwappedNibble = swappedNibble;
   char n1,n2;
   for(i = 0 ; i < strSize ; i+=2)
   {
	  //if we have odd digits, add 'F' to the last number
      if (i+1 == strSize)
      {
         *swappedNibble++ = (*str++ - '0') | 0xF0;
      }
      else
      {
    	 //get the value of the next two digits
         n1 =  (*str++ - '0');
         n2 = (*str++ - '0');
         //place the values such that the second number will be placed at the 4 MSB
         *swappedNibble++ = (n2<<4) | n1;
      }
   }

   return (unsigned)(swappedNibble-origSwappedNibble);
}

//convert nibble swapped bytes representation to string with the number
unsigned convNibbleSwapedToString(char* nibble,char* str,unsigned strSize)
{
   unsigned i;
   char* origNibble = nibble;

   for(i = 0 ; i < strSize ; i+=2)
   {
	   //parse every byte into two digits (unless the last byte has F at the 4 MSB)
      *str++ = (*nibble & 0xf) + '0';
      if (i+1 < strSize)
      {
         *str++ = ((*nibble >> 4) & 0xf) + '0';
      }
      nibble++;
   }

   return (unsigned)(nibble - origNibble);
}

//this function return the number represents the n bits from the byte byte
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
   //borrow will indicate how many bits from the current byte we "save" for the next char to encide
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

//decode 7 bits string stream into regular 8 bits string stream
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

   return EMBSYS_SUCCESS;
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
   
   return EMBSYS_SUCCESS;
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
   memset(msg_fields,0,sizeof(SMS_SUBMIT_ACK));
   do
   {
	  //message identifier
      if ((unsigned char)*buf++ != 0x07)
      {
         break;
      }

      //message reference number
      msg_fields->msg_reference = *buf++;
      numberLength = *buf++;

      if ((unsigned char)*buf++ != 0xc9)
      {
         break;
      }
      convNibbleSwapedToString(buf,msg_fields->recipient_id,numberLength);
      
      return EMBSYS_SUCCESS;

   }while(0);

   return EMBSYS_FAIL;
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
   memset(msg_fields,0,sizeof(SMS_DELIVER));
   do
   {
	  //message identifier
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

      return EMBSYS_SUCCESS;
   }while(0);

   return EMBSYS_FAIL;
   
}


#include "7segments.h"
#include "defs.h"

//define all the hex digits
#define DIGIT_0 0x3f
#define DIGIT_1 0x06
#define DIGIT_2 0x5b
#define DIGIT_3 0x4f
#define DIGIT_4 0x66
#define DIGIT_5 0x6d
#define DIGIT_6 0x7d
#define DIGIT_7 0x07
#define DIGIT_8 0x7f
#define DIGIT_9 0x6f
#define DIGIT_A 0x77
#define DIGIT_B 0x7c
#define DIGIT_C 0x39
#define DIGIT_D 0x5e 
#define DIGIT_E 0x79
#define DIGIT_F 0x71

#define LEFT_7SEGMENT_BIT 0x80
#define RIGHT_7SEGMENT_BIT 0x00
#define SEGMENTS_ADDR 0x104

//array of all the hex digits
UINT8 digitsMasks[] = {DIGIT_0,DIGIT_1,DIGIT_2,DIGIT_3,DIGIT_4,DIGIT_5,DIGIT_6,DIGIT_7,DIGIT_8,DIGIT_9,DIGIT_A,DIGIT_B,DIGIT_C,DIGIT_D,DIGIT_E,DIGIT_F};

/*
 * Initilize the two 7-segments screen to "00"
 */
void segmentsInit()
{
	segmentsSetNumber(0);
}

/*
 * Sets the two 7-segments to the given number%100
 */
void segmentsSetNumber(const UINT8 number)
{
  //the 4 first digits
  const UINT8 rightDigit = digitsMasks[number & 0x0f];
  //the 4 last digits
  const UINT8 leftDigit = digitsMasks[(number >> 4) & 0x0f];

	//set the left digit display
	_sr(leftDigit | LEFT_7SEGMENT_BIT, SEGMENTS_ADDR) ;

	//set the right digit display
	_sr(rightDigit | RIGHT_7SEGMENT_BIT, SEGMENTS_ADDR) ;
}


#ifndef __DEFS_H__
#define __DEFS_H__

//booleans
#define TRUE 	1
#define FALSE 	0

//signed integers
#define INT8	char
#define INT16	short
#define INT32	int

//unsigned integers
#define UINT8	unsigned char
#define UINT16	unsigned short
#define UINT32 	unsigned int

//max unsigned 32-bit integer 
#define MAX_UINT32 0xFFFFFFFF

//return types
#define BOOL UINT32
#define STATUS UINT8

//return values
#define STATUS_SUCCESS 0
#define STATUS_ERROR 1
#define STATUS_NOT_A_DIGIT 2

//macro for binary number that only the n'th bit in on
#define BIT(n) (1<<n)

//assert for debug mode
#ifdef DBG
#define DBG_ASSERT(cond) if (!(cond)) _ASM("BRK_S");
#else
#define DBG_ASSERT(cond)
#endif

#endif


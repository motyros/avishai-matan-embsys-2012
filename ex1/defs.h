#ifndef __DEFS_H__
#define __DEFS_H__

#define TRUE 	1
#define FALSE 	0

#define INT8	char
#define INT16	short
#define INT32	int

#define UINT8	unsigned char
#define UINT16	unsigned short
#define UINT32 	unsigned int

#define MAX_UINT32 0xFFFFFFFF

#define BOOL UINT32
#define STATUS UINT8

#define STATUS_SUCCESS 0
#define STATUS_ERROR 1
#define STATUS_NOT_A_DIGIT 2


#define BIT(n) (1<<n)

#ifdef DBG
#define DBG_ASSERT(cond) if (!(cond)) _ASM("BRK_S");
#else
#define DBG_ASSERT(cond)
#end

#endif


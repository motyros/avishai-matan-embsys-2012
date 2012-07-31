#ifndef _OUR_COMMON_DEFS_H__
#define _OUR_COMMON_DEFS_H__

#ifndef NULL
#define NULL 0x0
#endif

#ifdef DBG
#define DBG_ASSERT(cond) if (!(cond)) _ASM("BRK_S"); _nop(); _nop();
#else
#define DBG_ASSERT(cond)
#endif

#endif
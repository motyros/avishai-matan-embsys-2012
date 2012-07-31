#ifndef _DBG_ASSERT_H__
#define _DBG_ASSERT_H__

#ifdef DBG
#define DBG_ASSERT(cond) if (!(cond)) _ASM("BRK_S"); _nop(); _nop();
#else
#define DBG_ASSERT(cond)
#endif

#endif
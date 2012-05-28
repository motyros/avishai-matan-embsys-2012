/*
 * common_defs.h
 *
 * Descriptor: Common definitions that are used by the drivers.
 *
 */

#ifndef COMMON_DEFS_H_
#define COMMON_DEFS_H_

#include <stdint.h>
#include <stdbool.h>

#ifndef NULL
#define NULL 0x0
#endif
// Return values of external functions.
typedef enum
{
    OPERATION_SUCCESS				= 0,
    NOT_READY					= 1,
    NULL_POINTER				= 2,
    INVALID_ARGUMENTS	        	        = 3,

    NETWORK_TRANSMIT_BUFFER_FULL	        = 4

} result_t;

#ifdef DBG
#define DBG_ASSERT(cond) if (!(cond)) _ASM("BRK_S"); _nop(); _nop();
#else
#define DBG_ASSERT(cond)
#endif

#endif /* COMMON_DEFS_H_ */

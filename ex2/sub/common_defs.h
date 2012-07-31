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

// Return values of external functions.
typedef enum
{
    OPERATION_SUCCESS				= 0,
    NOT_READY					= 1,
    NULL_POINTER				= 2,
    INVALID_ARGUMENTS	        	        = 3,

    NETWORK_TRANSMIT_BUFFER_FULL	        = 4

} result_t;


#endif /* COMMON_DEFS_H_ */

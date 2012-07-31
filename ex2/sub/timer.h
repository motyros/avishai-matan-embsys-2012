/*
 * timer.h
 *
 * Descriptor: API of the timers.
 *
 */

#ifndef TIMER_H_
#define TIMER_H_

#include "./common_defs.h"

/**********************************************************************
 *
 * Function:    timer0_register
 *
 * Descriptor:  Set expiration interval from now in milliseconds
 *
 * Notes:
 *
 * Return:      OPERATION_SUCCESS:      Timer request done successfully.
 *              NULL_POINTER:           One of the arguments points to NULL.
 *
 ***********************************************************************/
result_t timer0_register(uint32_t interval, bool one_shot, void(*timer_cb)(void));

/**********************************************************************
 *
 * Function:    timer1_register
 *
 * Descriptor:  Set expiration interval from now in milliseconds
 *
 * Notes:
 *
 * Return:      OPERATION_SUCCESS:      Timer request done successfully.
 *              NULL_POINTER:           One of the arguments points to NULL.
 *
 ***********************************************************************/
result_t timer1_register(uint32_t interval, bool one_shot, void(*timer_cb)(void));


#endif /* TIMER_H_ */

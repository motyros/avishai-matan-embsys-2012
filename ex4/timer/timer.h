/*
 * timer.h
 *
 * Descriptor: API of the timers.
 *
 */

#ifndef TIMER_H_
#define TIMER_H_

#include "../common_defs.h"

#define CPU_FREQ 50*1000000
#define CYCLES_IN_MS (CPU_FREQ/1000)

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

void timer_arm_tx_timer(uint32_t interval);

#endif /* TIMER_H_ */

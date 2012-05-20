#include "timer.h"
#include "../common_defs.h"

#define TIMER0_COUNT_ADDR	0x21
#define TIMER0_CONTROL_ADDR	0x22
#define TIMER0_LIMIT_ADDR	0x23

#define TIMER1_COUNT_ADDR	0x100
#define TIMER1_CONTROL_ADDR	0x101
#define TIMER1_LIMIT_ADDR	0x102

#define CPU_FREQ 50*1000000
#define CYCLES_IN_MS (CPU_FREQ/1000)

#define NUM_OF_TIMERS 2

uint32_t gCountRegAddr[] 	= 	{TIMER0_COUNT_ADDR,	TIMER1_COUNT_ADDR};
uint32_t gControlRegAddr[] 	= 	{TIMER0_CONTROL_ADDR,	TIMER1_CONTROL_ADDR};
uint32_t gLimitRegAddr[] 	= 	{TIMER0_LIMIT_ADDR,	TIMER1_LIMIT_ADDR};

typedef struct
{
	union
	{
		uint32_t data;
		struct
		{
			uint32_t IE 		: 1;
			uint32_t NH 		: 1;
			uint32_t W		: 1;
			uint32_t IP 		: 1;
			uint32_t reserve 	: 28;
		}bits;
	}control;
	
	void (*timerCB)();
	
	bool oneShot;
}Timer;

Timer gTimers[NUM_OF_TIMERS];

void handleTimerExpiration(uint32_t timerIndex)
{
	//disable interrupts if this is one shot timer
	if (gTimers[timerIndex].oneShot)
	{
		gTimers[timerIndex].control.bits.IE = 0;
	}
	
	//acknowledge the interrupt
	_sr(gTimers[timerIndex].control.data,gControlRegAddr[timerIndex]);

	//jump to the call back
	gTimers[timerIndex].timerCB();

}


_Interrupt1 void timer0ISR()
{
	handleTimerExpiration(0);
}

_Interrupt1 void timer1ISR()
{
	handleTimerExpiration(1);
}

/*
 * this function registers timer
 */
result_t registerTimer(	const uint32_t timerIndex,
		       	const uint32_t interval, 
			const bool oneShot, 
			void(*timerCB)(void))
{
	if (!timerCB)
	{
		return NULL_POINTER;
	}
	
	gTimers[timerIndex].timerCB = timerCB;
	gTimers[timerIndex].oneShot = oneShot;
	
	gTimers[timerIndex].control.data = 0;
	gTimers[timerIndex].control.bits.IE = 1;
	
	_sr(0,gCountRegAddr[timerIndex]);
	_sr(CYCLES_IN_MS * interval,gLimitRegAddr[timerIndex]);
	//1. we want the timer to generate interrupts (IE=1)
	//2. we want to timer to be advanced even every clock cycle (NH=0)
	//3. we don't want the Watchdog to be set (W=0)
	_sr(gTimers[timerIndex].control.data,gControlRegAddr[timerIndex]);

    return OPERATION_SUCCESS;
	
	
	
}

result_t timer0_register(uint32_t interval, bool one_shot, void(*timer_cb)(void))
{
	return registerTimer(0,interval,one_shot,timer_cb);
}


result_t timer1_register(uint32_t interval, bool one_shot, void(*timer_cb)(void))
{
	return registerTimer(1,interval,one_shot,timer_cb);
}

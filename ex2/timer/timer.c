#include "timer.h"

#define TIMER0_COUNT_ADDR	0x21
#define TIMER0_CONTROL_ADDR	0x22
#define TIMER0_LIMIT_ADDR	0x23

#define TIMER1_COUNT_ADDR	0x100
#define TIMER1_CONTROL_ADDR	0x101
#define TIMER1_LIMIT_ADDR	0x102

//check if this register need to be set manually or not.
#define TBR_ADDR			0x75

void (*gpTimer0CB)();
void (*gpTimer1CB)();

void timer0Isr()
{
	
}

void timer1Isr()
{
	
}

result_t timer0_register(uint32_t interval, bool one_shot, void(*timer_cb)(void))
{
	if (!timer_cb)
	{
		return NULL_POINTER;
	}
	
	gpTimer0CB = timer_cb;
	_sr(0,TIMER0_COUNT_ADDR);
	_sr(interval,TIMER0_LIMIT_ADDR);
	//1. we want the timer to generate interrupts
	//2. we want to timer to be advanced even every clock cycle
	//3. we don't want the Watchdog to be set
	_sr(1,TIMER0_CONTROL_ADDR);
}


result_t timer1_register(uint32_t interval, bool one_shot, void(*timer_cb)(void))
{
	if (!timer_cb)
	{
		return NULL_POINTER;
	}
	
	gpTimer1CB = timer_cb;
	_sr(0,TIMER1_COUNT_ADDR);
	_sr(interval,TIMER1_LIMIT_ADDR);
	//1. we want the timer to generate interrupts
	//2. we want to timer to be advanced even every clock cycle
	//3. we don't want the Watchdog to be set
	_sr(1,TIMER1_CONTROL_ADDR);
	
	
}
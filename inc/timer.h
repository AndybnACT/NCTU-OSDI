#ifndef TIMER_H
#define TIMER_H
void timer_init();
unsigned long get_tick();
void sched_yield(void);
void sys_do_sleep(uint32_t);
#endif

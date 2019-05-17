#ifndef TIMER_H
#define TIMER_H
void timer_init();
unsigned long sys_get_ticks();
void sys_do_sleep(uint32_t);
#endif

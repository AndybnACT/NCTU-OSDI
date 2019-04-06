/* Reference: http://www.osdever.net/bkerndev/Docs/pit.htm */
#include <kernel/trap.h>
#include <kernel/picirq.h>
#include <inc/mmu.h>
#include <inc/x86.h>

#define TIME_HZ 100

static unsigned long jiffies = 0;

void set_timer(int hz)
{
    int divisor = 1193180 / hz;       /* Calculate our divisor */
    outb(0x43, 0x36);             /* Set our command byte 0x36 */
    outb(0x40, divisor & 0xFF);   /* Set low byte of divisor */
    outb(0x40, divisor >> 8);     /* Set high byte of divisor */
}

/* 
 * Timer interrupt handler
 */
void timer_handler()
{
	jiffies++;
}

unsigned long get_tick()
{
	return jiffies;
}

void busy_wait(unsigned long sec){
    unsigned long cur = jiffies;
    unsigned long until = cur + sec*100;
    while (jiffies < until) { // !! overflow may happens
        // cprintf("Now tick = %d %d\n", get_tick(), until);
        // weird, if we do nothing here, busywait won't break
    }
}

void timer_init()
{
	set_timer(TIME_HZ);

	/* Enable interrupt */
	irq_setmask_8259A(irq_mask_8259A & ~(1<<IRQ_TIMER));
}


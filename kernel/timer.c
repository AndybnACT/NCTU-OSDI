/* Reference: http://www.osdever.net/bkerndev/Docs/pit.htm */
#include <kernel/trap.h>
#include <kernel/picirq.h>
#include <kernel/task.h>
#include <kernel/cpu.h>
#include <inc/mmu.h>
#include <inc/x86.h>
#include <kernel/timer.h>
#include <kernel/cpu.h>

#define TIME_HZ 100

static unsigned long jiffies[NCPU] = {0, };

void set_timer(int hz)
{
  int divisor = 1193180 / hz;       /* Calculate our divisor */
  outb(0x43, 0x36);             /* Set our command byte 0x36 */
  outb(0x40, divisor & 0xFF);   /* Set low byte of divisor */
  outb(0x40, divisor >> 8);     /* Set high byte of divisor */
}

/* It is timer interrupt handler */
//
// TODO: Lab6
// Modify your timer_handler to support Multi processor
// Don't forget to acknowledge the interrupt using lapic_eoi()
//
void timer_handler(struct Trapframe *tf)
{
    // extern void sched_yield();
    int i;

    jiffies[cpunum()]++;

    extern Task tasks[];
    lapic_eoi();
    if (cur_task != NULL)
    {
    /* TODO: Lab 5
    * 1. Maintain the status of slept tasks
    * 
    * 2. Change the state of the task if needed
    *
    * 3. Maintain the time quantum of the current task
    *
    * 4. sched_yield() if the time is up for current task
    *
    */
        for (i = 0; i < NR_TASKS; i++) { // should implement percpu sleep queue
            if (tasks[i].state == TASK_SLEEP) {
                tasks[i].remind_ticks--;
                if (!tasks[i].remind_ticks) {
                    tasks[i].state = TASK_RUNNABLE;
                    dispatch_cpu(&tasks[i], NULL);
                }
            }
        }
        
        if (--cur_task->remind_ticks <= 0) {
            cur_task->state = TASK_RUNNABLE;
            if (cur_task->parent_id == -1) {
                // first task should not be migrated 
                dispatch_cpu(cur_task, thiscpu);
            }else{
                dispatch_cpu(cur_task, NULL);// should lock
            }
            cur_task = NULL;
            // printk("timer\n");
            sched_yield();
        }
        // printk("timer return\n");
    }
}

unsigned long sys_get_ticks()
{
  return jiffies[cpunum()];
}
void timer_init()
{
  set_timer(TIME_HZ);

  /* Enable interrupt */
  irq_setmask_8259A(irq_mask_8259A & ~(1<<IRQ_TIMER));

  /* Register trap handler */
  extern void TIM_ISR();
  register_handler( IRQ_OFFSET + IRQ_TIMER, &timer_handler, &TIM_ISR, 0, 0);
}

void sys_do_sleep(uint32_t ticks) {
    cur_task->state = TASK_SLEEP;
    cur_task->remind_ticks = ticks;
    sched_yield();
}


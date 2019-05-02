#include <kernel/task.h>
#include <kernel/sched.h>
#include <inc/x86.h>
#include <kernel/mem.h>

#define ctx_switch(ts) \
  do { env_pop_tf(&((ts)->tf)); } while(0)

extern Task *cur_task;
int test;
/* TODO: Lab5
* Implement a simple round-robin scheduler (Start with the next one)
*
* 1. You have to remember the task you picked last time.
*
* 2. If the next task is in TASK_RUNNABLE state, choose
*    it.
*
* 3. After your choice set cur_task to the picked task
*    and set its state, remind_ticks, and change page
*    directory to its pgdir.
*
* 4. CONTEXT SWITCH, leverage the macro ctx_switch(ts)
*    Please make sure you understand the mechanism.
*/
void sched_yield(void)
{
    static int last_task = 0;
    int i, pick;
	extern Task tasks[];

    
    for (i = 1; i <= NR_TASKS; i++) {
        pick = (last_task + i)%NR_TASKS;
        if (tasks[pick].state == TASK_RUNNABLE){
            // if (i != NR_TASKS)
            //     printk("%d page addr of i=%lx\n", cur_task->task_id,page_lookup(cur_task->pgdir, &i, NULL));
            test = i;
            last_task = pick;
            // set up state, remind_ticks, pgdir 
            cur_task = &tasks[pick];
            cur_task->state = TASK_RUNNING;
            cur_task->remind_ticks = TIME_QUANT;
            // !!! IMPORTANT NOTE: must not use any local variable afrer lcr3 
            // !!! if we use private kernel stack.
            lcr3(PADDR(cur_task->pgdir));
            // printk("switch to %d\n", pick);
            // if (test != NR_TASKS)
            //     printk("%d page addr of i after cr3 loaded =%lx\n", cur_task->task_id, page_lookup(cur_task->pgdir, &i, NULL));
            // context switch
            ctx_switch(cur_task);
            // no return
        }
    }
    panic("No runnable task found!");
}

void sys_do_sleep(uint32_t ticks) {
    cur_task->state = TASK_SLEEP;
    cur_task->remind_ticks = ticks;
    sched_yield();
}
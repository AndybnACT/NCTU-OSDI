#include <kernel/task.h>
#include <kernel/cpu.h>
#include <kernel/timer.h>
#include <inc/x86.h>
#include <inc/string.h>

#define ctx_switch(ts) \
  do { env_pop_tf(&((ts)->tf)); } while(0)

// extern Task *cur_task;
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

//
// TODO: Lab6
// Modify your Round-robin scheduler to fit the multi-core
// You should:
//
// 1. Design your Runqueue structure first (in kernel/task.c)
//
// 2. modify sys_fork() in kernel/task.c ( we dispatch a task
//    to cpu runqueue only when we call fork system call )
//
// 3. modify sys_kill() in kernel/task.c ( we remove a task
//    from cpu runqueue when we call kill_self system call
//
// 4. modify your scheduler so that each cpu will do scheduling
//    with its runqueue
//    
//    (cpu can only schedule tasks which in its runqueue!!) 
//    (do not schedule idle task if there are still another process can run)	
//
void sched_yield(void)
{

    Runqueue *cur_queue = &thiscpu->cpu_rq;
    Task *next_task;
    
    next_task = rq_tsk_dequeue(cur_queue);
    if (cpunum() != 0) {
        // if there's a non-idle job on the non-boot cpu, switch to it.
        if (next_task->parent_id == -1 && cur_queue->rq_cnt > 0) {
            rq_tsk_enqueue(cur_queue, next_task);
            next_task = rq_tsk_dequeue(cur_queue);
        }
    }
    
    // should lock the tasks_lock here
    cur_task = next_task;
    cur_task->state = TASK_RUNNING;
    cur_task->remind_ticks = TIME_QUANT;
    // printk("[%d]cur_queue count = %d\n",cpunum(), cur_queue->rq_cnt);
    lcr3(PADDR(cur_task->pgdir));
    ctx_switch(cur_task);
    panic("No runnable task found!");
}



void dispatch_cpu(Task *t, struct CpuInfo *cpu){

    Runqueue *rq;
    if (cpu) {
        rq = &cpu->cpu_rq;
    }else{
        // pick a runqueue from cpus;
        rq = cpu_pick_rq();
    }
    // assign to the cpu;
    if (t->state != TASK_RUNNABLE)
        panic("task not runnable");
    rq_tsk_enqueue(rq, t);
    return;
}
Runqueue* cpu_pick_rq(void) {
    Runqueue *rq, *rq_pick;
    int i;
    rq = rq_pick = &cpus[0].cpu_rq;
    for (i = 1; i < ncpu; i++) {
        rq = &cpus[i].cpu_rq;
        if (rq->rq_cnt < rq_pick->rq_cnt)
            rq_pick = rq;
    }
    if (rq_pick->rq_cnt == MAXQ)
        panic("CPUs cannot accept new job");
    return rq_pick;
}

void tsk_queue_init(struct task_queue *q) {
    memset(q, 0, sizeof(struct task_queue));
    q->_st = 0;
    q->_ed = 0;
    q->_emptyq = 1;
    return;
}
int tskq_enqueue(struct task_queue *q, Task *t){
    int put = q->_ed;
    int nextput = (put + 1) % MAXQ;
    if (put == q->_st && !q->_emptyq) {
        panic("queue overflow!!");
    }else{
        q->task[put] = t;
        q->_ed = nextput;
        q->_emptyq = 0;
        return put;
    }
}
Task* tskq_dequeue(struct task_queue *q){
    Task *ret;
    int pop = (q->_st);
    int nextpop = (pop + 1) % MAXQ;
    if (pop == q->_ed && q->_emptyq) {
        panic("queue underflow!!");
    }else{
        ret = q->task[pop];
        q->task[pop] = NULL;
        q->_st = nextpop;
        if (nextpop == q->_ed)
            q->_emptyq = 1;
        return ret;
    }
}

void rq_init(Runqueue *rq) {
    // rq->last_task = first_tsk->task_id;
    rq->rq_cnt = 0;
    tsk_queue_init(&rq->rq_task);
    return;
}
void rq_tsk_enqueue(Runqueue *rq, Task * t){
    if (rq->rq_cnt++ >= MAXQ)
        panic("runqueue is full");
    tskq_enqueue(&rq->rq_task, t);
    t->cur_queue = rq;    
    return;
}
Task* rq_tsk_dequeue(Runqueue *rq){
    Task *ret;
    if (--rq->rq_cnt < 0)
        panic("already no job in runqueue");
    ret = tskq_dequeue(&rq->rq_task);
    ret->cur_queue = NULL;
    return ret;
}

#ifndef TASK_H
#define TASK_H

#include <inc/trap.h>
#include <kernel/mem.h>
// do not exceed MAXQ*ncpu
#define NR_TASKS	20
#define TIME_QUANT	100

typedef enum
{
	TASK_FREE = 0,
	TASK_RUNNABLE,
	TASK_RUNNING,
	TASK_SLEEP,
	TASK_STOP,
} TaskState;

// Each task's user space
#define USR_STACK_SIZE	(40960)


// TODO Lab6
// 
// Design your Runqueue structure for cpu
// your runqueue sould have at least two
// variables:
//
// 1. an index for Round-robin scheduling
//
// 2. a list indicate the tasks in the runqueue
//
#define MAXQ 32
typedef struct Task Task;

struct task_queue{
    Task *task[MAXQ];
    int _st;
    int _ed;
    int _emptyq;
};

typedef struct
{
    uint32_t rq_cnt;
    struct task_queue rq_task;
} Runqueue;

struct Task
{
    int cpuid;
	int task_id;
	int parent_id;
	struct Trapframe tf; //Saved registers
	int32_t remind_ticks;
	TaskState state;	//Task state
    pde_t *pgdir;  //Per process Page Directory
	Runqueue *cur_queue;
};




Runqueue* cpu_pick_rq(void);
void rq_init(Runqueue *rq);
void rq_tsk_enqueue(Runqueue *rq, Task * t);
Task* rq_tsk_dequeue(Runqueue *rq);

void task_init();
void task_init_percpu();
void env_pop_tf(struct Trapframe *tf);

/* TODO Lab 5
 * Interface for real implementation of kill and fork
 * Since their real implementation should be in kernel/task.c
 */
void sys_kill(int pid);
int sys_get_pid(void);
int sys_fork(void);

#endif

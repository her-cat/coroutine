#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "coroutine.h"

/* 协程调度函数原型 */
typedef void (*coroutine_schedule_function_t)(void);
/* 协程死锁函数原型 */
typedef void (*coroutine_dead_lock_function_t)(void);

/* 协程调度器结构体 */
typedef struct {
    coroutine_schedule_function_t schedule;
    coroutine_dead_lock_function_t dead_lock;
} coroutine_scheduler_t;

coroutine_t *coroutine_scheduler_run(coroutine_t *coroutine, const coroutine_scheduler_t *scheduler);

#endif

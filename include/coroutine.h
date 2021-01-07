#ifndef COROUTINE_H
#define COROUTINE_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ucontext.h>
#include "builtin.h"

#define COROUTINE_G(x)      GLOBALS_GET(coroutine, x)
#define coroutine_resume    COROUTINE_G(resume)

/* 协程栈大小 */
#define COROUTINE_STACK_ALIGNED_SIZE                (4 * 1024)
#define COROUTINE_MIN_STACK_SIZE                    (128 * 1024)
#define COROUTINE_RECOMMENDED_STACK_SIZE            (256 * 1024)
#define COROUTINE_MAX_STACK_SIZE                    (16 * 1024 * 1024)
#define COROUTINE_ALIGN_STACK_SIZE(size, alignment) (((size) + ((alignment) - 1LL)) & ~((alignment) - 1LL))

/* 协程id */
#define COROUTINE_MAX_ID    UINT64_MAX
#define COROUTINE_MAIN_ID   1

/* 协程状态枚举 */
typedef enum {
    COROUTINE_STATE_INIT,
    COROUTINE_STATE_READY,
    COROUTINE_STATE_RUNNING,
    COROUTINE_STATE_WAITING,
    COROUTINE_STATE_LOCKED,
    COROUTINE_STATE_FINISHED,
    COROUTINE_STATE_DEAD,
} coroutine_state_t;

/* 协程操作码 */
typedef enum {
    COROUTINE_OPCODE_NONE,
    COROUTINE_OPCODE_CHECKED,
    COROUTINE_OPCODE_WAIT,
} coroutine_opcode_t;

/* 协程回调函数原型 */
typedef void *(*coroutine_function_t)(void *);
/* 协程类型 */
typedef struct coroutine_s coroutine_t;
/* 协程上下文类型 */
typedef ucontext_t coroutine_context_t;

/* 协程数据结构 */
struct coroutine_s {
    union {
        coroutine_t *coroutine;
    } waiter;
    uint64_t id; /* 协程id */
    coroutine_state_t state; /* 协程状态 */
    coroutine_opcode_t opcodes; /* 协程操作码 */
    coroutine_t *from; /* 协程来源 */
    coroutine_t *previous; /* 前置协程 */
    void *stack; /* 自定义栈 */
    uint32_t stack_size; /* 栈大小 */
    coroutine_function_t function; /* 协程回调函数 */
    coroutine_context_t context; /* 协程上下文 */
    void *transfer_data; /* 传输数据 */
};

typedef bool_t (*coroutine_resume_t)(coroutine_t *coroutine, void *data, void **retval);

/* 协程全局状态 */
GLOBALS_STRUCT_BEGIN(coroutine)
    uint64_t last_id; /* 最后一个协程id */
    uint32_t count; /* 协程数量 */
    uint32_t default_stack_size; /* 默认栈大小 */
    /* coroutines */
    coroutine_t *current; /* 当前协程指针 */
    coroutine_t *main; /* 主协程指针 */
    coroutine_t _main; /* 主协程 */
    /* scheduler */
    coroutine_t *scheduler; /* 协程调度器 */
    /* functions */
    coroutine_resume_t resume; /* 协程恢复函数指针 */
GLOBALS_STRUCT_END(coroutine)

extern GLOBALS_DECLARE(coroutine)

bool_t coroutine_runtime_init(void);
uint32_t coroutine_set_default_stack_size(uint32_t size);
coroutine_resume_t coroutine_register_resume(coroutine_resume_t resume);
void coroutine_init(coroutine_t *coroutine);
coroutine_t *coroutine_create(coroutine_t *coroutine, coroutine_function_t function);
coroutine_t *coroutine_create_ex(coroutine_t *coroutine, coroutine_function_t function, uint32_t stack_size);
bool_t coroutine_resume_standard(coroutine_t *coroutine, void *data, void **retval);
bool_t coroutine_is_resumable(coroutine_t *coroutine);
bool_t coroutine_is_alive(coroutine_t *coroutine);
void *coroutine_jump(coroutine_t *coroutine, void *data);
void coroutine_close(coroutine_t *coroutine);
bool_t coroutine_yield(void *data, void **retval);

#endif

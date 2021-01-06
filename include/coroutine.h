#ifndef COROUTINE_H
#define COROUTINE_H

#include <stdint.h>
#include "builtin.h"

#define COROUTINE_G(x)  GLOBALS_GET(coroutine, x)

typedef enum {
    COROUTINE_STATE_INIT,
    COROUTINE_STATE_READY,
    COROUTINE_STATE_RUNNING,
    COROUTINE_STATE_FINISHED,
    COROUTINE_STATE_DEAD,
} coroutine_state_t;

typedef enum {
    COROUTINE_OPCODE_NONE,
    COROUTINE_OPCODE_CHECKED,
} coroutine_opcode_t;

typedef void *(*coroutine_function_t)(void *);

typedef struct coroutine_s coroutine_t;

struct coroutine_s {
    uint64_t id;
    coroutine_state_t state;
    coroutine_opcode_t opcodes;
    coroutine_t *from;
    coroutine_t *previous;
    void *stack;
    uint32_t stack_size;
    coroutine_function_t function;
    void *context;
    void *transfer_data;
};

typedef bool_t (*coroutine_resume_t)(coroutine_t *coroutine, void *data, void **retval);

GLOBALS_STRUCT_BEGIN(coroutine)
    uint64_t last_id;
    uint32_t count;
    uint32_t default_stack_size;
    /* coroutines */
    coroutine_t *current;
    coroutine_t *main;
    coroutine_t _main;
    /* scheduler */
    coroutine_t *scheduler;
    /* functions */
    coroutine_resume_t resume;

GLOBALS_STRUCT_END(coroutine)

extern GLOBALS_DECLARE(coroutine)

#endif

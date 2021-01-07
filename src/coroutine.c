#include "coroutine.h"

#define coroutine_context_make(ucontext, function, argc, ...) \
        makecontext(ucontext, function, argc, ##__VA_ARGS__)

#define coroutine_context_jump(current_ucontext, ucontext) \
        swapcontext(current_ucontext, ucontext)

GLOBALS_DECLARE(coroutine)

/* 协程运行时初始化 */
bool_t coroutine_runtime_init(void) {
    /* 注册协程恢复函数 */
    coroutine_register_resume(coroutine_resume_standard);

    /* 初始化协程 */
    coroutine_set_default_stack_size(COROUTINE_RECOMMENDED_STACK_SIZE);

    /* 初始化主协程 */
    coroutine_t *main_coroutine = &COROUTINE_G(_main);
    main_coroutine->id = COROUTINE_MAIN_ID;
    main_coroutine->state = COROUTINE_STATE_RUNNING;
    main_coroutine->opcodes = COROUTINE_OPCODE_NONE;
    main_coroutine->from = NULL;
    main_coroutine->previous = NULL;
    main_coroutine->stack = NULL;
    main_coroutine->stack_size = 0;
    main_coroutine->function = NULL;
    main_coroutine->transfer_data = NULL;
    memset(&main_coroutine->context, 0, sizeof(coroutine_context_t));

    /* 初始化协程全局变量 */
    COROUTINE_G(last_id) = main_coroutine->id + 1;
    COROUTINE_G(count) = 1;
    COROUTINE_G(current) = main_coroutine;
    COROUTINE_G(main) = main_coroutine;
    COROUTINE_G(scheduler) = NULL;

    return true;
}

/* 设置协程默认堆栈大小 */
uint32_t coroutine_set_default_stack_size(uint32_t size) {
    uint32_t origin_size = COROUTINE_G(default_stack_size);
    COROUTINE_G(default_stack_size) = size;
    return origin_size;
}

/* 注册恢复协程的函数，并返回原来的恢复函数 */
coroutine_resume_t coroutine_register_resume(coroutine_resume_t resume) {
    coroutine_resume_t origin_resume = coroutine_resume;
    coroutine_resume = resume;
    return origin_resume;
}

/* 对齐堆栈大小 */
static uint32_t coroutine_align_stack_size(uint32_t size) {
    if (size == 0) {
        return COROUTINE_G(default_stack_size);
    } else if (size < COROUTINE_MIN_STACK_SIZE) {
        return COROUTINE_MIN_STACK_SIZE;
    } else if (size > COROUTINE_MAX_STACK_SIZE) {
        return COROUTINE_MAX_STACK_SIZE;
    } else {
        return COROUTINE_ALIGN_STACK_SIZE(size, COROUTINE_STACK_ALIGNED_SIZE);
    }
}

/* 协程上下文回调函数 */
static void coroutine_context_function(void *data) {
    coroutine_t *coroutine = COROUTINE_G(current);

    /* 执行回调函数 */
    data = coroutine->function(coroutine->transfer_data);
    /* 更新协程状态 */
    coroutine->state = COROUTINE_STATE_FINISHED;
    /* 协程数量-1 */
    COROUTINE_G(count)--;

    printf("finished coroutine[%ld], count = %d\n", coroutine->id, COROUTINE_G(count));

    /* 让出控制权给前置协程 */
    coroutine_yield(data, NULL);
}

/* 初始化协程 */
void coroutine_init(coroutine_t *coroutine) {
    coroutine->state = COROUTINE_STATE_INIT;
}

/* 创建协程 */
coroutine_t *coroutine_create(coroutine_t *coroutine, coroutine_function_t function) {
    return coroutine_create_ex(coroutine, function, 0);
}

/* 创建协程（可指定堆栈大小） */
coroutine_t *coroutine_create_ex(coroutine_t *coroutine, coroutine_function_t function, uint32_t stack_size) {
    void *stack;
    uint32_t real_stack_size;
    coroutine_context_t context;

    /* 对齐堆栈大小 */
    stack_size = coroutine_align_stack_size(stack_size);
    /* 分配内存 */
    stack = (void *) malloc(stack_size);
    if (stack == NULL) {
        printf("alloc stack failed\n");
        return NULL;
    }

    real_stack_size = coroutine ? stack_size : (stack_size - sizeof(coroutine_t));
    if (coroutine == NULL) {
        coroutine = (coroutine_t *) (((char *)stack) + real_stack_size);
    }

    /* 将当前进程/线程的上下文信息保存到 context 中 */
    if (getcontext(&context) == -1) {
        free(stack);
        printf("getcontext failed\n");
        return NULL;
    }

    /* 设置堆栈 */
    context.uc_stack.ss_sp = stack;
    context.uc_stack.ss_size = real_stack_size;
    context.uc_stack.ss_flags = 0;
    context.uc_link = NULL;

    /* 将回调函数和自定义的堆栈填充到 context 中 */
    coroutine_context_make(&context, (void (*)(void)) coroutine_context_function, 1, NULL);

    /* 初始化协程信息 */
    coroutine->id = COROUTINE_G(last_id)++;
    coroutine->state = COROUTINE_STATE_READY;
    coroutine->opcodes = COROUTINE_OPCODE_NONE;
    coroutine->from = NULL;
    coroutine->previous = NULL;
    coroutine->stack = stack;
    coroutine->stack_size = real_stack_size;
    coroutine->function = function;
    coroutine->context = context;
    coroutine->transfer_data = NULL;

    printf("create coroutine[%ld] stack = %p, stack_size = %d, function = %p\n",
           coroutine->id, coroutine->stack, coroutine->stack_size, coroutine->function);

    return coroutine;
}

/* 恢复协程 */
bool_t coroutine_resume_standard(coroutine_t *coroutine, void *data, void **retval) {
    if (!(coroutine->opcodes & COROUTINE_OPCODE_CHECKED)) {
        if (!coroutine_is_resumable(coroutine)) {
            return false;
        }
    }

    data = coroutine_jump(coroutine, data);
    if (retval != NULL) {
        *retval = data;
    }

    return true;
}

/* 判断协程是否可恢复 */
bool_t coroutine_is_resumable(coroutine_t *coroutine) {
    coroutine_t *current_coroutine = COROUTINE_G(current);

    /* 如果 coroutine 是 current_coroutine 的前置协程 */
    if (coroutine == current_coroutine->previous) {
        return true;
    }

    /* 检查协程状态 */
    switch (coroutine->state) {
        case COROUTINE_STATE_READY:
        case COROUTINE_STATE_WAITING:
            break;
        case COROUTINE_STATE_RUNNING:
            printf("coroutine[%ld] is running\n", coroutine->id);
            return false;
        case COROUTINE_STATE_LOCKED:
            printf("coroutine[%ld] is locked\n", coroutine->id);
            return false;
        default:
            printf("coroutine[%ld] is not available\n", coroutine->id);
            return false;
    }

    /* current_coroutine 是否为 coroutine 等待的协程 */
    if ((coroutine->opcodes & COROUTINE_OPCODE_WAIT) && current_coroutine != coroutine->waiter.coroutine) {
        printf("coroutine[%ld] is waiting\n", coroutine->id);
        return false;
    }

    return true;
}

/* 协程是否存活 */
bool_t coroutine_is_alive(coroutine_t *coroutine) {
    return coroutine->state < COROUTINE_STATE_LOCKED && coroutine->state > COROUTINE_STATE_READY;
}

/* 协程跳转 */
void *coroutine_jump(coroutine_t *coroutine, void *data) {
    coroutine_t *current_coroutine = COROUTINE_G(current);

    printf("jump to coroutine[%ld]\n", coroutine->id);

    /* 记录 coroutine 是从哪个协程切换过来的 */
    coroutine->from = current_coroutine;
    if (current_coroutine->previous == coroutine) {
        /* 如果是 yield，则更新当前协程的状态为等待中 */
        if (current_coroutine->state == COROUTINE_STATE_RUNNING) {
            current_coroutine->state = COROUTINE_STATE_WAITING;
        }
        current_coroutine->previous = NULL;
    } else {
        /* current_coroutine 成为 coroutine 的前置协程 */
        coroutine->previous = current_coroutine;
    }

    /* 将 coroutine 设置为当前正在运行的协程 */
    COROUTINE_G(current) = coroutine;
    /* 更新协程状态 */
    coroutine->state = COROUTINE_STATE_RUNNING;
    /* 重置操作码 */
    coroutine->opcodes = COROUTINE_OPCODE_NONE;
    /* 设置传输数据 */
    coroutine->transfer_data = data;
    /* 开始跳转到 coroutine */
    coroutine_context_jump(&current_coroutine->context, &coroutine->context);
    data = current_coroutine->transfer_data;
    /* 处理来源 */
    coroutine = current_coroutine->from;

    /* 如果已完成则关闭协程 */
    if (coroutine->state == COROUTINE_STATE_FINISHED) {
        coroutine_close(coroutine);
    }

    return data;
}

/* 关闭协程 */
void coroutine_close(coroutine_t *coroutine) {
    void *stack = coroutine->stack;

    if (stack == NULL) {
        printf("coroutine[%ld] is unready or closed\n", coroutine->id);
        return;
    }

    if (coroutine_is_alive(coroutine)) {
        printf("coroutine[%ld] should not be active\n", coroutine->id);
        return;
    }

    coroutine->state = COROUTINE_STATE_DEAD;
    coroutine->stack = NULL;
    free(stack);
}

/* 让出协程 */
bool_t coroutine_yield(void *data, void **retval) {
    coroutine_t *coroutine = COROUTINE_G(current)->previous;

    if (coroutine == NULL) {
        printf("coroutine[%ld] yield failed\n", COROUTINE_G(current)->id);
        return false;
    }

    return coroutine_resume(coroutine, data, retval);
}

void *test_callback(void *data) {
    printf("hello, world\n");
    return data;
}

int main() {
    coroutine_runtime_init();

    coroutine_t *coroutine = coroutine_create(NULL, test_callback);
    coroutine_resume(coroutine, NULL, NULL);

    printf("hello, coroutine\n");
    return 0;
}

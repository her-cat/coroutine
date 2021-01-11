#include "scheduler.h"

/* 调度器的协程回调函数 */
static void *coroutine_scheduler_function(void *data) {
    coroutine_t *coroutine = COROUTINE_G(current);
    coroutine_scheduler_t *scheduler = (coroutine_scheduler_t *) data;

    /* 让当前协程成为调度器 */
    COROUTINE_G(scheduler) = coroutine;
    /* 活跃的协程数量排除调度器的协程 */
    COROUTINE_G(count)--;

    /* 让出控制权 */
    coroutine_yield(NULL, NULL);

    while (coroutine == coroutine_get_root()) {
        if (COROUTINE_G(count) > 0) {
            printf("Dead lock: all coroutines are asleep\n");
            while (usleep(60 * 1000 * 1000) == 0);
        }
        /* TODO: 通知等待的协程 */
    }

    /* 关闭协程调度器 */
    COROUTINE_G(scheduler) = NULL;
    /* 恢复活跃的协程数量，避免数据异常 */
    COROUTINE_G(count)++;

    return NULL;
}

/* 协程调度器运行入口 */
coroutine_t *coroutine_scheduler_run(coroutine_t *coroutine, const coroutine_scheduler_t *scheduler) {
    uint64_t last_id;
    coroutine_function_t function;

    /* 判断调度器是否已存在 */
    if (COROUTINE_G(scheduler) != NULL) {
        printf("scheduler already exists\n");
        return NULL;
    }

    /* 让调度器的协程 id 为 0 */
    last_id = COROUTINE_G(last_id);
    COROUTINE_G(last_id) = 0;
    /* 这里的 scheduler 实际上是传输的数据，而不是回调函数，在下面要被替换掉 */
    coroutine = coroutine_create(coroutine, (void *) scheduler);
    COROUTINE_G(last_id) = last_id;

    /* 取出上面传入的 scheduler */
    function = coroutine->function;
    /* 设置真正的协程回调函数 */
    coroutine->function = coroutine_scheduler_function;
    /* 尝试恢复协程，将 function（实际上是 scheduler） 作为数据 */
    coroutine_resume(coroutine, function, NULL);
    /* 断言协程还存活 */
    assert(coroutine_is_alive(coroutine));
    /* 断言已设置调度器 */
    assert(COROUTINE_G(scheduler) != NULL);
    /* 协程状态设置为运行中 */
    coroutine->state = COROUTINE_STATE_RUNNING;
    /* 让调度器的协程成为根协程的前置协程 */
    coroutine_get_root()->previous = coroutine;

    return coroutine;
}

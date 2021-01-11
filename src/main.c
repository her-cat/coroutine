#include "scheduler.h"

void *test_callback(void *data) {
    printf("coroutine[%ld] create\n", COROUTINE_G(current)->id);
    coroutine_yield(NULL, NULL);
    printf("coroutine[%ld] resumed\n", COROUTINE_G(current)->id);
    return data;
}

int main() {
    coroutine_runtime_init();
    coroutine_scheduler_run(NULL, NULL);

    printf("main coroutine\n");
    coroutine_t *coroutine = coroutine_run(NULL, test_callback, NULL);
    printf("main coroutine\n");
    coroutine_t *coroutine1 = coroutine_run(NULL, test_callback, NULL);

    printf("main coroutine\n");
    coroutine_resume(coroutine, NULL, NULL);
    printf("main coroutine\n");
    coroutine_resume(coroutine1, NULL, NULL);
    printf("main coroutine\n");

    coroutine_yield(NULL, NULL);

    return 0;
}

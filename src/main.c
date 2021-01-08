#include "scheduler.h"

void *test_callback(void *data) {
    printf("hello, world\n");
    return data;
}

int main() {
    coroutine_runtime_init();
    coroutine_scheduler_run(NULL, NULL);

    coroutine_t *coroutine = coroutine_create(NULL, test_callback);
    coroutine_resume(coroutine, NULL, NULL);

    printf("hello, coroutine\n");

    coroutine_yield(NULL, NULL);

    return 0;
}

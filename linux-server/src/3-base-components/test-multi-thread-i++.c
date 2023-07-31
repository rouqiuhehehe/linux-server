//
// Created by 115282 on 2023/7/28.
//

#include <pthread.h>
#include <stdio.h>

#define THREAD_NUM 4
#define THREAD_INCR_COUNT 100000
int atomicIncr (int *ptr, int step)
{
    int oldValue = *ptr;
    __asm__ volatile(
    "lock xadd %0, %1"
    : "=r"(oldValue), "=m"(*ptr)
    : "0"(step), "m"(*ptr)
    : "cc", "memory"
    );
    return *ptr;
}
void *threadFn (void *arg)
{
    int *count = (int *)arg;
    for (int i = 0; i < THREAD_INCR_COUNT; ++i)
        atomicIncr(count, 3);

    printf("%d\n", *count);
    return NULL;
}

int main ()
{
    setbuf(stdout, 0);
    int count = 0;
    pthread_t pthread[THREAD_NUM];
    for (int i = 0; i < THREAD_NUM; ++i)
        pthread_create(&pthread[i], NULL, threadFn, &count);
    for (int i = 0; i < THREAD_NUM; ++i)
        pthread_join(pthread[i], NULL);

    printf("count is : %d\n", count);
    return 0;
}
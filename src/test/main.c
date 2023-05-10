//
// Created by Administrator on 2023/3/28.
//
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
int main ()
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    void *stack;
    posix_memalign(&stack, getpagesize(), 128 * 1024);
    printf("%d\n", getpagesize());
    char a = 0;
    char b = 0;
    char c = 0;

    int aa = 1;
    int *bb = &aa;
    long *cc = &aa;
    return 0;
}
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>

#define SIZE 100

void f1 ()
{
    printf("f1 running --------\n");

    void *buffer[SIZE];
    int size = backtrace(buffer, SIZE);
    printf("return %d address in backtrace()\n", size);

    char **strings = backtrace_symbols(buffer, size);
    for (int i = 0; i < size; ++i)
        printf("%s\n", strings[i]);

    printf("f1 end ----------\n");

    free(strings);
}
void f2 ()
{
    f1();
}
void f3 ()
{
    f2();
}
void f4 ()
{
    f3();
}

void f5 ()
{
    f4();
}
void f6 ()
{
    f5();
}
void f7 ()
{
    f6();
}

int main ()
{
    f7();

    return 0;
}
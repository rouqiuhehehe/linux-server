//
// Created by Yoshiki on 2023/6/13.
//
#define _GNU_SOURCE
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <assert.h>
#include <dlfcn.h>
#include <errno.h>
// #define __USE_MACRO
#define __USE_HOOK

#define DIR_NAME "./memory"
#define FORMAT_OUT(buf, str, ...) sprintf(buf, "%s:%s::%ud\tcaller: %p, "str"%c", filename, \
                                            functionName, line, __builtin_return_address(0), ##__VA_ARGS__, '\0')

#if defined(__USE_MACRO)
void *__malloc (size_t size, const char *filename, const char *functionName, unsigned line)
{
    if (access(DIR_NAME, F_OK))
    {
        assert(!mkdir(DIR_NAME, 0777));
    }

    void *ptr = malloc(size);
    char memName[128] = {};
    sprintf(memName, DIR_NAME"/%p.mem", ptr);

    FILE *file = fopen(memName, "w");
    assert(file);
    void *caller = __builtin_return_address(0);

    fprintf(file, "%s:%s::%ud\tcaller: %p, addr: %p\n", filename, functionName, line, caller, ptr);
    fflush(file);
    fclose(file);

    return ptr;
}
void __free (void *ptr, const char *filename, const char *functionName, unsigned line)
{
    char memName[128] = {};
    sprintf(memName, DIR_NAME"/%p.mem", ptr);

    if (unlink(memName) < 0)
    {
        if (errno == ENOENT)
        {
            FORMAT_OUT(memName, "double free");
            perror(memName);
            return;
        }
        else
        {
            FORMAT_OUT(memName, "unlink error");
            perror(memName);
        }
    }

    free(ptr);
}

#define malloc(size) __malloc(size, __FILE__, __FUNCTION__, __LINE__)
#define free(ptr) __free(ptr, __FILE__, __FUNCTION__, __LINE__)

#elif defined(__USE_HOOK)

typedef void *(*malloc_t) (size_t size);
typedef void (*free_t) (void *ptr);

malloc_t malloc_f = NULL;
free_t free_f = NULL;

char enableMallocHook = 1;
char enableFreeHook = 1;

void initHook ()
{
    if (!malloc_f)
        malloc_f = dlsym(RTLD_NEXT, "malloc");

    if (!free_f)
        free_f = dlsym(RTLD_NEXT, "free");
}

void *malloc (size_t size)
{
    initHook();
    void *ptr;
    if (enableMallocHook)
    {
        enableMallocHook = 0;
        if (access(DIR_NAME, F_OK))
        {
            assert(!mkdir(DIR_NAME, 0777));
        }

        ptr = malloc_f(size);
        char memName[128] = {};
        sprintf(memName, DIR_NAME"/%p.mem", ptr);

        FILE *file = fopen(memName, "w");
        assert(file);
        void *caller = __builtin_return_address(0);

        fprintf(
            file,
            "caller: %p, addr: %p\n",
            caller,
            ptr
        );
        fflush(file);
        enableFreeHook = 0;
        fclose(file);
        enableFreeHook = 1;
        enableMallocHook = 1;
    }
    else
        ptr = malloc_f(size);
    return ptr;
}

void free (void *ptr)
{
    initHook();
    if (enableFreeHook)
    {
        enableFreeHook = 0;
        char memName[128] = {};
        sprintf(memName, DIR_NAME"/%p.mem", ptr);

        if (unlink(memName) < 0)
        {
            if (errno == ENOENT)
            {
                sprintf(
                    memName,
                    "caller: %p, addr: %p\t double free",
                    __builtin_return_address(0),
                    ptr
                );
                perror(memName);
                return;
            }
            else
            {
                sprintf(
                    memName,
                    "caller: %p, addr: %p\t unlink error",
                    __builtin_return_address(0),
                    ptr
                );
                perror(memName);
            }
        }
        enableFreeHook = 1;
    }

    free_f(ptr);
}

#elif defined(__USE_SYSTEM)
#endif

int main ()
{
    void *a = malloc(16);
    void *b = malloc(16);
    void *c = malloc(16);

    free(b);
    free(c);

    return 0;
}
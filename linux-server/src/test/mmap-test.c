//
// Created by Yoshiki on 2023/7/26.
//

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#define FILE_SIZE 1024
#define FILE_NAME "./test.ccc"
#define MESSAGE "懵懂少年佛南施工的时空观没课么改日给你\n"

int main ()
{
    int fd = open(FILE_NAME, O_RDWR | O_CREAT | O_TRUNC, 0664);

    struct stat fileStat;
    stat(FILE_NAME, &fileStat);

    if (fileStat.st_size < FILE_SIZE)
        truncate(FILE_NAME, FILE_SIZE);

    char *fileData = mmap(NULL, FILE_SIZE, PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);

    memcpy(fileData, MESSAGE, sizeof(MESSAGE));

    munmap(fileData, FILE_SIZE);

    return 0;
}
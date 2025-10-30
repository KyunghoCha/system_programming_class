//
// Created by gcha792 on 10/30/25.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#define MAX_LINE 100

int main(int argc, char *argv[]) {
    int fd[2];
    pipe(fd);
    if (fork() == 0) {
        close(fd[0]);
        char msg[MAX_LINE];
        sprintf(msg, "Hello from child with PID%d", getpid());
        write(fd[1], msg, strlen(msg) + 1);
        close(fd[1]);
    } else {
        close(fd[1]);
        char line[MAX_LINE];
        read(fd[0], line, MAX_LINE);
        printf("Parent PID %d receives message from child : %s\n", getpid(), line);
        wait(NULL);
    }

    return 0;
}
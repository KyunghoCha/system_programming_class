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

        dup2(fd[1], STDOUT_FILENO);
        close(fd[1]);

        printf("Hello, pipe to screen\n");
        printf("Bye! pipe tp screen\n");

        exit(0);
    } else {
        close(fd[1]);
        char line[MAX_LINE];
        ssize_t n;
        while ((n = read(fd[0], line, MAX_LINE)) > 0) {
            write(STDOUT_FILENO, line, n);
        }
        close(fd[0]);
        wait(NULL);
    }

    return 0;
}
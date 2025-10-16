//
// Created by gcha792 on 9/26/25.
//

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    int newmode;
    newmode = (int)strtoul(argv[1], (char **)NULL, 8);
    if (chmod(argv[2], newmode) == -1) {
        perror(argv[2]);
        exit(1);
    }

    exit(0);
}
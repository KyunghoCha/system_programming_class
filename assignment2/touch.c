//
// Created by gcha792 on 9/26/25.
//
#include <utime.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "사용법: touch file1 \n");
        exit(-1);
    }
    utime(argv[1], NULL);
}
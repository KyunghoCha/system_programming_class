//
// Created by gcha792 on 9/26/25.
//
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
    if (link(argv[1], argv[2]) == -1) exit(1);
    exit(0);
}
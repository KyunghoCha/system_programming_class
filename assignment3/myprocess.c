//
// Created by gcha792 on 10/2/25.
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char* argv[]) {
    int pid;

    // 1단계: 기본 정보 출력
    printf("\n=== 1단계: 프로세스 정보 ===\n");
    printf("나의 PID: %d\n", getpid());
    printf("부모 PID: %d\n", getppid());
    printf("사용자 ID: %d\n", getuid());
    // TODO: 실행예시와 같이 프로세스 정보 출력 코드 작성

    // 2단계: 자식 프로세스 생성
    printf("\n=== 2단계: 자식 프로세스 생성 ===\n");
    // TODO: 자식 프로세스 생성 코드 작성
    pid = fork();
    if (pid == 0) {
        // TODO: 자식 프로세스 코드 작성
        printf("자식 프로세스 입니다. PID=%d\n", getpid());
    } else {
        // TODO: 부모 프로세스 코드 작성
        printf("부모 프로세스 입니다. PID=%d\n", getpid());
        wait(NULL);
        printf("자식 프로세스가 종료되었습니다.\n");
    }

    // 3단계는 부모 프로세스만 실행
    if (pid != 0) {
        printf("\n=== 3단계: 명령어 실행 ===\n");
        // TODO: 자식 프로세스 생성 코드 작성
        pid = fork();
        if (pid == 0) {
            // TODO: execl()을 사용하여 "ls -l" 실행
            execl("/bin/ls", "ls", "-l", NULL);
        } else {
            // TODO: wait() 호출
            wait(NULL);
            printf("명령어 실행이 완료되었습니다.\n");
        }
    }

    return 0;
}
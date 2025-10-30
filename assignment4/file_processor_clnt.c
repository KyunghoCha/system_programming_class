//
// Created by gcha792 on 10/30/25.
//

// 1. C 표준 라이브러리 (알파벳 순서 권장)
#include <stdarg.h>    // 가변 인자 처리 (handle_error 함수용)
#include <stdio.h>     // 표준 입출력 (printf, perror 등)
#include <stdlib.h>    // 일반 유틸리티 (malloc, free, exit 등)
#include <stdint.h>    // uint32_t, size_t 등
#include <string.h>    // 문자열 처리 (strlen, memcpy 등)
#include <errno.h>     // EINTR 커널 인터럽트 처리
#include <arpa/inet.h> // htonl, ntohl 등

// 2. POSIX/UNIX 시스템 인터페이스
#include <unistd.h>   // UNIX 표준 함수 (read, write, close)
#include <fcntl.h>    // 파일 제어 (open 플래그, fcntl)

#define MODE_NUM 4        // 모드 갯수
#define BUFFER_SIZE 8192  // 읽기, 쓰기 버퍼 크기

// 통신 프로토콜 헤더
typedef struct {
    int total_lines;
} MessageHeader, *pMessageHeader;

// 통계 구조체
typedef struct {
    char *mode;
    int total_lines;
    int times; // TODO 이름 변경
} Stats, *pStats;

int resolve_mode(const char *);                // 모드 옵션 처리
ssize_t read_all(int, void *, size_t);         // 읽기 무결성 보장
ssize_t write_all(int, const void *, size_t);  // 쓰기 무결성 보장
int send_message(int, const MessageHeader *, const char *, size_t);  // 서버로 데이터 전송
char *receive_message(int, const MessageHeader *, size_t *);         // 서버에서 받은 데이터 읽기
char *read_line(int, size_t *);        // 한번에 읽은 파일 줄로 분리 및 처리
void print_stats(const Stats *);       // 통계 출력
void handle_error(const char *, ...);  // 에러 처리

int main(int argc, char *argv[]) {
    // 사용자 입력 인자 갯수 확인
    if (argc != 3) handle_error("Usage: file_processor_clnt <input_file> <mode>\n");

    int fd_input = open(argv[1], O_RDONLY);              // 사용자 지정 파일 열기
    int fd_c2s = open("./fifo/fifo_c2s", O_WRONLY);  // fifo_c2s 파일 쓰기 전용 열기
    int fd_s2c = open("./fifo/fifo_s2c", O_RDONLY);  // fifo_s2c 파일 읽기 전용 열기
    if (fd_input == -1 || fd_c2s == -1 || fd_s2c == -1)       // input, fifo 파일 열기 실패 예외 처리
        handle_error("Fail to open FIFO.\n");

    char write_buf[BUFFER_SIZE];  // 송신 버퍼
    char line_buf[BUFFER_SIZE];   // 한 줄 처리용 송신 임시 버퍼
    char read_buf[BUFFER_SIZE];   // 수신 버퍼

    int curr_mode = resolve_mode(argv[2]);  // 현재 모드 처리
    snprintf(write_buf, sizeof(curr_mode), "%d\n", curr_mode);  // 현재 모드 쓰기 버퍼에 저장

    if (write(fd_c2s, write_buf, strlen(write_buf)) == -1)  // 현재 모드 서버로 전송
        handle_error("Fail to write to FIFO.\n");         // 전송 실패 예외처리

    do {
        // Input file의 내용을 한 줄씩 읽기
        ssize_t byte_read = read(fd_input, write_buf, BUFFER_SIZE);
        if (byte_read == -1) handle_error("Fail to read from Input File.\n");

        // 읽은 내용을 fifo_c2s 파일에 작성
        ssize_t bytes_write = write(fd_c2s, write_buf, sizeof(write_buf));  // 읽은 파일의 내용 한 줄씩 서버에 보내기
        if (bytes_write == -1) handle_error("Fail to write to FIFO.\n");  // 예외 처리

        // 서버 프로세스에서 처리된 fifo_s2c 파일 읽기
        ssize_t bytes_written = read(fd_s2c, read_buf, BUFFER_SIZE);       // 서버의 결과 읽기
        if (bytes_written == -1) handle_error("Fail to read from FIFO.\n");  // 예외 처리
        read_buf[bytes_written] = '\0';      // 레코드 끝에 null 문자를 추가해 문자열 비교 함수 보장
    } while (strcmp(read_buf, "END") != 0);  // 파일을 다 보내거나 END가 오기 전까지 계속 반복

    return 0;
}

// 모드 옵션 처리
int resolve_mode(const char *mode) {
    int curr_mode = MODE_NUM;
    char *mode_name[] = { "count", "upper", "lower", "reverse" };

    for (int i = 0; i < MODE_NUM; i++)
        if (strcmp(mode, mode_name[i]) == 0) {
            curr_mode = i;
            break;
        }

    if (curr_mode == MODE_NUM)
        handle_error("Error: Invalid mode '%s'. Valid modes are: count, upper, lower, reverse.\n", mode);

    return curr_mode;
}

ssize_t read_all(int fd, void *buf, size_t count) {
    size_t bytes_left = count;    // 읽을 바이트
    ssize_t current_read = 0;     // 현재 읽은 바이트
    char *buf_ptr = (char *)buf;  // 파일 포인터 위치

    while (bytes_left > 0) {  // 다 읽을때 까지 반복
        if ((current_read = read(fd, buf_ptr, bytes_left)) == -1) {  // 파일 읽기
            if (errno == EINTR) continue;  // 커널 인터럽트시 재시도

            return -1;  // 이외 심각한 오류
        }

        if (current_read == 0) break;  // 파일을 다 읽었다면

        bytes_left -= current_read;  // 남은 읽을 바이트
        buf_ptr += current_read;     // 파일 포인터 이동
    }

    return (ssize_t)(count - bytes_left);  // 읽은 바이트 반환
}

ssize_t write_all(int fd, const void *buf, size_t count) {
    size_t bytes_left = count;    // 쓸 바이트
    ssize_t current_write = 0;    // 현재 쓴 바이트
    char *buf_ptr = (char *)buf;  // 파일 포인터 위치

    while (bytes_left > 0) {  // 다 쓸때까지 반복
        if ((current_write = write(fd, buf_ptr, bytes_left)) == -1) {  // 파일 쓰기
            if (errno == EINTR) continue;  // 커널 인터럽스시 재시도

            return -1;  // 이외 심각한 오류
        }

        bytes_left -= current_write;  // 남은 쓸 바이트
        buf_ptr += current_write;     // 파일 포인터 이동
    }

    return (ssize_t)(count - bytes_left);  // 쓴 바이트 반환
}

int send_message(int fd, const MessageHeader *header, const char *data, size_t len) {
    uint32_t net_length = htonl(header->total_lines);
    if (write_all(fd, &net_length, sizeof(net_length)) == -1) return -1;
    if (write_all(fd, (char *)data, len) == -1) return -1;
    return 0;
}

char *receive_message(int fd, const MessageHeader *header, size_t *out_len) {

}

char *read_line(int fd, size_t *out_len) {

}

void print_stats(const Stats *stats) {

}

// 에러 처리
void handle_error(const char *fmt, ...) {  // 가변인자로 포메팅 지원
    va_list args;
    va_start(args, fmt);

    // stderr로 출력
    vfprintf(stderr, fmt, args);

    va_end(args);
    exit(EXIT_FAILURE);
}
//
// Created by gcha792 on 10/30/25.
//

// 1. C 표준 라이브러리
#include <stdarg.h>  // 가변 인자 처리 (handle_error 함수용)
#include <stdio.h>   // 표준 입출력 (printf, perror 등)
#include <stdlib.h>  // 일반 유틸리티 (malloc, free, exit 등)
#include <stdint.h>  // uint32_t, size_t 등
#include <string.h>  // 문자열 처리 (strlen, memcpy 등)
#include <errno.h>   // EINTR 커널 인터럽트 처리
#include <time.h>    // clock_gettime() 시간 측정용

// 2. POSIX/UNIX 시스템 인터페이스
#include <arpa/inet.h>  // htonl, ntohl 등
#include <unistd.h>     // UNIX 표준 함수 (read, write, close)
#include <fcntl.h>      // 파일 제어 (open 플래그, fcntl)

#define BUFFER_SIZE       4096  // 읽기, 쓰기 버퍼 크기
#define READ_LINE_CLEANUP -1    // read_line 클린업 모드

// 프로토콜 헤더 구조체
typedef struct {
    uint32_t line_bytes;  // 보내고 받을 줄의 크기
} MessageHeader, *pMessageHeader;

// 통계 구조체
typedef struct {
    char *mode;               // 처리 모드
    size_t total_proc_lines;  // 총 처리한 줄의 수
    double elapsed_time;      // 처리 시간
} Stats, *pStats;

// 파일 구조체
typedef struct {
    int fd;             // 파일 디스크립터
    char *stash_buf;    // 임시 저장 버퍼
    size_t stash_len;   // 입시 저장 버퍼 길이
    size_t stash_size;  // 임시 저장 버퍼 메모리 크기
} rn_fd_t, *p_rn_fd_t;

// 모드 열거형
enum Mode {
    COUNT,    // 글자 및 단어 개수 세기
    UPPER,    // 대문자로 변환
    LOWER,    // 소문자로 변환
    REVERSE,  // 반대로 출력
    MODE_NUM  // 모드 갯수
};

uint32_t resolve_mode(const char *);                             // 모드 옵션 처리
ssize_t read_all(int, void *, size_t);                           // 읽기 무결성 보장
ssize_t write_all(int, const void *, size_t);                    // 쓰기 무결성 보장
ssize_t send_message(int, const MessageHeader *, const void *);  // 서버 데이터 송신
ssize_t receive_message(int, MessageHeader *, void **);          // 서버 데이터 수신
char *read_line(int, size_t *);                                  // 한번에 읽은 파일 줄로 분리 및 처리
void print_stats(const Stats *);                                 // 통계 출력
void handle_error(const char *, ...);                            // 에러 처리

int main(int argc, char *argv[]) {
    // 사용자 입력 인자 갯수 확인
    if (argc != 3) handle_error("Usage: file_processor_clnt <input_file> <mode>\n");

    int fd_input = open(argv[1], O_RDONLY);                 // 사용자 지정 파일 열기
    int fd_c2s = open("/tmp/fifo/fifo_c2s", O_WRONLY);  // fifo_c2s 파일 쓰기 전용 열기
    int fd_s2c = open("/tmp/fifo/fifo_s2c", O_RDONLY);  // fifo_s2c 파일 읽기 전용 열기
    if (fd_input == -1) handle_error("Fail to open input file: %s\n", argv[1]);
    if (fd_c2s == -1) handle_error("Fail to open FIFO c2s\n");
    if (fd_s2c == -1) handle_error("Fail to open FIFO s2c\n");

    // 시간 측정 구조체
    struct timespec start_time, end_time;
    MessageHeader header = { .line_bytes=0 };
    Stats stats = { .mode=NULL, .total_proc_lines=0, .elapsed_time=0.0 };

    uint32_t mode = MODE_NUM;  // 모드를 초기화
    char *line = NULL;         // 한 줄 처리용 송신 임시 버퍼
    char *read_buf = NULL;     // 수신 버퍼
    if ((read_buf = (char *)malloc(1)) == NULL) handle_error("Fail to allocate memory.\n");

    if ((mode = resolve_mode(argv[2])) == MODE_NUM)  // 현재 모드 처리
        handle_error("Error: Invalid mode '%s'. Valid modes are: count, upper, lower, reverse.\n", argv[2]);

    header.line_bytes = sizeof(mode);
    if (send_message(fd_c2s, &header, &mode) == -1)  // 서버에 처리 모드 전송
        handle_error("Fail to send mode: %s\n", argv[2]);

    size_t len = 0;
    clock_gettime(CLOCK_MONOTONIC, &start_time);  // 시간 측정 시작
    while ((line = read_line(fd_input, &len)) != NULL) {
        stats.total_proc_lines++;  // 총 처리 줄 수 증가

        // 서버에 헤더 및 데이터 송신
        header.line_bytes = len;  // 줄의 길이 헤더에 저장
        if (send_message(fd_c2s, &header, line) == -1) handle_error("Fail to send message: %s\n", line);
        printf("%lu번째 줄 전송...", stats.total_proc_lines);  // 처리 과정 출력

        // 서버에서 헤더 및 데이터 수신
        if (receive_message(fd_s2c, &header, (void **)&read_buf) == -1) handle_error("Fail to receive message\n");

        //정보 출력
        printf("%lu번째 줄 결과 수신: %s\n", stats.total_proc_lines, read_buf);

        // line 메모리 해제로 메모리 누수 방지
        free(line);
        line = NULL;
    }

    // 프로세스 종료 END 메시지 전송
    header.line_bytes = strlen("END");
    if (send_message(fd_c2s, &header, "END") == -1) handle_error("Fail to send message: %s\n", "END");
    clock_gettime(CLOCK_MONOTONIC, &end_time);  // 시간 측정 종료

    // 처리 모드 및 처리 시간 저장
    stats.mode = argv[2];
    stats.elapsed_time = (end_time.tv_sec - start_time.tv_sec) * 1.0;     // 초 단위
    stats.elapsed_time += (end_time.tv_nsec - start_time.tv_nsec) / 1e9;  // 나노초 -> 초

    // 통계 출력
    printf("=== 처리 통계 ===\n");
    printf("처리 모드: %s\n", stats.mode);
    printf("처리한 줄 수: %lu\n", stats.total_proc_lines);
    printf("소요 시간: %.2lf초\n", stats.elapsed_time);

    // read_line의 메모리 정리
    read_line(READ_LINE_CLEANUP, NULL);

    free(line);
    free(read_buf);

    close(fd_input);
    close(fd_c2s);
    close(fd_s2c);

    return 0;
}

// 모드 옵션 처리
uint32_t resolve_mode(const char *mode) {
    uint32_t curr_mode = MODE_NUM;
    char *mode_name[] = { "count", "upper", "lower", "reverse" };

    for (int i = 0; i < MODE_NUM; i++)
        if (strcmp(mode, mode_name[i]) == 0) {
            curr_mode = i;
            break;
        }

    if (curr_mode == MODE_NUM)
        return MODE_NUM;

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

ssize_t send_message(int fd, const MessageHeader *header, const void *data) {
    MessageHeader net_header;
    net_header.line_bytes = htonl(header->line_bytes);

    if (write_all(fd, &net_header, sizeof(net_header)) == -1) return -1;
    if (write_all(fd, data, header->line_bytes) == -1) return -1;

    return net_header.line_bytes;
}

ssize_t receive_message(int fd, MessageHeader *header, void **buf) {
    ssize_t bytes_read = 0;

    if (read_all(fd, header, sizeof(MessageHeader)) == -1) return -1;

    header->line_bytes = ntohl(header->line_bytes);
    if (header->line_bytes == 0) return 0;

    // realloc 안전 처리
    char *temp_buf = (char *)realloc(*buf, header->line_bytes);
    if (temp_buf == NULL) return -1;
    *buf = temp_buf;

    if ((bytes_read = read_all(fd, *buf, header->line_bytes)) == -1) {
        free(*buf);
        *buf = NULL;
        return -1;
    }

    return bytes_read;
}


// 프로세스당 파일 하나 처리용임 여러 파일 처리하려면 파일 구조체 필요
// 질문 리스트
// malloc 시 타입 캐스팅?
// free(NULL) 의미?
// memcpy, memmove NULL 반환값 예외 처리?
char *read_line(int fd, size_t *out_len) {
    static char *stash_buf = NULL;
    static size_t stash_len = 0;
    static size_t stash_size = BUFFER_SIZE;

    // 프로세스가 끝나고 메모리 정리 (Gemini)
    if (fd == READ_LINE_CLEANUP) {
        if (stash_buf != NULL) {
            free(stash_buf);
            stash_buf = NULL;
        }
        stash_len = 0;
        stash_size = 0;
        return NULL; // 정리 후 NULL 반환
    }

    if (stash_buf == NULL) {
        stash_size = BUFFER_SIZE;
        if ((stash_buf = (char *)malloc(stash_size)) == NULL) return NULL;
        else stash_buf[0] = '\0';
    }

    char read_buf[BUFFER_SIZE] = { 0, };
    char *line = NULL;
    char *new_line_ptr = NULL;
    ssize_t bytes_read = 0;
    size_t line_len = 0;
    while ((new_line_ptr = memchr(stash_buf, '\n', stash_len)) == NULL) {  // stash_buf에 줄이 있는지 확인
        if ((bytes_read = read(fd, read_buf, BUFFER_SIZE)) == -1) goto error;  // 파일에서 읽기
        if (bytes_read == 0) {  // EOF에 도달 했다면
            if (stash_len == 0) return NULL;  // 버퍼에 남은 줄이 없다면 NULL 반환

            // 남은 줄이 있다면 처리
            if ((line = (char *)malloc(stash_len + 1)) == NULL) goto error;
            memcpy(line, stash_buf, stash_len);
            line[stash_len] = '\0';
            if (out_len) *out_len = stash_len;

            free(stash_buf);
            stash_buf = NULL;
            stash_len = 0;
            return line;
        }

        // 만약 줄이 기존 버퍼보다 크면 늘리기
        if (stash_len + bytes_read + 1 >= stash_size) {
            size_t new_size = stash_size * 2;
            char *temp_buf = realloc(stash_buf, new_size);
            if (temp_buf == NULL) goto error;
            stash_buf = temp_buf;
            stash_size = new_size;
        }

        // stash_buf에 남은 데이터 이동
        memcpy(stash_buf + stash_len, read_buf, bytes_read);
        stash_len += bytes_read;
        stash_buf[stash_len] = '\0';
    }

    // stash_buf에서 line 처리
    line_len = (size_t)(new_line_ptr - stash_buf);
    if ((line = (char *)malloc(line_len + 1)) == NULL) goto error;

    memcpy(line, stash_buf, line_len);
    line[line_len] = '\0';

    memmove(stash_buf, stash_buf + line_len + 1, stash_len - (line_len + 1));
    stash_len -= (line_len + 1);

    if (out_len != NULL) *out_len = line_len;

    return line;

// 에러 처리 레이블
error:
    free(stash_buf);
    free(line);
    stash_buf = NULL;
    stash_len = 0;
    stash_size = BUFFER_SIZE;
    return NULL;
}

void print_stats(const Stats *stats) {
    printf("처리 모드: %s\n", stats->mode);
    printf("처리한 줄 수: %lu\n", stats->total_proc_lines);
    printf("소요 시간: %lf\n", stats->elapsed_time);
}

// 에러 처리 및 종료
void handle_error(const char *fmt, ...) {  // 가변인자로 포메팅 지원
    va_list args;
    va_start(args, fmt);

    // stderr로 출력
    vfprintf(stderr, fmt, args);

    va_end(args);
    exit(EXIT_FAILURE);
}
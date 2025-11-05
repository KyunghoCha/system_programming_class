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
#include <ctype.h>
#include <arpa/inet.h>  // htonl, ntohl 등
#include <unistd.h>     // UNIX 표준 함수 (read, write, close)
#include <fcntl.h>      // 파일 제어 (open 플래그, fcntl)
#include <sys/stat.h>   // umask 설정

// 나중에 헤더파일로 빼기
#define BUFFER_SIZE       4096   // 읽기, 쓰기 버퍼 크기
#define READ_LINE_CLEANUP -1     // read_line 클린업 모드
#define END_MESSAGE       "END"  // 전송이 끝나면 서버에 알릴 메시지

// 프로토콜 헤더 구조체
typedef struct {
    uint32_t line_bytes;  // 보내고 받을 줄의 크기
} MessageHeader, *pMessageHeader;

// 모드 열거형
// X-Macro 대체 가능
typedef enum {
    COUNT,    // 글자 및 단어 개수 세기
    UPPER,    // 대문자로 변환
    LOWER,    // 소문자로 변환
    REVERSE,  // 반대로 출력
    MODE_NUM  // 모드 갯수
} Mode;

// 메시지 타입 열거형 (아직 안씀, 구현x)
// 나중에 확장하면 X-와 함수포인터를 응용
typedef enum {
    MSG_TYPE_NULL,
    MSG_TYPE_MODE,
    MSG_TYPE_STRING
    // 앞으로 여러 타입이 있으면 계속 추가
} MessageType;

// 파일 구조체 (아직 안씀, 구현x)
typedef struct {
    int fd;             // 파일 디스크립터
    char *stash_buf;    // 임시 저장 버퍼
    size_t stash_len;   // 입시 저장 버퍼 길이
    size_t stash_size;  // 임시 저장 버퍼 메모리 크기
} rn_fd_t, *p_rn_fd_t;  // 이름 통일 필요

ssize_t read_all(int, void *, size_t);                           // 읽기 무결성 보장
ssize_t write_all(int, const void *, size_t);                    // 쓰기 무결성 보장
ssize_t send_message(int, const MessageHeader *, const void *);  // 서버 데이터 송신
ssize_t receive_message(int, MessageHeader *, void **);          // 서버 데이터 수신
char *proc_count(char *);                                        // count모드 문자열 처리
char *proc_upper(char *);                                        // upper모드 문자열 처리
char *proc_lower(char *);                                        // lower모드 문자열 처리
char *proc_reverse(char *);                                      // reverse모드 문자열 처리
void handle_error(const char *, ...);                            // 에러 처리

int main(int argc, char *argv[]) {
    if (argc != 1) handle_error("Usage: %s\n", argv[0]);

    // 디랙터리 없으면 생성
    if (mkdir("/tmp/fifo", 0755) == -1 && errno != EEXIST) {
        handle_error("mkdir failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    // FIFO 생성
    mode_t old_mask = umask(0);
    if (mkfifo("/tmp/fifo/fifo_c2s", 0660) == -1 && errno != EEXIST) {
        umask(old_mask);
        handle_error("mkfifo: failed to create /tmp/fifo/fifo_c2s: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    if (mkfifo("/tmp/fifo/fifo_s2c", 0660) == -1 && errno != EEXIST) {
        umask(old_mask);
        handle_error("mkfifo: failed to create /tmp/fifo/fifo_s2c: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    umask(old_mask);

    int fd_c2s;
    int fd_s2c;

    MessageHeader header = { .line_bytes = 0 };
    char *read_buf = NULL;
    char *proc_buf = NULL;

    // 함수 포인터 배열
    char *(*proc_funcs[])(char *) = {
        proc_count,
        proc_upper,
        proc_lower,
        proc_reverse
    };

    // 서버 메인 루프
    while (1) {
        // FIFO 열기
        fd_c2s = open("/tmp/fifo/fifo_c2s", O_RDONLY);
        if (fd_c2s == -1) {
            handle_error("open: failed to open /tmp/fifo/fifo_c2s: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        fd_s2c = open("/tmp/fifo/fifo_s2c", O_WRONLY);
        if (fd_s2c == -1) {
            close(fd_c2s);
            handle_error("open: failed to open /tmp/fifo/fifo_s2c: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }

        uint32_t mode = MODE_NUM;
        read_buf = NULL;
        proc_buf = NULL;

        // 모드 수신
        ssize_t len = 0;
        if ((len = read_all(fd_c2s, &header, sizeof(MessageHeader))) == -1) {
            handle_error("read_all: failed to receive mode header");
            continue;
        } else header.line_bytes = ntohl(header.line_bytes);
        if (len == 0) continue;

        if (read_all(fd_c2s, &mode, header.line_bytes) == -1) {
            handle_error("read_all: failed to receive mode");
            continue;
        } else mode = ntohl(mode);

        if (mode >= MODE_NUM) {
            handle_error("error: invalid mode %u received\n", mode);
            continue;
        }

        // 줄 처리 루프
        uint32_t total_proc_lines = 0;
        while (1) {
            // 클라이언트로부터 줄 수신
            if (receive_message(fd_c2s, &header, (void **)&read_buf) == -1) {
                handle_error("receive_message: failed to receive line");
                free(read_buf);
                break;
            }

            // END 메시지 확인
            if (strcmp(read_buf, END_MESSAGE) == 0) {
                free(read_buf);
                break;
            }

            total_proc_lines++;

            // 모드에 따라 처리
            printf("%lu번째 줄 처리 중...\n", (unsigned long)total_proc_lines);
            proc_buf = proc_funcs[mode](read_buf);
            if (proc_buf == NULL) {
                handle_error("error: failed to process line %u\n", total_proc_lines);
                free(read_buf);
                break;
            }

            // 처리 결과 전송
            header.line_bytes = strlen(proc_buf);
            if (send_message(fd_s2c, &header, proc_buf) == -1) {
                handle_error("send_message: failed to send result");
                free(read_buf);
                if (proc_buf != read_buf) free(proc_buf);  // count만 다른 버퍼
                break;
            }

            // 메모리 해제 (count는 새 버퍼, 나머지는 같은 버퍼)
            if (proc_buf != read_buf) free(proc_buf);
            free(read_buf);
            read_buf = NULL;
            proc_buf = NULL;
        }
        printf("\n");
    }

    close(fd_c2s);
    close(fd_s2c);
    return 0;
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
    char *tmp_buf = (char *)realloc(*buf, header->line_bytes + 1);
    if (tmp_buf == NULL) return -1;
    *buf = tmp_buf;

    if ((bytes_read = read_all(fd, *buf, header->line_bytes)) == -1) return -1;
    else ((char *)*buf)[header->line_bytes] = '\0';

    return bytes_read;
}

char *proc_count(char *line) {
    if (line == NULL) return NULL;

    size_t len = strlen(line);
    uint32_t w_count = 0;
    int is_word = 0;

    for (size_t i = 0; line[i] != '\0'; i++) {
        if (isspace((unsigned char)line[i])) {
            is_word = 0;
        } else if (!is_word) {
            is_word = 1;
            w_count++;
        }
    }

    char *result = malloc(64);
    if (result == NULL) return NULL;

    snprintf(result, 64, "%zu chars, %u words", len, w_count);
    return result;
}

char *proc_upper(char *line) {
    if (line == NULL) return NULL;

    for (size_t i = 0; line[i] != '\0'; i++)
        line[i] = (char)toupper((unsigned char)line[i]);

    return line;
}


char *proc_lower(char *line) {
    if (line == NULL) return NULL;

    for (size_t i = 0; line[i] != '\0'; i++)
        line[i] = (char)tolower((unsigned char)line[i]);

    return line;
}

char *proc_reverse(char *line) {
    if (line == NULL) return NULL;

    size_t len = strlen(line);
    char temp;

    for (size_t i = 0; i < len / 2; i++) {
        temp = line[i];
        line[i] = line[len - i - 1];
        line[len - i - 1] = temp;
    }

    return line;
}

// 에러 처리
void handle_error(const char *fmt, ...) {  // 가변인자로 포메팅 지원
    va_list args;
    va_start(args, fmt);

    // stderr로 출력
    vfprintf(stderr, fmt, args);

    va_end(args);
}
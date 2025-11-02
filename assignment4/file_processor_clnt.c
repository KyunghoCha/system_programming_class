//
// Created by gcha792 on 10/30/25.
//

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <arpa/inet.h>

#define MODE_NUM 4
#define BUFFER_SIZE 8192

typedef struct {
    uint32_t line_bytes;
} MessageHeader;

typedef struct {
    char *mode;
    int total_proc_lines;
    double elapsed_time;
} Stats;

int resolve_mode(const char *);
ssize_t read_all(int, void *, size_t);
ssize_t write_all(int, const void *, size_t);
int send_message(int, const MessageHeader *, const char *, size_t);
char *receive_message(int, MessageHeader *, size_t *);
char *read_line(int, size_t *);
void print_stats(const Stats *);
void handle_error(const char *, ...);

int main(int argc, char *argv[]) {
    if (argc != 3) {
        handle_error("Usage: %s <input_file> <mode>\n", argv[0]);
    }

    // FIFO 생성
    mkfifo("./fifo/fifo_c2s", 0666);
    mkfifo("./fifo/fifo_s2c", 0666);

    int fd_input = open(argv[1], O_RDONLY);
    if (fd_input == -1) {
        handle_error("Error: Cannot open input file '%s'\n", argv[1]);
    }

    int fd_c2s = open("./fifo/fifo_c2s", O_WRONLY);
    int fd_s2c = open("./fifo/fifo_s2c", O_RDONLY);
    if (fd_c2s == -1 || fd_s2c == -1) {
        close(fd_input);
        handle_error("Error: Cannot open FIFO files\n");
    }

    int curr_mode = resolve_mode(argv[2]);
    char *mode_names[] = {"count", "upper", "lower", "reverse"};

    // 모드 전송
    MessageHeader mode_header = {sizeof(int)};
    if (send_message(fd_c2s, &mode_header, (char *)&curr_mode, sizeof(int)) == -1) {
        handle_error("Error: Failed to send mode\n");
    }

    Stats stats = {
        .mode = mode_names[curr_mode],
        .total_proc_lines = 0,
        .elapsed_time = 0.0
    };

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    char *line;
    size_t line_len;
    int line_num = 1;

    // 파일을 한 줄씩 읽어서 처리
    while ((line = read_line(fd_input, &line_len)) != NULL) {
        printf("  %d번째 줄 전송...\n", line_num);

        // 줄 전송
        MessageHeader header = {line_len};
        if (send_message(fd_c2s, &header, line, line_len) == -1) {
            free(line);
            handle_error("Error: Failed to send line\n");
        }
        free(line);

        // 결과 수신
        MessageHeader recv_header;
        size_t recv_len;
        char *result = receive_message(fd_s2c, &recv_header, &recv_len);
        if (result == NULL) {
            handle_error("Error: Failed to receive result\n");
        }

        printf("  %d번째 줄 결과 수신: %s", line_num, result);
        if (result[recv_len - 1] != '\n') {
            printf("\n");
        }

        free(result);
        stats.total_proc_lines++;
        line_num++;
    }

    // END 메시지 전송
    const char *end_msg = "END";
    MessageHeader end_header = {strlen(end_msg)};
    send_message(fd_c2s, &end_header, end_msg, strlen(end_msg));

    clock_gettime(CLOCK_MONOTONIC, &end);
    stats.elapsed_time = (end.tv_sec - start.tv_sec) +
                        (end.tv_nsec - start.tv_nsec) / 1e9;

    print_stats(&stats);

    close(fd_input);
    close(fd_c2s);
    close(fd_s2c);

    return 0;
}

int resolve_mode(const char *mode) {
    char *mode_names[] = {"count", "upper", "lower", "reverse"};

    for (int i = 0; i < MODE_NUM; i++) {
        if (strcmp(mode, mode_names[i]) == 0) {
            return i;
        }
    }

    handle_error("Error: Invalid mode '%s'. Valid modes are: count, upper, lower, reverse.\n", mode);
    return -1;
}

ssize_t read_all(int fd, void *buf, size_t count) {
    size_t bytes_left = count;
    ssize_t current_read = 0;
    char *buf_ptr = (char *)buf;

    while (bytes_left > 0) {
        current_read = read(fd, buf_ptr, bytes_left);
        if (current_read == -1) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (current_read == 0) break;

        bytes_left -= current_read;
        buf_ptr += current_read;
    }

    return (ssize_t)(count - bytes_left);
}

ssize_t write_all(int fd, const void *buf, size_t count) {
    size_t bytes_left = count;
    ssize_t current_write = 0;
    const char *buf_ptr = (const char *)buf;

    while (bytes_left > 0) {
        current_write = write(fd, buf_ptr, bytes_left);
        if (current_write == -1) {
            if (errno == EINTR) continue;
            return -1;
        }

        bytes_left -= current_write;
        buf_ptr += current_write;
    }

    return (ssize_t)(count - bytes_left);
}

int send_message(int fd, const MessageHeader *header, const char *data, size_t len) {
    uint32_t net_length = htonl(header->line_bytes);
    if (write_all(fd, &net_length, sizeof(net_length)) == -1) return -1;
    if (write_all(fd, data, len) == -1) return -1;
    return 0;
}

char *receive_message(int fd, MessageHeader *header, size_t *out_len) {
    uint32_t net_length = 0;

    if (read_all(fd, &net_length, sizeof(net_length)) == -1) {
        return NULL;
    }

    uint32_t msg_length = ntohl(net_length);

    char *data = (char *)malloc(msg_length + 1);
    if (data == NULL) return NULL;

    if (read_all(fd, data, msg_length) != (ssize_t)msg_length) {
        free(data);
        return NULL;
    }

    if (out_len != NULL) *out_len = msg_length;

    header->line_bytes = msg_length;
    data[msg_length] = '\0';

    return data;
}

char *read_line(int fd, size_t *out_len) {
    static char *stash = NULL;
    static size_t stash_len = 0;

    char read_buf[BUFFER_SIZE];
    char *line = NULL;
    char *newline_pos = NULL;
    ssize_t bytes_read;

    if (stash == NULL) {
        stash = malloc(1);
        if (!stash) return NULL;
        stash[0] = '\0';
        stash_len = 0;
    }

    while (1) {
        newline_pos = memchr(stash, '\n', stash_len);

        if (newline_pos != NULL) {
            size_t line_len = newline_pos - stash;
            line = malloc(line_len + 2);
            if (!line) goto error;

            memcpy(line, stash, line_len + 1);
            line[line_len + 1] = '\0';

            size_t remaining = stash_len - line_len - 1;
            memmove(stash, newline_pos + 1, remaining);
            stash_len = remaining;
            stash[stash_len] = '\0';

            if (out_len) *out_len = line_len + 1;
            return line;
        }

        bytes_read = read(fd, read_buf, BUFFER_SIZE);

        if (bytes_read == -1) {
            if (errno == EINTR) continue;
            goto error;
        }

        if (bytes_read == 0) {
            if (stash_len == 0) {
                free(stash);
                stash = NULL;
                return NULL;
            }

            line = malloc(stash_len + 1);
            if (!line) goto error;

            memcpy(line, stash, stash_len);
            line[stash_len] = '\0';

            if (out_len) *out_len = stash_len;

            free(stash);
            stash = NULL;
            stash_len = 0;
            return line;
        }

        char *new_stash = realloc(stash, stash_len + bytes_read + 1);
        if (!new_stash) goto error;

        stash = new_stash;
        memcpy(stash + stash_len, read_buf, bytes_read);
        stash_len += bytes_read;
        stash[stash_len] = '\0';
    }

error:
    free(line);
    free(stash);
    stash = NULL;
    stash_len = 0;
    return NULL;
}

void print_stats(const Stats *stats) {
    printf("\n  === 처리 통계 ===\n");
    printf("  처리 모드: %s\n", stats->mode);
    printf("  처리한 줄 수: %d줄\n", stats->total_proc_lines);
    printf("  소요 시간: %.2f초\n", stats->elapsed_time);
}

void handle_error(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    exit(EXIT_FAILURE);
}
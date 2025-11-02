//
// Created by gcha792 on 10/30/25.
//

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 8192

typedef struct {
    uint32_t line_bytes;
} MessageHeader;

ssize_t read_all(int, void *, size_t);
ssize_t write_all(int, const void *, size_t);
int send_message(int, const MessageHeader *, const char *, size_t);
char *receive_message(int, MessageHeader *, size_t *);
void process_count(const char *, char *, size_t);
void process_upper(const char *, char *, size_t);
void process_lower(const char *, char *, size_t);
void process_reverse(const char *, char *, size_t);

int main(void) {
    // FIFO 생성
    mkfifo("./fifo/fifo_c2s", 0666);
    mkfifo("./fifo/fifo_s2c", 0666);

    int fd_c2s = open("./fifo/fifo_c2s", O_RDONLY);
    int fd_s2c = open("./fifo/fifo_s2c", O_WRONLY);

    if (fd_c2s == -1 || fd_s2c == -1) {
        fprintf(stderr, "Error: Cannot open FIFO files\n");
        exit(EXIT_FAILURE);
    }

    // 모드 수신
    MessageHeader mode_header;
    size_t mode_len;
    char *mode_data = receive_message(fd_c2s, &mode_header, &mode_len);
    if (mode_data == NULL) {
        fprintf(stderr, "Error: Failed to receive mode\n");
        exit(EXIT_FAILURE);
    }

    int mode = *(int *)mode_data;
    free(mode_data);

    void (*process_funcs[])(const char *, char *, size_t) = {
        process_count,
        process_upper,
        process_lower,
        process_reverse
    };

    int line_num = 1;

    while (1) {
        MessageHeader header;
        size_t len;
        char *line = receive_message(fd_c2s, &header, &len);

        if (line == NULL) {
            fprintf(stderr, "Error: Failed to receive message\n");
            break;
        }

        // END 메시지 확인
        if (strcmp(line, "END") == 0) {
            free(line);
            break;
        }

        printf("  %d번째 줄 처리 중...\n", line_num);

        // 결과 버퍼
        char result[BUFFER_SIZE];
        process_funcs[mode](line, result, len);

        // 결과 전송
        MessageHeader result_header = {strlen(result)};
        send_message(fd_s2c, &result_header, result, strlen(result));

        free(line);
        line_num++;
    }

    close(fd_c2s);
    close(fd_s2c);

    return 0;
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

void process_count(const char *line, char *result, size_t len) {
    int char_count = 0;
    int word_count = 0;
    int in_word = 0;

    for (size_t i = 0; i < len; i++) {
        if (line[i] == '\n') break;

        char_count++;

        if (isspace(line[i])) {
            in_word = 0;
        } else {
            if (!in_word) {
                word_count++;
                in_word = 1;
            }
        }
    }

    snprintf(result, BUFFER_SIZE, "%d chars, %d words\n", char_count, word_count);
}

void process_upper(const char *line, char *result, size_t len) {
    size_t i;
    for (i = 0; i < len && line[i] != '\0'; i++) {
        result[i] = toupper(line[i]);
    }
    result[i] = '\0';
}

void process_lower(const char *line, char *result, size_t len) {
    size_t i;
    for (i = 0; i < len && line[i] != '\0'; i++) {
        result[i] = tolower(line[i]);
    }
    result[i] = '\0';
}

void process_reverse(const char *line, char *result, size_t len) {
    size_t actual_len = len;

    // 개행 문자 제외
    if (actual_len > 0 && line[actual_len - 1] == '\n') {
        actual_len--;
    }

    // 역순으로 복사
    for (size_t i = 0; i < actual_len; i++) {
        result[i] = line[actual_len - 1 - i];
    }

    result[actual_len] = '\n';
    result[actual_len + 1] = '\0';
}
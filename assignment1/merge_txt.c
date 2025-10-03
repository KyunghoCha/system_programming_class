//
// Created by gcha792 on 9/26/25.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdbool.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#define BUFFER_SIZE 1
#define FILE_MODE 0644

// #pragma pack(push, 1)
typedef struct {
    DIR *dirent;
    struct dirent *entry;
} DirInfo, *pDirInfo;

typedef struct {
    char path[PATH_MAX];
    int src_fd;
    int output_fd;
    char io_buffer[BUFFER_SIZE];
    ssize_t bytes_read;
} FileInfo, *pFileInfo;

typedef struct {
    pDirInfo dir_info;
    pFileInfo file_info;
} AppContext, *pAppContext;
// #pragma pack(pop)

void init_dir_file_info(pAppContext *);
void check_argument_count(int);
void open_src_dir(pAppContext, const char *);
bool read_src_dir(pAppContext);
bool build_file_path(pAppContext, const char *);
void open_output_file(pAppContext, const char *, int, mode_t);
void open_src_file(pAppContext, int, mode_t);
bool read_src_file(pAppContext);
void write_output_file(pAppContext);
void close_src_file(pAppContext);
bool is_not_text_file(const char *);
void error_porc(pAppContext, const char *);
void cleanup(pAppContext);

int main(const int argc, char *argv[]) {
    check_argument_count(argc);

    pAppContext ctx = NULL;
    init_dir_file_info(&ctx);

    open_src_dir(ctx, argv[1]);
    open_output_file(ctx, argv[2], O_WRONLY | O_CREAT | O_TRUNC, FILE_MODE);

    while (read_src_dir(ctx)) {
        if (strcmp(ctx->dir_info->entry->d_name, ".")  == 0 ||
            strcmp(ctx->dir_info->entry->d_name, "..") == 0) continue;
        if (is_not_text_file(ctx->dir_info->entry->d_name)) continue;
        if (!build_file_path(ctx, argv[1])) continue;
        open_src_file(ctx, O_RDONLY, FILE_MODE);
        while (read_src_file(ctx))
            write_output_file(ctx);
        close_src_file(ctx);
        ctx->file_info->src_fd = -1;
    }

    cleanup(ctx);

    return 0;
}

void init_dir_file_info(pAppContext *ctx) {
    if (ctx == NULL) exit(EXIT_FAILURE);

    *ctx = (pAppContext)calloc(1, sizeof(AppContext));
    if (*ctx == NULL) {
        perror("Memory allocation failed for AppContext");
        exit(EXIT_FAILURE);
    }

    (*ctx)->dir_info = (pDirInfo)calloc(1, sizeof(DirInfo));
    if ((*ctx)->dir_info == NULL) {
        perror("Memory allocation failed for dir_info");
        free(*ctx);
        exit(EXIT_FAILURE);
    }

    (*ctx)->file_info = (pFileInfo)calloc(1, sizeof(FileInfo));
    if ((*ctx)->file_info == NULL) {
        perror("Memory allocation failed for file_info");
        free((*ctx)->dir_info);
        free(*ctx);
        exit(EXIT_FAILURE);
    }

    (*ctx)->file_info->src_fd = -1;
    (*ctx)->file_info->output_fd = -1;
}

void check_argument_count(int argc) {
    if (argc != 3) {
        fprintf(stderr, "Usage: merge_txt <directory> <file>\n");
        exit(EXIT_FAILURE);
    }
}

void open_src_dir(pAppContext ctx, const char *d_name) {
    pDirInfo dir_info = ctx->dir_info;

    dir_info->dirent = opendir(d_name);
    if (dir_info->dirent == NULL) error_porc(ctx, "open dir");
}

bool read_src_dir(pAppContext ctx) {
    pDirInfo dir_info = ctx->dir_info;

    errno = 0;  // 에러 초기화
    dir_info->entry = readdir(dir_info->dirent);
    if (dir_info->entry == NULL) {
        if (errno != 0) error_porc(ctx, "read dir");
        return false;  // 정상적으로 디렉터리 끝 도달
    }
    return true;
}

bool build_file_path(pAppContext ctx, const char *d_name) {
    pDirInfo dir_info = ctx->dir_info;

    if (snprintf(ctx->file_info->path, PATH_MAX, "%s/%s", d_name, dir_info->entry->d_name) >= PATH_MAX) {
        fprintf(stderr, "Path too long: %s/%s\n", d_name, dir_info->entry->d_name);
        return false;
    }
    return true;
}

void open_output_file(pAppContext ctx, const char *fpath, int oflags, mode_t mode) {
    pFileInfo file_info = ctx->file_info;

    file_info->output_fd = open(fpath, oflags, mode);
    if (file_info->output_fd == -1) error_porc(ctx, "open output file");
}

void open_src_file(pAppContext ctx, int oflags, mode_t mode) {
    pFileInfo file_info = ctx->file_info;

    file_info->src_fd = open(file_info->path, oflags, mode);
    if (file_info->src_fd == -1) error_porc(ctx, "open src file");
}

bool read_src_file(pAppContext ctx) {
    pFileInfo file_info = ctx->file_info;

    file_info->bytes_read = read(file_info->src_fd, file_info->io_buffer, sizeof(file_info->io_buffer));
    if (file_info->bytes_read == -1) error_porc(ctx, "read src file");
    return file_info->bytes_read > 0;
}

// void write_output_file(pAppContext ctx) {
//     pFileInfo file_info = ctx->file_info;
//
//     ssize_t total_written = 0;
//     while (total_written < file_info->bytes_read) {
//         ssize_t written = write(file_info->output_fd, file_info->io_buffer + total_written, file_info->bytes_read - total_written);
//
//         if (written == -1) error_porc(ctx, "write output file");
//
//         total_written += written;
//     }
// }

void write_output_file(pAppContext ctx) {
    pFileInfo file_info = ctx->file_info;

    // 공백 문자는 출력하지 않음
    if (file_info->io_buffer[0] == ' ')
        return;

    ssize_t written = write(file_info->output_fd, file_info->io_buffer, file_info->bytes_read);

    if (written == -1) error_porc(ctx, "write output file");
}

void close_src_file(pAppContext ctx) {
    pFileInfo file_info = ctx->file_info;

    if (close(file_info->src_fd) == -1) error_porc(ctx, "close src file");
    file_info->src_fd = -1;
}

bool is_not_text_file(const char *fname) {
    size_t len = strlen(fname);
    if (len < 4) return true;
    const char *ext = fname + len - 4;
    return strcasecmp(ext, ".txt") != 0;
}

void error_porc(pAppContext ctx, const char * error_message) {
    perror(error_message);
    cleanup(ctx);
    exit(EXIT_FAILURE);
}

void cleanup(pAppContext ctx) {
    if (ctx == NULL) return;

    pDirInfo dir_info = ctx->dir_info;
    pFileInfo file_info = ctx->file_info;

    if (dir_info) {
        if (dir_info->dirent) closedir(dir_info->dirent);
        free(dir_info);
    }
    if (file_info) {
        if (file_info->src_fd >= 0) close(file_info->src_fd);
        if (file_info->output_fd >= 0) close(file_info->output_fd);
        free(file_info);
    }
    free(ctx);
}
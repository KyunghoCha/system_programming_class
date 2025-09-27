//
// Created by gcha792 on 9/26/25.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
typedef struct DirInfo {
    DIR *dirent;
    struct dirent *entry;
} DirInfo, *pDirInfo;

typedef struct FileInfo {
    char path[PATH_MAX];
    int src_fd;
    int output_fd;
    char io_buffer[BUFFER_SIZE];
    ssize_t bytes_read;
} FileInfo, *pFileInfo;
// #pragma pack(pop)

void init_dir_file_info(pDirInfo *, pFileInfo *);
void check_argument_count(int);
void open_src_dir(pDirInfo, const char *);
bool read_src_dir(pDirInfo);
void open_output_file(pFileInfo, const char *, int, mode_t);
void open_src_file(pFileInfo, int, mode_t);
bool read_src_file(pFileInfo);
void write_output_file(pFileInfo);
bool is_not_text_file(const char *);

int main(const int argc, char *argv[]) {
    check_argument_count(argc);

    pDirInfo dir_info;
    pFileInfo file_info;

    init_dir_file_info(&dir_info, &file_info);
    open_src_dir(dir_info, argv[1]);
    open_output_file(file_info, argv[2], O_WRONLY | O_CREAT | O_TRUNC, FILE_MODE);

    while (read_src_dir(dir_info)) {
        if (is_not_text_file(dir_info->entry->d_name)) continue;
        snprintf(file_info->path, PATH_MAX, "%s/%s", argv[1], dir_info->entry->d_name);
        open_src_file(file_info, O_RDONLY, FILE_MODE);
        while (read_src_file(file_info))
            write_output_file(file_info);
        if (close(file_info->src_fd) == -1) perror("close");
    }

    if (closedir(dir_info->dirent) == -1) perror("close dir");
    if (close(file_info->output_fd) == -1) perror("close file");

    free(dir_info);
    free(file_info);

    return 0;
}

void init_dir_file_info(pDirInfo *dir_info, pFileInfo *file_info) {
    *dir_info = (pDirInfo)malloc(sizeof(DirInfo));
    *file_info = (pFileInfo)malloc(sizeof(FileInfo));
    if (*dir_info == NULL) {
        printf("Memory allocation failed for di\n");
        exit(1);
    }
    if (*file_info == NULL) {
        printf("Memory allocation failed for fi\n");
        exit(1);
    }
    memset(*dir_info, 0, sizeof(DirInfo));
    memset(*file_info, 0, sizeof(FileInfo));
}

void check_argument_count(int argc) {
    if (argc != 3) {
        printf("Usage: merge_txt <source_directory> <output_file>\n");
        exit(1);
    }
}

void open_src_dir(pDirInfo dir_info, const char *dname) {
    dir_info->dirent = opendir(dname);
    if (dir_info->dirent == NULL) {
        perror("open dir");
        exit(1);
    }
}

bool read_src_dir(pDirInfo dir_info) {
    errno = 0;  // 에러 초기화
    dir_info->entry = readdir(dir_info->dirent);
    if (dir_info->entry == NULL) {
        if (errno != 0) {
            perror("read dir");
            exit(1);
        }
        return false;  // 정상적으로 디렉터리 끝 도달
    }
    return true;
}

void open_output_file(pFileInfo file_info, const char *fpath, int oflags, mode_t mode) {
    file_info->output_fd = open(fpath, oflags, mode);
    if (file_info->output_fd == -1) {
        perror("open file");
        exit(1);
    }
}

void open_src_file(pFileInfo file_info, int oflags, mode_t mode) {
    file_info->src_fd = open(file_info->path, oflags, mode);
    if (file_info->src_fd == -1) {
        perror("open file");
        exit(1);
    }
}

bool read_src_file(pFileInfo file_info) {
    file_info->bytes_read = read(file_info->src_fd, file_info->io_buffer, sizeof(file_info->io_buffer));
    if (file_info->bytes_read == -1) {
        perror("read file");
        exit(1);
    }
    return file_info->bytes_read > 0;
}

void write_output_file(pFileInfo file_info) {
    if (file_info->io_buffer[0] == ' ') return;
    ssize_t written = write(file_info->output_fd, file_info->io_buffer, file_info->bytes_read);
    if (written == -1) {
        perror("write file");
        exit(1);
    }
}

bool is_not_text_file(const char *fname) {
    size_t len = strlen(fname);
    if (len < 4) return true;
    const char *ext = fname + len - 4;
    return strcasecmp(ext, ".txt") != 0;
}
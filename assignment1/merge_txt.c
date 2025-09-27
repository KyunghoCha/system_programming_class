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

#define CREAT_O_FLAG
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
    char io_buffer[1];
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

    pDirInfo di;
    pFileInfo fi;

    init_dir_file_info(&di, &fi);
    open_src_dir(di, argv[1]);
    open_output_file(fi, argv[2], O_WRONLY | O_CREAT | O_TRUNC, FILE_MODE);

    while (read_src_dir(di)) {
        if (is_not_text_file(di->entry->d_name)) continue;
        snprintf(fi->path, PATH_MAX, "%s/%s", argv[1], di->entry->d_name);
        open_src_file(fi, O_RDONLY, FILE_MODE);
        while (read_src_file(fi))
            write_output_file(fi);
        if (close(fi->src_fd) == -1) perror("close");
    }

    if (closedir(di->dirent) == -1) perror("close dir");
    if (close(fi->output_fd) == -1) perror("close file");

    free(di);
    free(fi);

    return 0;
}

void init_dir_file_info(pDirInfo *di, pFileInfo *fi) {
    *di = (pDirInfo)malloc(sizeof(DirInfo));
    *fi = (pFileInfo)malloc(sizeof(FileInfo));
    if (*di == NULL) {
        printf("Memory allocation failed for di\n");
        exit(1);
    }
    if (*fi == NULL) {
        printf("Memory allocation failed for fi\n");
        exit(1);
    }
    memset(*di, 0, sizeof(DirInfo));
    memset(*fi, 0, sizeof(FileInfo));
}

void check_argument_count(int argc) {
    if (argc != 3) {
        printf("Usage: merge_txt <source_directory> <output_file>\n");
        exit(1);
    }
}

void open_src_dir(pDirInfo di, const char *dname) {
    di->dirent = opendir(dname);
    if (di->dirent == NULL) {
        perror("open dir");
        exit(1);
    }
}

bool read_src_dir(pDirInfo di) {
    errno = 0;  // 에러 초기화
    di->entry = readdir(di->dirent);
    if (di->entry == NULL) {
        if (errno != 0) {
            perror("read dir");
            exit(1);
        }
        return false;  // 정상적으로 디렉터리 끝 도달
    }
    return true;
}

void open_output_file(pFileInfo fi, const char *fpath, int oflags, mode_t mode) {
    fi->output_fd = open(fpath, oflags, mode);
    if (fi->output_fd == -1) {
        perror("open file");
        exit(1);
    }
}

void open_src_file(pFileInfo fi, int oflags, mode_t mode) {
    fi->src_fd = open(fi->path, oflags, mode);
    if (fi->src_fd == -1) {
        perror("open file");
        exit(1);
    }
}

bool read_src_file(pFileInfo fi) {
    fi->bytes_read = read(fi->src_fd, fi->io_buffer, sizeof(fi->io_buffer));
    if (fi->bytes_read == -1) {
        perror("read file");
        exit(1);
    }
    return fi->bytes_read > 0;
}

void write_output_file(pFileInfo fi) {
    if (fi->io_buffer[0] == ' ') return;
    ssize_t written = write(fi->output_fd, fi->io_buffer, fi->bytes_read);
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
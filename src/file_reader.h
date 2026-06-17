#pragma once

#include <stddef.h>

#define LOG_BUF_S_MAX 1 << 13 // 8 KB
#define FILE_RD_S_MAX 1 << 22 // 4 MB

struct FileReadArgs {
	const char *path;
	const char **output;
};

size_t read_file(const char *path, const char **output);

void *read_file_parallel(void *args);

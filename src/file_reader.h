#pragma once

#define LOG_BUF_S_MAX 1 << 13 // 8 KB
#define FILE_RD_S_MAX 1 << 22 // 4 MB

size_t read_file(const char *path, const char **output);

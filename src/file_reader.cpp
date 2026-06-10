#include <stdio.h>
#include <stdlib.h>

#include "file_reader.h"

size_t read_file(const char *path, const char **output) {
	FILE *f = fopen(path, "r");
	if (f == NULL) {
		fprintf(stderr, "Failed to read file: File not found: %s",
			path);
		return 0;
	}
	fseek(f, 0, SEEK_END);
	size_t file_s = ftell(f);
	if (file_s + 1 > FILE_RD_S_MAX) {
		fprintf(stderr,
			"Failed to read file: Max file size allowed exceeded: "
			"Max File size %d, file path: %s",
			FILE_RD_S_MAX, path);
		return 0;
	}
	char *buf = (char *)malloc(file_s + 1);
	fseek(f, 0, SEEK_SET);
	size_t bytes_read = fread(buf, file_s, 1, f);
	fclose(f);
	buf[file_s] = '\0';
	*output = buf;
	return bytes_read;
}

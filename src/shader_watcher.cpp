#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <unistd.h>

#include "file_reader.h"
#include "shader.h"
#include "shader_watcher.h"

void *setup_shader_reload_watcher(void *args) {
	if (args == NULL) {
		fprintf(stderr, "Failed to setup shader watcher\n");
		return NULL;
	}
	struct ShaderContext *context = (struct ShaderContext *)args;
	int fd = inotify_init();
	if (fd == -1) {
		perror("inotify_init");
		return (void *)-1;
	}
	int wd_vertex_shader =
	    inotify_add_watch(fd, context->vertexShaderPath, IN_MODIFY);
	int wd_fragment_shader =
	    inotify_add_watch(fd, context->fragmentShaderPath, IN_MODIFY);
	if (wd_vertex_shader == -1) {
		perror("inotify_add_watch vertex");
		return (void *)-1;
	}
	if (wd_fragment_shader == -1) {
		perror("inotify_add_watch fragment");
		return (void *)-1;
	}
	char buf[4096]
	    __attribute__((aligned(__alignof__(struct inotify_event))));
	ssize_t size;
	fprintf(stdout, "Shader watcher shader on\n- %s\n- %s\n",
		context->vertexShaderPath, context->fragmentShaderPath);
	for (;;) {
		// should wait util shader referesh
		size = read(fd, buf, sizeof(buf));
		if (size == -1 && errno != EAGAIN) {
			perror("read");
			return (void *)-1;
		}
		// no errors but data not read
		if (size <= 0) {
			return 0;
		}

		// read shader files
		const char *vertexShaderCode = NULL;
		const char *fragmentShaderCode = NULL;

		struct FileReadArgs vertexShaderArgs = {
		    .path = context->vertexShaderPath,
		    .output = &vertexShaderCode,
		};
		struct FileReadArgs fragmentShaderArgs = {
		    .path = context->fragmentShaderPath,
		    .output = &fragmentShaderCode,
		};
		pthread_t t1, t2;
		pthread_create(&t1, NULL, read_file_parallel,
			       (void *)&vertexShaderArgs);
		pthread_create(&t2, NULL, read_file_parallel,
			       (void *)&fragmentShaderArgs);

		ssize_t vertexShaderFileReadStatus,
		    fragmentShaderFileReadStatus;
		pthread_join(t1, (void **)&vertexShaderFileReadStatus);
		pthread_join(t2, (void **)&fragmentShaderFileReadStatus);

		// no error, set recompile shader flag to true
		*context->recompileShaderFlag = 1;
	}
	return 0;
}

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <unistd.h>

#include "shader.h"
#include "shader_watcher.h"

void *setup_shader_reload_watcher(void *args) {
	if (args == NULL) {
		fprintf(stderr, "Failed to setup shader watcher\n");
		return NULL;
	}
	struct ShaderContext *shaderProg = (struct ShaderContext *)args;
	int fd = inotify_init();
	if (fd == -1) {
		perror("inotify_init");
		return (void *)-1;
	}
	int wd_vertex_shader =
	    inotify_add_watch(fd, shaderProg->vertexShaderPath, IN_MODIFY);
	int wd_fragment_shader =
	    inotify_add_watch(fd, shaderProg->fragmentShaderPath, IN_MODIFY);
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
		shaderProg->vertexShaderPath, shaderProg->fragmentShaderPath);
	int status;
	for (;;) {
		size = read(fd, buf, sizeof(buf));
		if (size == -1 && errno != EAGAIN) {
			perror("read");
			return (void *)-1;
		}
		// no errors but data not read
		if (size <= 0) {
			return 0;
		}
		// no error, reload shader
		if ((status = load_and_use_shader(
			 shaderProg->programID, shaderProg->vertexShaderPath,
			 shaderProg->fragmentShaderPath)) != 0) {
			fprintf(stderr, "Failed to load and use shaders\n");
		}
	}
	return 0;
}

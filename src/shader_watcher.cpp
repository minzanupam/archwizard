#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <unistd.h>

#include "file_reader.h"
#include "shader.h"
#include "shader_watcher.h"

void *process_buffer_event(char *buf, ssize_t size, int fd,
			   int *wd_vertex_shader, int *wd_fragment_shader,
			   struct ShaderContext *context) {
	for (char *ptr = buf; ptr < buf + size;) {
		struct inotify_event *event = (struct inotify_event *)ptr;

		if (event->wd == *wd_vertex_shader) {
			printf("Vertex shader changed! Reloading...\n");
			// Call your shader reload function here
		} else if (event->wd == *wd_fragment_shader) {
			printf("Fragment shader changed! Reloading...\n");
			// Call your shader reload function here
		}

		if (event->mask & IN_IGNORED) {
			printf("Watch was REMOVED by the kernel "
			       "(IN_IGNORED). Your editor killed the "
			       "watch!\n");
			*wd_vertex_shader = inotify_add_watch(
			    fd, context->vertexShaderPath, IN_MODIFY);
			*wd_fragment_shader = inotify_add_watch(
			    fd, context->fragmentShaderPath, IN_MODIFY);
			if (*wd_vertex_shader == -1) {
				perror("inotify_add_watch vertex");
				return (void *)-1;
			}
			if (*wd_fragment_shader == -1) {
				perror("inotify_add_watch fragment");
				return (void *)-1;
			}
			printf("Watcher readded\n");
		}
		if (event->mask & IN_DELETE_SELF) {
			printf("The file was deleted/replaced "
			       "(IN_DELETE_SELF).\n");
		}
		if (event->mask & IN_MOVE_SELF) {
			printf("The file was moved/renamed "
			       "(IN_MOVE_SELF).\n");
		}

		// Move to the next event in the buffer
		ptr += sizeof(struct inotify_event) + event->len;
	}
	return NULL;
}

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
		char *vertexShaderCode = NULL;
		char *fragmentShaderCode = NULL;

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

		// Process all events pulled into the buffer
		void *ret =
		    process_buffer_event(buf, size, fd, &wd_vertex_shader,
					 &wd_fragment_shader, context);
		if (ret != NULL) {
			fprintf(
			    stderr,
			    "File watcher: Buffer event processing failed\n");
			return ret;
		}

		// no error, set recompile shader flag to true
		*context->recompileShaderFlag = 1;
	}
	return 0;
}

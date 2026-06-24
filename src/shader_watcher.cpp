#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef __linux__
#include <sys/inotify.h>
#elif _WIN32
#include <stringapiset.h>
#include <windows.h>
#endif

#include "file_reader.h"
#include "shader.h"
#include "shader_watcher.h"

#define SBUF_S 1 << 16 // 64KB

#ifdef __linux__
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
#endif

#ifdef _WIN32
VOID process_read_directory_change(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered,
			     LPOVERLAPPED lpOverlapped) {
	struct ShaderContext *context = (struct ShaderContext *)lpOverlapped;
	*context->recompileShaderFlag = 1;
}
#endif

void *setup_shader_reload_watcher(void *args) {
	if (args == NULL) {
		fprintf(stderr, "Failed to setup shader watcher\n");
		return NULL;
	}
	struct ShaderContext *context = (struct ShaderContext *)args;
#ifdef __linux__
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
#elif _WIN32
	DWORD dwVertexShaderPathSize = 2 * context->vertexShaderPathSize;
	LPWSTR _lpwstrVertexShaderPath =
	    (wchar_t *)malloc(dwVertexShaderPathSize);
	if (!MultiByteToWideChar(CP_UTF8, 0, context->vertexShaderPath, -1,
				 _lpwstrVertexShaderPath,
				 dwVertexShaderPathSize)) {
		DWORD errorCode = GetLastError();
		fprintf(stderr, "**WIN32 API ERROR** error code: %lu\n",
			errorCode);
		return (void *)-1;
	}
	LPCWSTR lpcwstrVertexShaderPath = (LPCWSTR)_lpwstrVertexShaderPath;

	DWORD dwFragmentShaderPathSize = 2 * context->fragmentShaderPathSize;
	LPWSTR _lpwstrFragmentShaderPath =
	    (wchar_t *)malloc(dwFragmentShaderPathSize);
	if (!MultiByteToWideChar(CP_UTF8, 0, context->fragmentShaderPath, -1,
				 _lpwstrFragmentShaderPath,
				 dwFragmentShaderPathSize)) {
		DWORD errorCode = GetLastError();
		fprintf(stderr, "**WIN32 API ERROR** error code: %lu\n",
			errorCode);
		return (void *)-1;
	}
	LPCWSTR lpcwstrFragmentShaderPath = (LPCWSTR)_lpwstrFragmentShaderPath;

	HANDLE hVertexShaderFile =
	    CreateFileW(lpcwstrVertexShaderPath, GENERIC_READ,
			FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL, NULL);
	if (hVertexShaderFile == INVALID_HANDLE_VALUE) {
		DWORD errorCode = GetLastError();
		fprintf(stderr, "**WIN32 API ERROR** error code: %lu\n",
			errorCode);
		fprintf(stderr, "Failed to open vertex shader file\n");
		return (void *)-1;
	}

	LPCSTR lpBuf = (LPCSTR)malloc(SBUF_S);
	DWORD lpBufSize;
	DWORD dwStatus = 0;
	if ((dwStatus = ReadDirectoryChangesW(
		hVertexShaderFile,
		(LPVOID)lpBuf,
		SBUF_S,
		0,
		FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE,
		&lpBufSize,
		NULL,
		process_read_directory_change
	)) == ERROR_INVALID_FUNCTION) {
		DWORD errorCode = GetLastError();
		fprintf(stderr, "**WIN32 API ERROR** error code: %lu",
			errorCode);
		fprintf(stderr, "Failed to watch vertex shader file\n");
	}

#endif
	return 0;
}

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include <glad/glad.h>

#include "file_reader.h"
#include "shader.h"

struct FileReadArgs {
	const char *path;
	const char **output;
};

void *read_file_parallel(void *args) {
	struct FileReadArgs *argsData = (FileReadArgs *)args;
	const char *vertexShaderPath = argsData->path;
	const char **vertexShaderCode = argsData->output;
	ssize_t ret = read_file(vertexShaderPath, vertexShaderCode);
	return (void *)ret;
}

int load_and_use_shader(unsigned int programID, const char *vertexShaderPath,
			const char *fragmentShaderPath) {
	const char *vertexShaderCode = NULL;
	const char *fragmentShaderCode = NULL;
	unsigned int vertexShaderID, fragmentShaderID;
	int shaderStatus = 0, programStatus = 0;
	char *logBuffer = (char *)malloc(LOG_BUF_S_MAX);
	int logBufferLen = 0;

	struct FileReadArgs vertexShaderArgs = {
	    .path = vertexShaderPath,
	    .output = &vertexShaderCode,
	};
	struct FileReadArgs fragmentShaderArgs = {
	    .path = fragmentShaderPath,
	    .output = &fragmentShaderCode,
	};
	pthread_t t1, t2;
	pthread_create(&t1, NULL, read_file_parallel, (void *)&vertexShaderArgs);
	pthread_create(&t2, NULL, read_file_parallel, (void *)&fragmentShaderArgs);

	ssize_t vertexShaderFileReadStatus, fragmentShaderFileReadStatus;
	pthread_join(t1, (void **)&vertexShaderFileReadStatus);
	pthread_join(t2, (void **)&fragmentShaderFileReadStatus);

	vertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShaderID, 1, &vertexShaderCode, NULL);
	glCompileShader(vertexShaderID);
	glGetShaderiv(vertexShaderID, GL_COMPILE_STATUS, &shaderStatus);
	if (shaderStatus == GL_FALSE) {
		char *logBuffer = (char *)malloc(LOG_BUF_S_MAX);
		int logBufferLen = 0;
		glGetShaderInfoLog(vertexShaderID, LOG_BUF_S_MAX, &logBufferLen,
				   logBuffer);
		if (logBufferLen == LOG_BUF_S_MAX) {
			fprintf(stderr, "Failed to load output log: Log "
					"buffer max size reached. The rest of "
					"the log would be truncated\n");
		} else {
			logBuffer[logBufferLen] = '\0';
		}
		fprintf(stderr,
			"**GL ERROR**\nVertex Shader Compilation Failed\n%s\n",
			logBuffer);
		return -1;
	}

	fragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShaderID, 1, &fragmentShaderCode, NULL);
	glCompileShader(fragmentShaderID);
	glGetShaderiv(fragmentShaderID, GL_COMPILE_STATUS, &shaderStatus);
	if (shaderStatus == GL_FALSE) {
		glGetShaderInfoLog(fragmentShaderID, LOG_BUF_S_MAX,
				   &logBufferLen, logBuffer);
		if (logBufferLen == LOG_BUF_S_MAX) {
			fprintf(stderr, "Failed to load output log: Log "
					"buffer max size reached. The rest of "
					"the log would be truncated\n");
		} else {
			logBuffer[logBufferLen] = '\0';
		}
		fprintf(
		    stderr,
		    "**GL ERROR**\nFragment Shader Compilation Failed\n%s\n",
		    logBuffer);
		return -1;
	}

	glAttachShader(programID, vertexShaderID);
	glAttachShader(programID, fragmentShaderID);
	glLinkProgram(programID);
	glGetProgramiv(programID, GL_LINK_STATUS, &programStatus);

	if (programStatus == GL_FALSE) {
		glGetProgramInfoLog(programID, LOG_BUF_S_MAX, &logBufferLen,
				    logBuffer);
		if (logBufferLen == LOG_BUF_S_MAX) {
			fprintf(stderr, "Failed to load output log: Log "
					"buffer max size reached. The rest of "
					"the log would be truncated\n");
		} else {
			logBuffer[logBufferLen] = '\0';
		}
		fprintf(stdout, "**GL ERROR**\nProgram Link Failed\n%s\n",
			logBuffer);
		return -1;
	}
	glDetachShader(programID, vertexShaderID);
	glDetachShader(programID, fragmentShaderID);
	glDeleteShader(vertexShaderID);
	glDeleteShader(fragmentShaderID);

	free(logBuffer);
	free((void *)vertexShaderCode);
	free((void *)fragmentShaderCode);

	return 0;
}

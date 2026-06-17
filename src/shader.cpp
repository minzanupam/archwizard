#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include <glad/glad.h>

#include "file_reader.h"
#include "shader.h"

int compile_shaders(unsigned int programID, const char *vertexShaderCode,
		    const char *fragmentShaderCode) {
	unsigned int vertexShaderID, fragmentShaderID;
	int shaderStatus = 0, programStatus = 0;
	char *logBuffer = (char *)malloc(LOG_BUF_S_MAX);
	int logBufferLen = 0;

	vertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	if (vertexShaderID == 0) {
		fprintf(stderr, "Failed to create vertex shader\n");
		return -1;
	}
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
	if (fragmentShaderID == 0) {
		fprintf(stderr, "Failed to create fragment shader\n");
		return -1;
	}
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

	return 0;
}

int read_shaders(struct ShaderContext *context) {
	unsigned int programID = context->programID;
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

	ssize_t vertexShaderFileReadStatus, fragmentShaderFileReadStatus;
	pthread_join(t1, (void **)&vertexShaderFileReadStatus);
	pthread_join(t2, (void **)&fragmentShaderFileReadStatus);

	if (compile_shaders(programID, vertexShaderCode, fragmentShaderCode) !=
	    0) {
		fprintf(stderr, "Failed to complie shader\n");
	}

	free((void *)vertexShaderCode);
	free((void *)fragmentShaderCode);

	return 0;
}

void recompile_shader(struct ShaderContext *context) {
	int status;
	if ((status = read_shaders(context)) != 0) {
		fprintf(stderr, "Failed to read shader\n");
		context->recompileShaderFlag = 0;
		return;
	}
	if (context->vertexShaderCode == NULL) {
		fprintf(stderr, "Vertex shader code not loaded\n");
		context->recompileShaderFlag = 0;
		return;
	}
	if (context->fragmentShaderCode == NULL) {
		fprintf(stderr, "Fragment shader code not loaded\n");
		context->recompileShaderFlag = 0;
		return;
	}
	if ((status = compile_shaders(context->programID,
				      context->fragmentShaderCode,
				      context->vertexShaderCode)) != 0) {
		fprintf(stderr, "Failed to compile shader\n");
		context->recompileShaderFlag = 0;
		return;
	}
}

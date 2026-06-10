#include <cstdio>
#include <cstdlib>

#include <glad/glad.h>
// link after glad
#include <GLFW/glfw3.h>

#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 1024
#define LOG_BUF_S_MAX 1 << 13 // 8 KB
#define FILE_RD_S_MAX 1 << 22 // 4 MB

void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id,
				GLenum severity, GLsizei length,
				const GLchar *message, const void *userParam) {
	fprintf(stderr,
		"GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
		(type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""), type,
		severity, message);
}

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

int load_and_use_shader(unsigned int programID, const char *vertexShaderPath,
			const char *fragmentShaderPath) {
	const char *vertexShaderCode = NULL;
	const char *fragmentShaderCode = NULL;
	unsigned int vertexShaderID, fragmentShaderID;
	int shaderStatus = 0, programStatus = 0;
	char *logBuffer = (char *)malloc(LOG_BUF_S_MAX);
	int logBufferLen = 0;

	if (read_file(vertexShaderPath, &vertexShaderCode) == 0) {
		fprintf(stderr, "Failed to read vertex shader");
		return -1;
	}
	if (read_file(fragmentShaderPath, &fragmentShaderCode) == 0) {
		fprintf(stderr, "Failed to read vertex shader");
		return -1;
	}

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
		fprintf(stdout,
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
		    stdout,
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

	return 0;
}

int hot_reload_shader(unsigned int programID, const char *vertexShaderPath,
		      const char *fragmentShaderPath) {
	int status;
	if ((status = load_and_use_shader(programID, vertexShaderPath,
					  fragmentShaderPath)) != 0) {
		fprintf(stderr, "Failed to load and use shaders\n");
		return status;
	}
	return 0;
}

int main() {
	unsigned int VAO, VBO;

	if (!glfwInit()) {
		perror("init glfw");
		return -1;
	}

	// glfw window hints
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow *window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT,
					      "triangle", NULL, NULL);
	if (window == NULL) {
		fprintf(stderr, "Failed to create glfw window\n");
		return -1;
	}
	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		fprintf(stderr, "Failed to init glad");
		glfwTerminate();
		return -1;
	}

	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(MessageCallback, NULL);

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	float triangle[] = {
	    -0.5f, -0.5f, 0.0f, ///
	    0.0f,  0.5f,  0.0f, ///
	    0.5f,  -0.5f, 0.0f, ///
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 9, triangle,
		     GL_STATIC_DRAW);

	unsigned int programID = glCreateProgram();
	int status;
	if ((status =
		 load_and_use_shader(programID, "shaders/basic/vertex.glsl",
				     "shaders/basic/fragment.glsl")) != 0) {
		fprintf(stderr, "Failed to load and use shaders\n");
		return status;
	}
	glUseProgram(programID);

	if ((status = hot_reload_shader(programID, "shaders/basic/vertex.glsl",
					"shaders/basic/fragment.glsl")) != 0) {
		fprintf(stderr, "Failed to load and use shaders\n");
		return status;
	}

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float),
			      (void *)0);
	glEnableVertexAttribArray(0);

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		glClear(GL_COLOR_BUFFER_BIT);
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

		glDrawArrays(GL_TRIANGLES, 0, 3);

		glfwSwapBuffers(window);
	}
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}

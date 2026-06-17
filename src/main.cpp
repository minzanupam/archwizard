#include <atomic>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include <glad/glad.h>

#include "shader.h"
#include "shader_watcher.h"

#include <GLFW/glfw3.h>

#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 1024

void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id,
				GLenum severity, GLsizei length,
				const GLchar *message, const void *userParam) {
	fprintf(stderr,
		"GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
		(type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""), type,
		severity, message);
}

int main() {
	unsigned int VAO, VBO;
	pthread_t shader_reload_thread;

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

	std::atomic_bool recompile_shader_flag;

	struct ShaderContext context = {
	    .programID = programID,
	    .window = window,
	    .recompileShaderFlag = &recompile_shader_flag,
	    .vertexShaderPath = "shaders/basic/vertex.glsl",
	    .fragmentShaderPath = "shaders/basic/fragment.glsl",
	    .vertexShaderCode = NULL,
	    .fragmentShaderCode = NULL,
	};

	if ((status = read_shaders(&context)) != 0) {
		fprintf(stderr, "Failed to load and use shaders\n");
		return status;
	}
	compile_shaders(programID, context.vertexShaderCode,
			context.fragmentShaderCode);

	glUseProgram(programID);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float),
			      (void *)0);
	glEnableVertexAttribArray(0);

	pthread_create(&shader_reload_thread, NULL, setup_shader_reload_watcher,
		       (void *)&context);
	pthread_detach(shader_reload_thread);

	while (!glfwWindowShouldClose(window)) {
		glfwMakeContextCurrent(window);
		// hot reload shader
		if (recompile_shader_flag) {
			if ((status = read_shaders(&context)) != 0) {
				fprintf(stderr, "Failed to read shader\n");
				recompile_shader_flag = 0;
				continue;
			}
			if (context.vertexShaderCode == NULL) {
				fprintf(stderr,
					"Vertex shader code not loaded\n");
				recompile_shader_flag = 0;
				continue;
			}
			if (context.fragmentShaderCode == NULL) {
				fprintf(stderr,
					"Fragment shader code not loaded\n");
				recompile_shader_flag = 0;
				continue;
			}
			if ((status = compile_shaders(
				 programID, context.fragmentShaderCode,
				 context.vertexShaderCode)) != 0) {
				fprintf(stderr, "Failed to compile shader\n");
				recompile_shader_flag = 0;
				continue;
			}
			recompile_shader_flag = 0;
		}

		glfwPollEvents();
		glClear(GL_COLOR_BUFFER_BIT);
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

		glDrawArrays(GL_TRIANGLES, 0, 3);

		glfwSwapBuffers(window);
		glfwMakeContextCurrent(NULL);
	}

	pthread_cancel(shader_reload_thread);

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}

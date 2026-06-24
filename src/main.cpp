#include <atomic>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include <glad/glad.h>

#include "glm/ext/matrix_transform.hpp"
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

	std::atomic_bool recompile_shader_flag(0);

	struct ShaderContext context = {
	    .programID = programID,
	    .window = window,
	    .recompileShaderFlag = &recompile_shader_flag,
	    .vertexShaderPath = "shaders/basic/vertex.glsl",
	    .fragmentShaderPath = "shaders/basic/fragment.glsl",
	    .vertexShaderPathSize = 25,
	    .fragmentShaderPathSize = 27,
	};

	char *vertexShaderCode = NULL, *fragmentShaderCode = NULL;
	if ((status = read_shaders(
		 context.vertexShaderPath, context.fragmentShaderPath,
		 &vertexShaderCode, &fragmentShaderCode)) != 0) {
		fprintf(stderr, "Failed to load and use shaders\n");
		return status;
	}
	compile_shaders(programID, vertexShaderCode, fragmentShaderCode);
	free(vertexShaderCode);
	free(fragmentShaderCode);

	glUseProgram(programID);

	glm::mat4 model = glm::mat4(1.0f);
	glm::mat4 view = glm::mat4(1.0f);
	glm::mat4 projection = glm::perspective(
	    glm::radians(45.0f), (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT,
	    0.01f, 100.0f);
	unsigned int u_Model = glGetUniformLocation(programID, "model");
	unsigned int u_View = glGetUniformLocation(programID, "view");
	unsigned int u_Projection =
	    glGetUniformLocation(programID, "projection");

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float),
			      (void *)0);
	glEnableVertexAttribArray(0);

	pthread_create(&shader_reload_thread, NULL, setup_shader_reload_watcher,
		       (void *)&context);
	pthread_detach(shader_reload_thread);

	model = glm::translate(model, glm::vec3(0.0f, 0.0f, -3.0f));

	while (!glfwWindowShouldClose(window)) {
		glfwMakeContextCurrent(window);
		// hot reload shader
		if (recompile_shader_flag) {
			if ((status = recompile_shader(&context)) != 0) {
				fprintf(stderr, "Failed to recompile shader\n");
			}
			recompile_shader_flag = 0;
		}

		glfwPollEvents();
		glClear(GL_COLOR_BUFFER_BIT);
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

		glUniformMatrix4fv(u_Model, 1, GL_FALSE, glm::value_ptr(model));
		glUniformMatrix4fv(u_View, 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(u_Projection, 1, GL_FALSE,
				   glm::value_ptr(projection));

		glDrawArrays(GL_TRIANGLES, 0, 3);

		glfwSwapBuffers(window);
		glfwMakeContextCurrent(NULL);
	}

	pthread_cancel(shader_reload_thread);

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}

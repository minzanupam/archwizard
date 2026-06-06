#include <cstdio>

#include <glad/glad.h>
///
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
	uint32_t VAO, VBO;
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
	    -1.0f, -1.0f, 0.0f, ///
	    0.0f,  1.0f,  0.0f, ///
	    1.0f,  -1.0f, 0.0f, ///
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 9, triangle,
		     GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void *)0);
	glEnableVertexAttribArray(0);

	while (!glfwWindowShouldClose(window)) {
		glClear(GL_COLOR_BUFFER_BIT);

		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

		glDrawArrays(GL_TRIANGLES, 0, 3);

		glfwPollEvents();
		glfwSwapBuffers(window);
	}
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}

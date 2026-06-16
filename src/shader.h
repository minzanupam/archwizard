#pragma once

#include <pthread.h>

#include <GLFW/glfw3.h>

struct ShaderContext {
	unsigned int programID;
	GLFWwindow *window;
	pthread_mutex_t *shaderContextMutex;
	const char *vertexShaderPath;
	const char *fragmentShaderPath;
};

int load_shaders(struct ShaderContext *context);

#pragma once

#include <GLFW/glfw3.h>

struct ShaderContext {
	unsigned int programID;
	GLFWwindow *window;
	const char *vertexShaderPath;
	const char *fragmentShaderPath;
};

int load_shaders(struct ShaderContext *context);

int reload_shaders(struct ShaderContext *context);

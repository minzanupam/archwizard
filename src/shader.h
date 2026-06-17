#pragma once

#include <pthread.h>

#ifdef __cplusplus
#include <atomic>
#else
#include <stdatomic.h>
#endif

#include <GLFW/glfw3.h>

struct ShaderContext {
	unsigned int programID;
	GLFWwindow *window;
#ifdef __cplusplus
	std::atomic_bool *recompileShaderFlag;
#else
	atomic_bool *recompileShaderFlag;
#endif
	const char *vertexShaderPath;
	const char *fragmentShaderPath;
	const char *vertexShaderCode;
	const char *fragmentShaderCode;
};

int read_shaders(struct ShaderContext *context);

int compile_shaders(unsigned int programID, const char *vertexShaderCode,
		    const char *fragmentShaderCode);

int recompile_shader(struct ShaderContext *context);

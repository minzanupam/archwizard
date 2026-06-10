#pragma once

struct ShaderContext {
	unsigned int programID;
	const char *vertexShaderPath;
	const char *fragmentShaderPath;
};

int load_and_use_shader(unsigned int programID, const char *vertexShaderPath,
			const char *fragmentShaderPath);

#ifndef _SHADER_H_
#define _SHADER_H_

#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>

#include <GL/glew.h>

class Shader
{
public:
	Shader();

	void CreateFromString(const char *vertexCode, const char *fragmentCode);
	void CreateFromFiles(const char *vertexLocation, const char *fragmentLocation);

	std::string ReadFile(const char *fileLocation);

	GLuint GetProjectionLocation();
	GLuint GetModelLocation();

	void UseShader();
	void ClearShader();

	~Shader();

private:
	GLuint shaderID, uniformProjection, uniformModel;

	void CompileShader(const GLchar *vertexCode, const GLchar *fragmentCode);
	void AddShader(GLuint theProgram, const char *shaderCode, GLenum shaderType);
};

#endif
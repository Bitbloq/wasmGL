#ifndef _SHADER_H_
#define _SHADER_H_

#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>

#include "wasmgl_gl.h"

class Shader
{
public:
	Shader();

	void CreateFromString(const char *vertexCode, const char *fragmentCode);
	void CreateFromFiles(const char *vertexLocation, const char *fragmentLocation);

	std::string ReadFile(const char *fileLocation);

	GLuint GetProjectionLocation();
	GLuint GetModelLocation();
	GLuint GetViewLocation();
	GLuint GetNormalMatrixLocation() const { return uniformNormalMatrix; }
	GLuint GetObjectColorLocation() const { return uniformObjectColor; }
	GLuint GetViewPosLocation() const { return uniformViewPos; }
	GLuint GetLightDirLocation() const { return uniformLightDir; }
	GLuint GetAmbientLocation() const { return uniformAmbient; }
	GLuint GetSpecStrengthLocation() const { return uniformSpecStrength; }
	GLuint GetShininessLocation() const { return uniformShininess; }
	/** For line / wire pass (single mvp). */
	GLuint GetMVPLocation() const { return uniformMVP; }

	GLuint GetProgramId() const { return shaderID; }

	void UseShader();
	void ClearShader();

	~Shader();

private:
	GLuint shaderID, uniformProjection, uniformModel, uniformView;
	GLuint uniformNormalMatrix{0};
	GLuint uniformObjectColor{0};
	GLuint uniformViewPos{0};
	GLuint uniformLightDir{0};
	GLuint uniformAmbient{0};
	GLuint uniformSpecStrength{0};
	GLuint uniformShininess{0};
	GLuint uniformMVP{0};

	void CompileShader(const GLchar *vertexCode, const GLchar *fragmentCode);
	void AddShader(GLuint theProgram, const char *shaderCode, GLenum shaderType);
};

#endif
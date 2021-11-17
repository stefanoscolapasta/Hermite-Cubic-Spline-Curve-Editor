#pragma once
#include "Lib.h"

class ShaderMaker
{
    public:
		static GLuint createProgram(char* vertexfilename, char *fragmentfilename);
		static char* readShaderSource(const char* shaderFile);
	
    private:
		ShaderMaker() { }	
};
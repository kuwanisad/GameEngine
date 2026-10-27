#include "../../Public/Rendering/Shader.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <SDL.h>

FShaderPipeline::FShaderPipeline()
    : ProgramID(0),
    VertexShaderID(0),
    FragShaderID(0)
{
}

FShaderPipeline::~FShaderPipeline()
{
}

bool FShaderPipeline::IsVaildProgram()
{
    GLint status;
    glGetProgramiv(ProgramID, GL_LINK_STATUS, &status);
    if (status != GL_TRUE)
    {
        char buffer[512];
        memset(buffer, 0, 512);
        glGetProgramInfoLog(ProgramID, 511, nullptr, buffer);
        SDL_Log("GLSL Link Status:\n%s", buffer);
        return false;
    }

    return true;
}

void FShaderPipeline::Unload()
{
    glDeleteProgram(ProgramID);
    glDeleteShader(VertexShaderID);
    glDeleteShader(FragShaderID);
}

void FShaderPipeline::SetName(const FString& InName)
{
   Name = InName;
}

void FShaderPipeline::SetIDs(GLuint InProgramID, GLuint InVertexShaderID, GLuint InFragShaderID)
{
    ProgramID = InProgramID;
    VertexShaderID = InVertexShaderID;
    FragShaderID = InFragShaderID;
}

void FShaderPipeline::SetActive()
{
    glUseProgram(ProgramID);
}

void FShaderPipeline::SetMatrix(const char* Name, const FMatrix4x4& Matrix)
{
    GLuint loc = glGetUniformLocation(ProgramID, Name);

    if (loc == -1)
    {
        std::cout << "Shader error " << Name << std::endl;
    }

    glUniformMatrix4fv(loc, 1, GL_TRUE, Matrix.GetAsFloatPtr());
}

void FShaderPipeline::SetMatrices(const char* Name, const FMatrix4x4* Matrices, uint32 Count)
{
    GLuint loc = glGetUniformLocation(ProgramID, Name);

    if (loc == -1)
    {
        std::cout << "Shader error " << Name << std::endl;
    }

    // Matrix4는 순수 float 배열(내부 표현이 연속된 16개 float)이라, 매트릭스 여러 개를
    // 하나의 연속된 float 버퍼처럼 그대로 GPU에 통째로 업로드할 수 있다.
    glUniformMatrix4fv(loc, static_cast<GLsizei>(Count), GL_TRUE, Matrices[0].GetAsFloatPtr());
}

void FShaderPipeline::SetVector(const char* Name, const FVector3& Vector)
{
    GLuint loc = glGetUniformLocation(ProgramID, Name);

    if (loc == -1)
    {
        std::cout << "Shader error " << Name << std::endl;
    }

    glUniform3fv(loc, 1, Vector.GetAsFloatPtr());
}

void FShaderPipeline::SetFloat(char* Name, float Value)
{
    GLuint loc = glGetUniformLocation(ProgramID, Name);

    if (loc == -1)
    {
        std::cout << "Shader error " << Name << std::endl;
    }

    glUniform1f(loc, Value);
}

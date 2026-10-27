#pragma once
#include "../Core/CoreMinimal.h"
#include "../Core/Math.h"

#include <GL/glew.h>

struct FShaderPipeline
{
private:
	bool IsVaildProgram();

	GLuint VertexShaderID;
	GLuint FragShaderID;
	GLuint ProgramID;

	FString Name;

public:
	FShaderPipeline();
	~FShaderPipeline();
	void Unload();

	void SetName(const FString& InName);
	FString GetName() const { return Name; }

	void SetIDs(GLuint InProgramID, GLuint InVertexShaderID, GLuint InFragShaderID);
	GLuint GetVertexShaderID() const { return VertexShaderID; }
	GLuint GetFragShaderID() const { return FragShaderID; }
	GLuint GetProgramID() const { return ProgramID; }

	void SetActive();
	void SetMatrix(const char* Name, const FMatrix4x4& Matrix);
	void SetMatrices(const char* Name, const FMatrix4x4* Matrices, uint32 Count);
	void SetVector(const char* Name, const FVector3& Vector);
	void SetFloat(char* Name, float Value);
};
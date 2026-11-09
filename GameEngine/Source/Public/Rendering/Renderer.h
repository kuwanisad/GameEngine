#pragma once
#include "../Core/Math.h"

#include <SDL.h>

struct FShaderPipeline;

class URenderer
{
private:
	FShaderPipeline* ShaderPipeline_Sprite;
	FShaderPipeline* ShaderPipeline_Mesh;
	FShaderPipeline* ShaderPipeline_Skin;

	FMatrix4x4 Matrix_View;
	FMatrix4x4 Matrix_Projection;
	float ScreenWidth;
	float ScreenHeight;

	SDL_Window* Window;
	SDL_GLContext Context;
public:
	URenderer();
	~URenderer();

	bool Init();
	void Shutdown();
	void UnloadData();

	void BeginFrame();
	void EndFrame();

	void SetViewMatrix(const FMatrix4x4& InView) { Matrix_View = InView; }
	FMatrix4x4 GetViewProjMatrix() const { return Matrix_View * Matrix_Projection; }
};
#pragma once
#include "../Core/Math.h"

#include <SDL.h>

struct FShaderPipeline;
struct FVertexContainer;

struct FDirectionalLight
{
	FVector3 Direction;
	FVector3 DiffuseColor;
	FVector3 SpecColor;
};

class URenderer
{
private:
	FShaderPipeline* ShaderPipeline_Sprite;
	FShaderPipeline* ShaderPipeline_Mesh;
	FShaderPipeline* ShaderPipeline_Skin;

	FVertexContainer* VertexContainer_Sprite;

	FMatrix4x4 Matrix_View;
	FMatrix4x4 Matrix_Projection;
	float ScreenWidth;
	float ScreenHeight;

	SDL_Window* Window;
	SDL_GLContext Context;

	FVector3 AmbientLight;
	FDirectionalLight DirectionalLight;

	bool LoadShaders();

	//참고서의 함수
	void CreateSpriteVerts();

public:
	URenderer();
	~URenderer();

	//참고서의 함수를 수정했음
	bool Init();

	void Shutdown();
	void UnloadData();

	void BeginFrame();
	void EndFrame();

	void SetViewMatrix(const FMatrix4x4& InView) { Matrix_View = InView; }
	FMatrix4x4 GetViewProjMatrix() const { return Matrix_View * Matrix_Projection; }

	void SetScreenWidth(float InScreenWidth) { ScreenWidth = InScreenWidth; }
	float GetScreenWidth() const { return ScreenWidth; }

	void SetScreenHeight(float InScreenHeight) { ScreenHeight = InScreenHeight; }
	float GetScreenHeight() const { return ScreenHeight; }

	FShaderPipeline* GetSkinnedShader() const { return ShaderPipeline_Skin; }

	void SetLightUniforms(FShaderPipeline* InShader);

	void SetAmbientLight(FVector3& InAmbientLight) { AmbientLight = InAmbientLight; }
	FDirectionalLight& GetDirectionalLight() { return DirectionalLight; }
};
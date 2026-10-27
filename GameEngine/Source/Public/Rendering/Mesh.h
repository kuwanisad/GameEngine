#pragma once
#include "../Core/CoreMinimal.h"
#include "../Collision/Collision.h"

class UTexture;
class URenderer;

struct FVertexContainer;

//원래는 버텍스를 따로 분리해야 하지만 이 프로젝트에서 마땅히 둘 곳이 없어서 합침...
class UMesh
{
protected:
	TArray<UTexture*> Textures;
	FVertexContainer* VertexContainer;
	FString ShaderName;
	float Specular;
	FString Name;

public:
	UMesh();
	~UMesh();

	void SetName(const FString& InName);
	FString GetName() const { return Name; }

	void Unload();

	void SetVertexContainer(FVertexContainer* InVertexContainer);
	FVertexContainer* GetVertexContainer() { return VertexContainer; } const
	const FString& GetShaderName() const { return ShaderName; }
	float GetSpecular();
};
#pragma once
#include "../Core/CoreMinimal.h"
#include "../Core/Math.h"
#include "../Rendering/RenderingStatics.h"
#include "Skeleton.h"


struct FVertexContainer;

class UTexture;

class USkeletalMesh
{
private:


	FVertexContainer* Vertexes;
	USkeleton* Skeleton;
	UTexture* DefaultTexture;

public:
	USkeletalMesh();
	~USkeletalMesh();

	bool Load(const FString& FileName);
	void Unload();

	void SetVertexContainer(FVertexContainer* Value) { Vertexes = Value; }
	void SetTexture(UTexture* Value) { DefaultTexture = Value; }
	void SetSkeleton(USkeleton* Value) { Skeleton = Value; }
	FVertexContainer* GetVertexContainer() const { return Vertexes; }
	const USkeleton* GetSkeleton() const { return Skeleton; }
	UTexture* GetTexture() const { return DefaultTexture; }
};
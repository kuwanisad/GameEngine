#pragma once
#include "PrimitiveComponent.h"

class AActor;
class UMesh;
struct FShaderPipeline;

class UStaticMeshComponent : public UPrimitiveComponent
{
	UMesh* Mesh;
	bool Visible;
	int32 TextureIndex;

public:
	UStaticMeshComponent(AActor* _Owner);
	virtual ~UStaticMeshComponent();

	virtual void Init() override;

	virtual void OnDestroy() override;

	virtual void Draw(FShaderPipeline* Pipeline);

	virtual void SetMesh(UMesh* InMesh) { Mesh = InMesh; }
	
	void SetVisible(bool _Visible) { Visible = _Visible; }
	bool GetVisible() const { return Visible; }
	
	void SetTextureIndex(int32 _Index) { TextureIndex = _Index; }
};
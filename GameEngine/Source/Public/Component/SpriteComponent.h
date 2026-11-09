#pragma once
#include "../Component/ActorComponent.h"
#include "../Core/CoreMinimal.h"

class UTexture;
struct FShaderPipeline;
class AActor;

class USpriteComponent : public UActorComponent
{
protected:
	class UTexture* Sprite;
	int RenderingOrder;
	int Width;
	int32 Height;

public:
	USpriteComponent(AActor* InOwner, int InRenderingOrder);
	virtual ~USpriteComponent();

	virtual void Init() override;
	virtual void OnDestroy() override;
	virtual void Draw(FShaderPipeline* Pipeline);
	virtual void SetTexture(UTexture* InTexture);

	int GetRenderingOrder() const { return RenderingOrder; }
	int GetHeight() const { return Width; }
	int GetWidth() const { return Height; }
};
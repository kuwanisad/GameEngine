#pragma once
#include "../Component/SceneComponent.h"
#include "../Core/Math.h"

class AActor;

class UCameraComponent : public USceneComponent
{
public:
	UCameraComponent(AActor* _Owner);

	void Tick(float DeltaTime) override;

	// UE 실제 이름/시그니처 그대로.
	float GetFieldOfView() const { return FieldOfView; }
	virtual void SetFieldOfView(float InFieldOfView) { FieldOfView = InFieldOfView; }

protected:
	void SetViewMatrix(const FMatrix4x4& view);

	float FieldOfView = 90.0f;
};
#pragma once
#include "ShapeComponent.h"
#include "../Collision/Collision.h"

class AActor;

class UBoxComponent : public UShapeComponent
{
public:
	UBoxComponent(AActor* owner);
	~UBoxComponent();

	void Init() override;

	void OnUpdateWorldTransform() override;

	void SetObjectBox(const FAABB& model) { ObjectBox = model; }
	const FAABB& GetWorldBox() const { return WorldBox; }

	void SetShouldRotate(bool value) { ShouldRotate = value; }
private:
	bool ShouldRotate;
	FAABB ObjectBox;
	FAABB WorldBox;
};

#pragma once
#include "../Actor/Actor.h"

class UMovementComponent;
class UStaticMeshComponent;
class UBoxComponent;
class AFPSCamera;
class UEnhancedInputComponent;
class UInputMappingContext;

class AFPSActor : public AActor
{
public:
	AFPSActor();
	virtual ~AFPSActor();

	virtual void Tick(float DeltaTime) override;

	void SetFootstepSurface(float value);
	void SetVisible(bool _Visible);

private:
	UMovementComponent* MoveComp;
	UStaticMeshComponent* MeshComp;
	UBoxComponent* BoxComp;
	AFPSCamera* CameraComp;
	AActor* FPSModel;
	float LastFootstep;

	UEnhancedInputComponent* InputComp;
	UInputMappingContext* MappingContext;
};


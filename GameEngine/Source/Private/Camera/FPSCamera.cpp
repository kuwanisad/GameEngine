#include "../../Public/Camera/FPSCamera.h"
#include "../../Public/Actor/Actor.h"

UFPSCamera::UFPSCamera(AActor* _Owner) :
	UCameraComponent(_Owner),
	PitchSpeed(0),
	MaxPitch(Math::Pi / 3.0f),
	CurrentPitch(0)
{
}

void UFPSCamera::Tick(float DeltaTime)
{
	UCameraComponent::Tick(DeltaTime);

	FVector3 CameraPos = GetComponentLocation();

	// [즉시 반영] CurrentPitch는 이제 여기서 적분하지 않음 - FPSActor::UpdateActor가
	// 일반 그룹 타이밍에 IntegratePitch()로 이미 확정해둔 값을 그대로 읽기만 함.
	FQuaternion q(Owner->GetActorRightVector(), CurrentPitch);

	FVector3 ViewDirection = FVector3::Transform(GetForwardVector(), q);
	FVector3 TargetVector = CameraPos + ViewDirection * 100.f;
	FVector3 UpVector = FVector3::Transform(FVector3::UnitZ, q);
	FMatrix4x4 ViewMatrix = FMatrix4x4::CreateLookAt(CameraPos, TargetVector, UpVector);
	SetViewMatrix(ViewMatrix);
}

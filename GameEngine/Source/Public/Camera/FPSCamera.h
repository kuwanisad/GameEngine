#pragma once
#include "CameraComponent.h"

class AFPSCamera : public UCameraComponent
{
public:
	AFPSCamera(class AActor* _Owner);

	void Tick(float DeltaTime) override;

	// [즉시 반영] 예전엔 이 적분을 Update()(TG_PostUpdateWork, 지연 그룹) 안에서만 했는데,
	// 그러면 같은 프레임의 일반 그룹(트랜스폼 단계)에서 CurrentPitch를 읽는 코드(FPSActor의
	// 총 모델 회전 계산)가 "아직 안 갱신된, 지난 프레임 값"을 읽게 됨. 이 함수를 일반
	// 그룹 타이밍(FPSActor::UpdateActor)에서 직접 호출해서, 요(yaw)와 같은 타이밍에
	// 피치도 확정되도록 함. Update()는 이제 이미 확정된 CurrentPitch로 뷰 행렬만 만드는
	// 순수 소비자 역할만 함.
	void IntegratePitch(float DeltaTime)
	{
		CurrentPitch += PitchSpeed * DeltaTime;
		CurrentPitch = Math::Clamp(CurrentPitch, -MaxPitch, MaxPitch);
	}

	float GetPitch() const { return CurrentPitch; }
	float GetPitchSpeed() const { return PitchSpeed; }
	float GetMaxPitch() const { return MaxPitch; }

	void SetPitchSpeed(float speed) { PitchSpeed = speed; }
	void SetMaxPitch(float pitch) { MaxPitch = pitch; }
private:
	float PitchSpeed;
	float MaxPitch;
	float CurrentPitch;
};
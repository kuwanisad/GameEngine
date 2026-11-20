#pragma once
#include "../Core/CoreMinimal.h"
#include "../Actor/Actor.h"

class UMovementComponent;
class USkeletalMeshComponent;
class UBoxComponent;
class AFPSCamera;
class UEnhancedInputComponent;
class UInputMappingContext;
class UEngine;

// [3인칭 테스트용, FPSActor를 그대로 본뜸] FPSActor와 다른 지점만 짚으면:
//  - 총(UStaticMeshComponent, 별도 액터 FPSModel)이 아니라, 실제로 애니메이션되는
//    스켈레탈 메시(USkeletalMeshComponent)를 이 액터 자신에게 직접 부착함 -
//    FPSActor가 FPSModel을 별도 액터로 분리했던 이유(카메라가 모델 내부에 있어서
//    생기는 시야 문제)가 3인칭에는 없어서, 그 우회가 필요 없음.
//  - 카메라는 아직 3인칭 전용 카메라 클래스가 없어서 FPSCamera를 그대로 재사용함
//    (합의됨 - 지금은 시각 연출이 아니라 "실제 입력으로 애니메이션이 블렌딩되는지"가
//    목적이라 카메라 작업은 범위 밖으로 미룸).
class AThirdPersonCharacter : public AActor
{
public:
	// SkeletalMeshPath - --loadskm으로 받은 경로를 그대로 넘겨받음(FTestManager가
	// SpawnActor<ThirdPersonCharacter>(SkeletalMeshPath) 형태로 생성).
	AThirdPersonCharacter(const FString& SkeletalMeshPath);
	virtual ~AThirdPersonCharacter();

	virtual void Tick(float DeltaTime) override;

	// [UAnimInstance가 OwningActor를 이 타입으로 캐스팅해서 부름] 실제 이동
	// 속도를 UAnimInstance 쪽에 노출하는 유일한 창구 - UE의 "UAnimInstance가
	// 소유주를 통해 컴포넌트에 접근" 패턴 그대로.
	UMovementComponent* GetMovementComponent() const { return MoveComp; }

private:
	UMovementComponent* MoveComp;
	USkeletalMeshComponent* SkelMeshComp;
	UBoxComponent* BoxComp;
	AFPSCamera* CameraComp;

	UEnhancedInputComponent* InputComp;
	UInputMappingContext* MappingContext;
};
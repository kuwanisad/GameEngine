// ----------------------------------------------------------------
// 물리 테스트용 박스 액터 - 콜리전 박스가 루트, 메시가 거기 부착되는 흔한 UE 패턴.
// [배경] 예전엔 Game::LoadData() 안에서 빈 AActor를 SpawnActor로 만든 뒤, 그 밖에서
// UBoxComponent/UStaticMeshComponent를 따로 조립하는 낡은 방식(BallActor/TargetActor가
// 자리잡기 전의 패턴)을 그대로 쓰고 있었음 - 컴포넌트가 액터 "생성자 밖"에서 나중에
// 붙다 보니, SpawnActor가 생성 직후 부르는 Init()이 실행되는 시점엔 아직 콜리전이
// 하나도 없어서 물리 레이어 등록이 통째로 스킵되는 버그가 실제로 있었음. 이 클래스로
// 옮기면(다른 파생 액터들처럼 생성자 "안에서" 컴포넌트를 다 만듦) 그 문제 자체가
// 구조적으로 없어짐 - Init()이 불릴 때는 이미 생성자가 다 끝난 뒤라 콜리전이 준비돼있음.
// ----------------------------------------------------------------

#pragma once
#include "../Actor/Actor.h"

class APhysicsTestBox : public AActor
{
public:
	APhysicsTestBox(const FVector3& InLocation, float InScale, float InMass, float InRestitution);

private:
	class UBoxComponent* BoxComp;
	class UStaticMeshComponent* MeshComp;
};

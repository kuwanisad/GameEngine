#include "../../Public/Test/PhysicsTestBox.h"
#include "../../Public/Core/Engine.h"
#include "../../Public/Rendering/Renderer.h"
#include "../../Public/Component/StaticMeshComponent.h"
#include "../../Public/Component/BoxComponent.h"
#include "../../Public/Rendering/RenderingManager.h"
#include "../../Public/Asset/AssetManager.h"

APhysicsTestBox::APhysicsTestBox(const FVector3& InLocation, float InScale, float InMass, float InRestitution)
	:AActor()
{
	// [버그 수정 - Game::LoadData에서 그대로 옮겨옴] SetObjectBox는 "메시 로컬 좌표계"
	// 기준 크기를 받고, 그 위에 UBoxComponent::OnUpdateWorldTransform()이
	// GetComponentScale()을 곱해서 최종 콜리전 크기를 만듦. Cube.gpmesh는 로컬
	// 반너비가 0.5(원본 정점 범위 -0.5~0.5)이므로, 여기엔 0.5를 넣고 InScale이 곱해져서
	// 최종 반너비가 되게 함 - 예전엔 여기 최종 크기를 직접 넣어서 스케일이 중복
	// 적용됐던 게 폭주의 원인이었음(과거 세션 참고).
	const FAABB boxShape(FVector3(-0.5f, -0.5f, -0.5f), FVector3(0.5f, 0.5f, 0.5f));

	MeshComp = new UStaticMeshComponent(this);
	MeshComp->SetMesh(UAssetManager::Get()->LoadMesh("Assets/Cube.gpmesh"));

	BoxComp = new UBoxComponent(this);
	BoxComp->SetObjectBox(boxShape);
	BoxComp->SetShouldRotate(true);

	// [부착 계층] 콜리전 박스를 루트로 삼고(물리/이동은 다 이 박스 기준), 메시는 거기
	// 부착 - "콜리전이 위치를 정하고 메시가 그걸 따라간다"는 흔한 UE 패턴.
	SetRootComponent(BoxComp);
	MeshComp->SetupAttachment(BoxComp);

	// [순서 중요] SetRootComponent보다 반드시 뒤에 와야 함 - 먼저 부르면 그 시점의(곧
	// 버려질) 기본 루트한테 위치/스케일이 적용되고, 새 루트(BoxComp)로 안 넘어가서 사라짐.
	SetActorLocation(InLocation);
	SetActorScale(FVector3::One * InScale);

	BoxComp->BodyInstance.SimulatePhysics = true;
	BoxComp->BodyInstance.EnableGravity = true;
	BoxComp->BodyInstance.MassInKg = InMass;
	BoxComp->BodyInstance.PhysMaterial.Restitution = InRestitution;

	// [등록 시점 확인] 이 시점(생성자 본문 끝)엔 위에서 BoxComp->SetObjectBox()가 이미
	// 실제 콜리전 도형을 배정해뒀으므로, SpawnActor가 생성 직후 부르는 Init()이
	// GetCollisions()를 확인할 때 정상적으로 값이 잡힘 - 예전 Game::LoadData 방식
	// (컴포넌트를 생성자 "밖"에서 나중에 붙이던 것)에서 있었던, 등록이 통째로 스킵되는
	// 문제 자체가 이 클래스 구조에서는 발생하지 않음.
}

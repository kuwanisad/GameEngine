#include "../../Public/Camera/CameraComponent.h"
#include "../../Public/Actor/Actor.h"
#include "../../Public/Core/Engine.h"
#include "../../Public/Rendering/RenderingManager.h"
#include "../../Public/Rendering/Renderer.h"

UCameraComponent::UCameraComponent(AActor* _Owner) :
	USceneComponent(_Owner, ETickingGroup::TG_PostUpdateWork)
{
}

void UCameraComponent::Tick(float DeltaTime)
{
	USceneComponent::Tick(DeltaTime);

	// [보조 안전장치] 진짜 해법은 이제 TG_PostUpdateWork 스케줄링(생성자 참고, UE 공식
	// 문서로 확인됨) - 이 컴포넌트는 이번 서브스텝의 다른 모든 액터가 Update()를 다 끝낸
	// 뒤에야 불리므로, 이 시점엔 이동/물리가 이미 다 반영돼있어야 정상임. 그래도 혹시
	// 모를 경우(예: 나중에 실수로 TickGroup을 바꾸거나, 부모 쪽에서 뭔가 놓친 경우)를
	// 대비해 한 번 더 최신화를 요청해둠 - 비용은 더티 플래그 확인 정도라 거의 공짜임.
	if (Owner)
	{
		Owner->ComputeWorldTransform();
	}
}

void UCameraComponent::SetViewMatrix(const FMatrix4x4& view)
{
	UEngine::GetEngine()->GetRenderingManager()->GetRenderer()->SetViewMatrix(view);
}
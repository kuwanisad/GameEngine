// ----------------------------------------------------------------
// From Game Programming in C++ by Sanjay Madhav
// Copyright (C) 2017 Sanjay Madhav. All rights reserved.
// 
// Released under the BSD License
// See LICENSE in root directory for full details.
// ----------------------------------------------------------------

#include "../../Public/Test/PlaneActor.h"
#include "../../Public/Core/Engine.h"
#include "../../Public/Rendering/Renderer.h"
#include "../../Public/Component/StaticMeshComponent.h"
#include "../../Public/Component/BoxComponent.h"
#include "../../Public/Rendering/Mesh.h"
#include "../../Public/Asset/AssetManager.h"
#include "../../Public/Rendering/RenderingManager.h"

APlaneActor::APlaneActor(float InScale, bool bWithMesh, bool bWithCollision)
	:AActor()
{
	UMesh* mesh = UAssetManager::Get()->LoadMesh("Assets/Plane.gpmesh");

	UStaticMeshComponent* mc = nullptr;
	if (bWithMesh)
	{
		// [수정] new XComponent(this) 직접 생성은 Init()이 안 불려서 등록/콜리전 생성이
		// 빠짐 - UEngine::CreateComponent<T>()로 교체.
		mc = UEngine::GetEngine()->CreateComponent<UStaticMeshComponent>(this);
		mc->SetMesh(mesh);
	}

	mBox = nullptr;
	if (bWithCollision)
	{
		mBox = UEngine::GetEngine()->CreateComponent<UBoxComponent>(this);
		mBox->SetObjectBox(mesh->GetBox());
		// [버그 수정] 이게 없으면 콜리전 박스가 액터 회전(벽을 세우려고 90도 돌리는 것
		// 포함)과 무관하게 항상 축 정렬 상태로 남아있음 - 벽은 시각적으로만 세워져
		// 보이고, 실제 콜리전은 원본 메시 모양(얇은 판) 그대로 벽 위치에 눕게 됨. 이러면
		// 플레이어 Z가 그 얇은 판과 우연히 안 겹치면 그냥 통과되고(벽 통과 버그), 겹치면
		// 겹침 해소가 수평이 아니라 수직 방향으로 밀어버릴 수 있음(눈높이가 훅 낮아지는
		// 버그) - 둘 다 실제로 확인됨.
		mBox->SetShouldRotate(true);
	}

	// [부착 계층] 콜리전이 있으면 그게 루트(위치/회전의 기준)가 되고, 메시는 거기 부착.
	// 콜리전이 없고 메시만 있으면(순수 렌더링용 타일) 메시 자체가 루트가 됨.
	if (mBox)
	{
		SetRootComponent(mBox);
		if (mc)
		{
			mc->SetupAttachment(mBox);
		}
	}
	else if (mc)
	{
		SetRootComponent(mc);
	}

	// [순서 중요] 위의 SetRootComponent보다 반드시 뒤에 와야 함 - 먼저 부르면 그 시점의
	// (곧 버려질) 기본 루트한테 스케일이 적용되고, 새 루트로 안 넘어가서 사라짐.
	SetActorScale(FVector3::One * InScale);

	if (mBox != nullptr)
	{
		FAABB CollisionBox = mesh->GetBox();
		// [수정 - 콜리전에 최소 두께 부여] 평면 메시 자체는 두께가 0인 게 정상이지만,
		// 그 값을 콜리전에 그대로 쓰면 슬랩 테스트가 "정확히 그 평면"을 관통해야만
		// 충돌로 잡혀서, 부동소수점 오차로 거의 항상 놓치게 됨(레이캐스트가 계속
		// bHit=0만 내던 원인). 콜리전 전용으로 아주 얇은(하지만 0은 아닌) 두께를
		// 인위적으로 줌 - 시각적 두께와는 무관, 순전히 물리적 안정성을 위한 것.
		const float MinCollisionThickness = 1.0f;
		if (CollisionBox.GetExtent().z < MinCollisionThickness)
		{
			FVector3 Min = CollisionBox.GetMin();
			FVector3 Max = CollisionBox.GetMax();
			Min.z -= MinCollisionThickness;
			Max.z += MinCollisionThickness;
			CollisionBox.SetMin(Min);
			CollisionBox.SetMax(Max);
		}
		mBox->SetObjectBox(CollisionBox);
	}
}

APlaneActor::~APlaneActor()
{
}

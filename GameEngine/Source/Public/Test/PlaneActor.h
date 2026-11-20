// ----------------------------------------------------------------
// From Game Programming in C++ by Sanjay Madhav
// Copyright (C) 2017 Sanjay Madhav. All rights reserved.
// 
// Released under the BSD License
// See LICENSE in root directory for full details.
// ----------------------------------------------------------------

#pragma once
#include "../Actor/Actor.h"

class APlaneActor : public AActor
{
public:
	// InScale: 크기 배율. bWithMesh/bWithCollision: 메시(렌더링)와 콜리전(물리)을 각각
	// 독립적으로 켜고 끌 수 있음 - 기본값(둘 다 true)은 예전 동작 그대로. 렌더링용 작은
	// 타일과 콜리전용 큰 판을 분리해서 쓰려고 추가함(Game::LoadData 참고) - 실제 게임(UE
	// 포함)도 보통 시각적 타일은 잘게, 콜리전은 훨씬 단순하고 굵게 따로 둠. 우리는 예전에
	// 이 둘을 하나로 묶어서, 콜리전이 타일 경계를 넘나들 때마다 접촉 정체성이 끊기는
	// 문제가 있었음(실제로 확인됨: 물체가 타일 경계 근처에서 흔들리면 "담당 타일"이
	// 바뀌면서 정지 마찰 고정점이 계속 리셋됨).
	APlaneActor(float InScale = 10.0f, bool bWithMesh = true, bool bWithCollision = true);
	~APlaneActor();
	class UBoxComponent* GetBox() { return mBox; }
private:
	class UBoxComponent* mBox;
};
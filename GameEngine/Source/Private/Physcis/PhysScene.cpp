#include "../../Public/Physics/PhysScene.h"

FPhysScene::FPhysScene()
{
}

FPhysScene::~FPhysScene()
{
}

bool FPhysScene::RegisterBody(AActor* _actor)
{
	//TODO
	return false;
}

void FPhysScene::UnregisterBody(AActor* _actor)
{
	//TODO
}

void FPhysScene::MoveActor(AActor* _Actor, const FVector3& DesiredPos, float margin)
{
	//TOOD
}

FPhysScene* FPhysScene::Get()
{
	if (Instance == nullptr)
	{
		Instance = new FPhysScene();
	}

	return Instance;
}

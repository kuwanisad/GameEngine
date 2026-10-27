#include "../../Public/Core/Engine.h"
#include "../../Public/Physics/PhysScene.h"
#include "../../Public/Tick/TickManager.h"
#include "../../Public/Input/InputSubsystem.h"

#include <algorithm>

UEngine* UEngine::Instance = nullptr;

UEngine::UEngine()
	: IsRunning(true),
	IsUpdatingActors(false),
	TickCount(0),
	PhysScene(nullptr)
{
}

UEngine::~UEngine()
{
}

bool UEngine::Initialize()
{
	return false;
}

void UEngine::ProcessInput()
{
}

void UEngine::UpdateGame()
{
}

void UEngine::GenerateOutput()
{
}

void UEngine::LoadData()
{
}

void UEngine::UnloadData()
{
}

void UEngine::ResetAccumulatedForceThisTick()
{
}

void UEngine::RunLoop()
{
}

void UEngine::Shutdown()
{
}

UEngine* UEngine::GetEngine()
{
	if (Instance == nullptr)
	{
		Instance = new UEngine();
	}

	return Instance;
}

FPhysScene* UEngine::GetPhysScene()
{
	if (PhysScene == nullptr)
	{
		PhysScene = new FPhysScene();
	}

	return PhysScene;
}

UTickManager* UEngine::GetTickManager()
{
	if (TickManager == nullptr)
	{
		TickManager = new UTickManager();
	}

	return TickManager;
}

UInputSubsystem* UEngine::GetInputSubsystem()
{
	if (InputSubsystem == nullptr)
	{
		InputSubsystem = new UInputSubsystem();
	}

	return InputSubsystem;
}

void UEngine::AddActor(AActor* InActor)
{
	if (IsUpdatingActors)
	{
		PendingActors.emplace_back(InActor);
	}
	else
	{
		Actors.emplace_back(InActor);
	}
}

void UEngine::RemoveActor(AActor* InActor)
{
	auto iter = std::find(PendingActors.begin(), PendingActors.end(), InActor);
	if (iter != PendingActors.end())
	{
		std::iter_swap(iter, PendingActors.end() - 1);
		PendingActors.pop_back();
	}

	iter = std::find(Actors.begin(), Actors.end(), InActor);
	if (iter != Actors.end())
	{
		std::iter_swap(iter, Actors.end() - 1);
		Actors.pop_back();
	}
}

void UEngine::AddSprite(SpriteComponent* _Sprite)
{
}

void UEngine::RemoveSprite(SpriteComponent* _Sprite)
{
}

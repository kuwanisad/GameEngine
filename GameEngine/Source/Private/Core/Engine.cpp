#include "../../Public/Core/Engine.h"
#include <cstdio>
#include <algorithm>

UEngine::UEngine()
{
}

UEngine::~UEngine()
{
}

UEngine* UEngine::Get()
{
	if (Instance == nullptr)
	{
		Instance = new UEngine();
	}

	return Instance;
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

void UEngine::RemoveActor(AActor* _Actor)
{
	auto iter = std::find(PendingActors.begin(), PendingActors.end(), _Actor);
	if (iter != PendingActors.end())
	{
		std::iter_swap(iter, PendingActors.end() - 1);
		PendingActors.pop_back();
	}

	iter = std::find(Actors.begin(), Actors.end(), _Actor);
	if (iter != Actors.end())
	{
		std::iter_swap(iter, Actors.end() - 1);
		Actors.pop_back();
	}
}

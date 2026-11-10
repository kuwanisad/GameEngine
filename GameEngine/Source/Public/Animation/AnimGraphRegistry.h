#pragma once
#include "AnimGraphBase.h"
#include "../Core/CoreMinimal.h"

using FAnimGraphHandle = uint32;
using FAnimGraphFactoryFunc = IAnimGraphBase * (*)();

class UAnimGraphRegistry
{
private:
	FAnimGraphHandle NextHandle = 0;
	TMap<FAnimGraphHandle, FAnimGraphFactoryFunc> Factories;

public:
	static UAnimGraphRegistry& Get()
	{
		static UAnimGraphRegistry Instance;
		return Instance;
	}

	// 엔진 초기화 시점에 필요한 객체마다 자기 그래프 생성 함수 등록
	FAnimGraphHandle RegisterFactory(FAnimGraphFactoryFunc Builder)
	{
		FAnimGraphHandle Handle = NextHandle++;
		Factories[Handle] = Builder;
		return Handle;
	}

	IAnimGraphBase* CreateGraph(FAnimGraphHandle Handle) const
	{
		auto it = Factories.find(Handle);
		return (it != Factories.end()) ? it->second() : nullptr;
	}
};
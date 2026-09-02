#pragma once
#include <cstdint>

#include "../Core/CoreMinimal.h"

enum class ETickingGroup : uint8_t
{
	TG_PrePhysics,                    // input, actor tick
	TG_DuringPhysics,                // It's in Unreal, but I don't use it in this project... Because of too much work...
	TG_DuringPhysics_ComponentTick,
	TG_DuringPhysics_Physics,        // Transform, Physics

	TG_PostPhysics,                  // It's in Unreal, but I don't use it in this project. 
	TG_PostUpdateWork,               // After Camera Update
	TG_Max
};

constexpr int32 NumTickingGroups = static_cast<int32>(ETickingGroup::TG_Max);

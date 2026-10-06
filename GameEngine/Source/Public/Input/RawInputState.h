#pragma once
#include "../Core/CoreMinimal.h"
#include "Keys.h"
#include <unordered_map>

//UE에 없고 SDL 입력에 대응하기 위해 추가
struct FRawInputState
{
public:
	void SetPressed(EKeys Key, bool bPressed) { PressedKeys[Key] = bPressed; }
	void SetAxis(EKeys Key, float Value) { AxisValues[Key] = Value; }

	bool IsPressed(EKeys Key) const
	{
		auto it = PressedKeys.find(Key);
		return (it != PressedKeys.end()) ? it->second : false;
	}

	float GetAxis(EKeys Key) const
	{
		auto it = AxisValues.find(Key);
		return (it != AxisValues.end()) ? it->second : 0.0f;
	}

private:
	TMap<EKeys, bool> PressedKeys;
	TMap<EKeys, float> AxisValues;
};
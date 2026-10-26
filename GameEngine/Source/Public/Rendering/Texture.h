#pragma once
#include "../Core/CoreMinimal.h"

class UTexture
{
private:
	uint32 ID;
	int32 Width;
	int32 Height;

public:
	bool Load(const FString& FileName);
	void CreateWhite1x1();
	void Unload();

	void SetActive();

	int GetWidth() const { return Width; }
	int GetHeight() const { return Height; }
};
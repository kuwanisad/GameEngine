#pragma once
#include "../Core/CoreMinimal.h"

class UTexture
{
private:
	uint32 ID;
	int32 width;
	int32 Height;

	FString Name;

public:
	UTexture();
	~UTexture();

	void SetName(const FString& InName);
	FString GetName() const { return Name; }

	void SetSize(int32 InWidth, int32 InHeight);

	void SetID(uint32 InID);
	uint32 GetID() const { return ID; }

	void SetActive();

	void Unload();

	int GetWidth() const { return width; }
	int GetHeight() const { return Height; }
};
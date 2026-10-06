#pragma once
#include "../Core/CoreMinimal.h"
#include "EnhancedActionKeyMapping.h"

class UInputMappingContext
{
private:
	TArray<FEnhancedActionKeyMapping> Mappings;

public:
	~UInputMappingContext()
	{
		for (auto& Mapping : Mappings)
		{
			for (auto* Modifier : Mapping.Modifiers)
			{
				delete Modifier;
			}
		}
	}

	void AddKeyMapping(EKeys Key, UInputAction* Action, TArray<UInputModifier*> Modifiers = {})
	{
		Mappings.emplace_back(Key, Action, Modifiers);
	}

	const TArray<FEnhancedActionKeyMapping>& GetMappings() const { return Mappings; }
};
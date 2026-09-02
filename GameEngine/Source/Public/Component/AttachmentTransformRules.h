#pragma once
#include "../../Public/Core/CoreMinimal.h"

enum class EAttachmentRule : uint8
{
	KeepRelative,
	KeepWorld,      // Dummy
	SnapToTarget,   // Dummy
};

struct FAttachmentTransformRules
{
	EAttachmentRule LocationRule;
	EAttachmentRule RotationRule;
	EAttachmentRule ScaleRule;

	FAttachmentTransformRules(EAttachmentRule Rule)
		: LocationRule(Rule), RotationRule(Rule), ScaleRule(Rule)
	{
	}

	FAttachmentTransformRules(EAttachmentRule InLocationRule, EAttachmentRule InRotationRule, EAttachmentRule InScaleRule)
		: LocationRule(InLocationRule), RotationRule(InRotationRule), ScaleRule(InScaleRule)
	{
	}

	static const FAttachmentTransformRules KeepRelativeTransform;
	static const FAttachmentTransformRules KeepWorldTransform;
	static const FAttachmentTransformRules SnapToTargetTransform;
};

enum class EDetachmentRule : uint8
{
	KeepRelative,
	KeepWorld,
};

struct FDetachmentTransformRules
{
	EDetachmentRule LocationRule;
	EDetachmentRule RotationRule;
	EDetachmentRule ScaleRule;

	FDetachmentTransformRules(EDetachmentRule Rule)
		: LocationRule(Rule), RotationRule(Rule), ScaleRule(Rule)
	{
	}

	static const FDetachmentTransformRules KeepRelativeTransform;
	static const FDetachmentTransformRules KeepWorldTransform;
};
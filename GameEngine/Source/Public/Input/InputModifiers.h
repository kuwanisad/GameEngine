#pragma once
#include "InputModifier.h"

//UE의 일부 InputModifier를 구현

class UInputModifierNegate : public UInputModifier
{
public:
	FInputActionValue ModifyRaw(const FInputActionValue& CurrentValue) override
	{
		FInputActionValue Result = CurrentValue;
		Result.Value = FVector3(-CurrentValue.Value.x, -CurrentValue.Value.y, -CurrentValue.Value.z);
		return Result;
	}
};

class UInputModifierScalar : public UInputModifier
{
public:
	UInputModifierScalar(float InScalar) : Scalar(InScalar) {}

	FInputActionValue ModifyRaw(const FInputActionValue& CurrentValue) override
	{
		FInputActionValue Result = CurrentValue;
		Result.Value = CurrentValue.Value * Scalar;
		return Result;
	}

private:
	float Scalar;
};

//clamp
class UInputModifierDeadZone : public UInputModifier
{
private:
	float LowerThreshold;
	float UpperThreshold;

public:
	UInputModifierDeadZone(float InLowerThreshold = 0.2f, float InUpperThreshold = 1.0f)
		: LowerThreshold(InLowerThreshold)
		, UpperThreshold(InUpperThreshold)
	{}

	FInputActionValue ModifyRaw(const FInputActionValue& CurrentValue) override
	{
		const FVector3& v = CurrentValue.Value;
		float mag = Math::Sqrt(v.x * v.x + v.y * v.y + v.z * v.z);

		FInputActionValue result = CurrentValue;

		if (mag < LowerThreshold)
		{
			result.Value = FVector3::Zero;
			return result;
		}

		float val = (mag - LowerThreshold) / (UpperThreshold - LowerThreshold);
		val = val > 1.f ? 1.f : mag;
		float scale = mag > 0.f ? (val / mag) : 0.f;

		result.Value = v * scale;
		return result;
	}
};

enum class EInputAxisSwizzle
{
	YXZ,
	ZYX,
	XZY,
	YZX,
	ZXY,
};

class UInputModifierSwizzleAxis : public UInputModifier
{
public:
	UInputModifierSwizzleAxis(EInputAxisSwizzle InOrder = EInputAxisSwizzle::YXZ)
		: Order(InOrder)
	{
	}

	FInputActionValue ModifyRaw(const FInputActionValue& CurrentValue) override
	{
		const FVector3& V = CurrentValue.Value;
		FVector3 Swizzled;

		switch (Order)
		{
		case EInputAxisSwizzle::YXZ: Swizzled = FVector3(V.y, V.x, V.z); break;
		case EInputAxisSwizzle::ZYX: Swizzled = FVector3(V.z, V.y, V.x); break;
		case EInputAxisSwizzle::XZY: Swizzled = FVector3(V.x, V.z, V.y); break;
		case EInputAxisSwizzle::YZX: Swizzled = FVector3(V.y, V.z, V.x); break;
		case EInputAxisSwizzle::ZXY: Swizzled = FVector3(V.z, V.x, V.y); break;
		}

		FInputActionValue Result = CurrentValue;
		Result.Value = Swizzled;
		return Result;
	}

private:
	EInputAxisSwizzle Order;
};
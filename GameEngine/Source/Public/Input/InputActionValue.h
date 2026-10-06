#pragma once
#include "../../Public/Core/Math.h"

enum class EInputActionValueType
{
	Boolean,
	Axis1D,
	Axis2D,
	Axis3D,
};

struct FInputActionValue
{
	FVector3 Value;
	EInputActionValueType ValueType;

	FInputActionValue() 
		: Value(0.f, 0.f, 0.f),
		ValueType(EInputActionValueType::Axis1D)
	{

	}

	explicit FInputActionValue(float InValue)
		: Value(InValue, 0.0f, 0.0f)
		, ValueType(EInputActionValueType::Axis1D)
	{
	}

	FInputActionValue(float InX, float InY)
		: Value(InX, InY, 0.0f)
		, ValueType(EInputActionValueType::Axis2D)
	{
	}

	FInputActionValue(EInputActionValueType InType, const FVector3& InValue)
		: Value(InValue)
		, ValueType(InType)
	{
	}
	
	FInputActionValue& operator+=(const FInputActionValue& Rhs)
	{
		Value = Value + Rhs.Value;
		if (static_cast<int>(Rhs.ValueType) > static_cast<int>(ValueType))
		{
			ValueType = Rhs.ValueType;
		}
		return *this;
	}

	template<typename T>
	T Get() const;

	bool IsNonZero(float Tolerance = 0.0001f) const
	{
		float MagnitudeSq = Value.x * Value.x + Value.y * Value.y + Value.z * Value.z;
		return MagnitudeSq >= Tolerance * Tolerance;
	}
};

template<typename T>
inline T FInputActionValue::Get() const
{
	return T();
}

template<>
inline bool FInputActionValue::Get() const
{
	return Math::NearZero(Value.x) ? false : true;
}

template<>
inline float FInputActionValue::Get() const
{
	return Value.x;
}

template<>
inline FVector2 FInputActionValue::Get() const
{
	return FVector2(Value.x, Value.y);
}

template<>
inline FVector3 FInputActionValue::Get() const
{
	return Value;
}
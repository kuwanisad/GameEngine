#pragma once
#include "../Input/InputAction.h"
#include "../Input/InputActionValue.h"

//UE의 ThirdPerson이 목표이므로 그에 맞는 입력을 마련했음
namespace GameplayInputActions
{
	inline UInputAction& Move()
	{
		static UInputAction Instance("Move", EInputActionValueType::Axis2D);
		return Instance;
	}

	inline UInputAction& Forward()
	{
		static UInputAction Instance("Forward", EInputActionValueType::Axis1D);
		return Instance;
	}

	inline UInputAction& Turn()
	{
		static UInputAction Instance("Turn", EInputActionValueType::Axis1D);
		return Instance;
	}

	inline UInputAction& LookUp()
	{
		static UInputAction Instance("LookUp", EInputActionValueType::Axis1D);
		return Instance;
	}

	inline UInputAction& Jump()
	{
		static UInputAction Instance("Jump", EInputActionValueType::Boolean);
		return Instance;
	}
}

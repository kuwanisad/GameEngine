#pragma once
#include "CoreMinimal.h"

#include <cmath>
#include <memory.h>
#include <limits>
#include <stdexcept>

namespace Math
{
	const float Pi = 3.1415926535f;
	const float Tau = Pi * 2.f;
	const float Infinity = std::numeric_limits<float>::infinity();
	const float NegInfinity = -std::numeric_limits<float>::infinity();

	inline float ToRadians(float degrees)
	{
		return degrees * (Pi / 180.f);
	}

	inline float ToDegrees(float radians)
	{
		return radians * (180.f / Pi);
	}

	inline float Ceil(float val)
	{
		return std::ceil(val);
	}

	inline bool NearZero(float val, float epsilon = 0.001f)
	{
		if (fabs(val) <= epsilon)
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	template <typename T>
	T Max(const T& a, const T& b)
	{
		return (a < b ? b : a);
	}

	template <typename T>
	T Min(const T& a, const T& b)
	{
		return (a < b ? a : b);
	}

	template <typename T>
	T Clamp(const T& value, const T& lower, const T& upper)
	{
		return Min(upper, Max(lower, value));
	}

	inline float Abs(float value)
	{
		return fabs(value);
	}

	inline float Cos(float angle)
	{
		return cosf(angle);
	}

	inline float Sin(float angle)
	{
		return sinf(angle);
	}

	inline float Tan(float angle)
	{
		return tanf(angle);
	}

	inline float Acos(float value)
	{
		return acosf(value);
	}

	inline float Atan2(float y, float x)
	{
		return atan2f(y, x);
	}

	inline float Cot(float angle)
	{
		return 1.0f / Tan(angle);
	}

	inline float Lerp(float a, float b, float f)
	{
		return a + f * (b - a);
	}

	inline float Sqrt(float value)
	{
		return sqrtf(value);
	}

	inline float Fmod(float numer, float denom)
	{
		return fmod(numer, denom);
	}
}

struct FVector2
{
public:
	float x;
	float y;

	FVector2() :x(0.0f), y(0.0f)
	{}

	explicit FVector2(float inX, float inY) :x(inX), y(inY)
	{}

	void Set(float inX, float inY)
	{
		x = inX;
		y = inY;
	}

	friend FVector2 operator+(const FVector2& a, const FVector2& b)
	{
		return FVector2(a.x + b.x, a.y + b.y);
	}

	friend FVector2 operator-(const FVector2& a, const FVector2& b)
	{
		return FVector2(a.x - b.x, a.y - b.y);
	}

	friend FVector2 operator*(const FVector2& a, const FVector2& b)
	{
		return FVector2(a.x * b.x, a.y * b.y);
	}

	friend FVector2 operator*(const FVector2& vec, float scalar)
	{
		return FVector2(vec.x * scalar, vec.y * scalar);
	}

	friend FVector2 operator*(float scalar, const FVector2& vec)
	{
		return FVector2(vec.x * scalar, vec.y * scalar);
	}

	FVector2& operator*=(float scalar)
	{
		x *= scalar;
		y *= scalar;
		return *this;
	}

	FVector2& operator+=(const FVector2& right)
	{
		x += right.x;
		y += right.y;
		return *this;
	}

	FVector2& operator-=(const FVector2& right)
	{
		x -= right.x;
		y -= right.y;
		return *this;
	}

	float LengthSq() const
	{
		return (x * x + y * y);
	}

	float Length() const
	{
		return (Math::Sqrt(LengthSq()));
	}

	void Normalize()
	{
		float length = Length();
		x /= length;
		y /= length;
	}

	static FVector2 Normalize(const FVector2& vec)
	{
		FVector2 temp = vec;
		temp.Normalize();
		return temp;
	}

	static float Dot(const FVector2& a, const FVector2& b)
	{
		return (a.x * b.x + a.y * b.y);
	}

	static FVector2 Lerp(const FVector2& a, const FVector2& b, float f)
	{
		return FVector2(a + f * (b - a));
	}

	static FVector2 Reflect(const FVector2& v, const FVector2& n)
	{
		return v - 2.0f * FVector2::Dot(v, n) * n;
	}

	// return: vec * mat
	static FVector2 Transform(const FVector2& vec, const struct FMatrix3x3& mat, float w = 1.0f);

	static const FVector2 Zero;
	static const FVector2 UnitX;
	static const FVector2 UnitY;
	static const FVector2 NegUnitX;
	static const FVector2 NegUnitY;
};

struct FVector3
{
public:
	float x;
	float y;
	float z;

	FVector3() :x(0.0f), y(0.0f), z(0.0f)
	{}

	explicit FVector3(float inX, float inY, float inZ) :x(inX), y(inY), z(inZ)
	{}

	const float* GetAsFloatPtr() const
	{
		return reinterpret_cast<const float*>(&x);
	}

	void Set(float inX, float inY, float inZ)
	{
		x = inX;
		y = inY;
		z = inZ;
	}

	friend FVector3 operator+(const FVector3& a, const FVector3& b)
	{
		return FVector3(a.x + b.x, a.y + b.y, a.z + b.z);
	}

	friend FVector3 operator-(const FVector3& a, const FVector3& b)
	{
		return FVector3(a.x - b.x, a.y - b.y, a.z - b.z);
	}

	friend FVector3 operator*(const FVector3& left, const FVector3& right)
	{
		return FVector3(left.x * right.x, left.y * right.y, left.z * right.z);
	}

	friend FVector3 operator*(const FVector3& vec, float scalar)
	{
		return FVector3(vec.x * scalar, vec.y * scalar, vec.z * scalar);
	}

	friend FVector3 operator*(float scalar, const FVector3& vec)
	{
		return FVector3(vec.x * scalar, vec.y * scalar, vec.z * scalar);
	}

	friend FVector3 operator/(const FVector3& vec, float scalar)
	{
		return FVector3(vec.x / scalar, vec.y / scalar, vec.z / scalar);
	}

	friend FVector3 operator/=(const FVector3& vec, float scalar)
	{
		return FVector3(vec.x / scalar, vec.y / scalar, vec.z / scalar);
	}

	FVector3 operator-()
	{
		return FVector3(-x, -y, -z);
	}

	float& operator[](int Index)
	{
		switch (Index)
		{
		case 0:
			return x;
		case 1:
			return y;
		case 2:
			return z;
		default:
			throw std::out_of_range("FVector3 []: Index out of bounds");
		}
	}

	FVector3 operator+=(const FVector3& a)
	{
		x += a.x;
		y += a.y;
		z += a.z;

		return *this;
	}

	FVector3& operator*=(float scalar)
	{
		x *= scalar;
		y *= scalar;
		z *= scalar;
		return *this;
	}

	FVector3& operator-=(const FVector3& right)
	{
		x -= right.x;
		y -= right.y;
		z -= right.z;
		return *this;
	}

	float LengthSq() const
	{
		return (x * x + y * y + z * z);
	}

	float Length() const
	{
		return (Math::Sqrt(LengthSq()));
	}

	void Normalize()
	{
		float length = Length();
		x /= length;
		y /= length;
		z /= length;
	}

	static FVector3 Normalize(const FVector3& vec)
	{
		FVector3 temp = vec;
		temp.Normalize();
		return temp;
	}

	static float Dot(const FVector3& a, const FVector3& b)
	{
		return (a.x * b.x + a.y * b.y + a.z * b.z);
	}

	static FVector3 Cross(const FVector3& a, const FVector3& b)
	{
		FVector3 temp;
		temp.x = a.y * b.z - a.z * b.y;
		temp.y = a.z * b.x - a.x * b.z;
		temp.z = a.x * b.y - a.y * b.x;
		return temp;
	}

	static FVector3 Lerp(const FVector3& a, const FVector3& b, float f)
	{
		return FVector3(a + f * (b - a));
	}

	static FVector3 Reflect(const FVector3& v, const FVector3& n)
	{
		return v - 2.0f * FVector3::Dot(v, n) * n;
	}

	//return: vec * mat
	static FVector3 Transform(const FVector3& vec, const struct FMatrix4x4& mat, float w = 1.0f);

	static FVector3 TransformWithPerspDiv(const FVector3& vec, const struct FMatrix4x4& mat, float w = 1.0f);

	static FVector3 Transform(const FVector3& v, const struct FQuaternion& q);

	static const FVector3 Zero;
	static const FVector3 One;
	static const FVector3 UnitX;
	static const FVector3 UnitY;
	static const FVector3 UnitZ;
	static const FVector3 NegUnitX;
	static const FVector3 NegUnitY;
	static const FVector3 NegUnitZ;
	static const FVector3 Infinity;
	static const FVector3 NegInfinity;
};

struct FMatrix3x3
{
public:
	float mat[3][3];

	FMatrix3x3()
	{
		*this = FMatrix3x3::Identity;
	}

	explicit FMatrix3x3(float inMat[3][3])
	{
		memcpy(mat, inMat, 9 * sizeof(float));
	}

	const float* GetAsFloatPtr() const
	{
		return reinterpret_cast<const float*>(&mat[0][0]);
	}

	friend FMatrix3x3 operator*(const FMatrix3x3& left, const FMatrix3x3& right)
	{
		FMatrix3x3 retVal;

		// row 0
		retVal.mat[0][0] =
			left.mat[0][0] * right.mat[0][0] +
			left.mat[0][1] * right.mat[1][0] +
			left.mat[0][2] * right.mat[2][0];

		retVal.mat[0][1] =
			left.mat[0][0] * right.mat[0][1] +
			left.mat[0][1] * right.mat[1][1] +
			left.mat[0][2] * right.mat[2][1];

		retVal.mat[0][2] =
			left.mat[0][0] * right.mat[0][2] +
			left.mat[0][1] * right.mat[1][2] +
			left.mat[0][2] * right.mat[2][2];

		// row 1
		retVal.mat[1][0] =
			left.mat[1][0] * right.mat[0][0] +
			left.mat[1][1] * right.mat[1][0] +
			left.mat[1][2] * right.mat[2][0];

		retVal.mat[1][1] =
			left.mat[1][0] * right.mat[0][1] +
			left.mat[1][1] * right.mat[1][1] +
			left.mat[1][2] * right.mat[2][1];

		retVal.mat[1][2] =
			left.mat[1][0] * right.mat[0][2] +
			left.mat[1][1] * right.mat[1][2] +
			left.mat[1][2] * right.mat[2][2];

		// row 2
		retVal.mat[2][0] =
			left.mat[2][0] * right.mat[0][0] +
			left.mat[2][1] * right.mat[1][0] +
			left.mat[2][2] * right.mat[2][0];

		retVal.mat[2][1] =
			left.mat[2][0] * right.mat[0][1] +
			left.mat[2][1] * right.mat[1][1] +
			left.mat[2][2] * right.mat[2][1];

		retVal.mat[2][2] =
			left.mat[2][0] * right.mat[0][2] +
			left.mat[2][1] * right.mat[1][2] +
			left.mat[2][2] * right.mat[2][2];

		return retVal;
	}

	FMatrix3x3& operator*=(const FMatrix3x3& right)
	{
		*this = *this * right;
		return *this;
	}

	static FMatrix3x3 TransformScaleToMatrix(float ScaleX, float ScaleY)
	{
		float temp[3][3] =
		{
			{ ScaleX, 0.0f, 0.0f },
			{ 0.0f, ScaleY, 0.0f },
			{ 0.0f, 0.0f, 1.0f },
		};
		return FMatrix3x3(temp);
	}

	static FMatrix3x3 TransformScaleToMatrix(const FVector2& scaleVector)
	{
		return TransformScaleToMatrix(scaleVector.x, scaleVector.y);
	}

	static FMatrix3x3 TransformScaleToMatrix(float scale)
	{
		return TransformScaleToMatrix(scale, scale);
	}

	static FMatrix3x3 TrasnsformRotationToMatrix(float theta)
	{
		float temp[3][3] =
		{
			{ Math::Cos(theta), Math::Sin(theta), 0},
			{ -Math::Sin(theta), Math::Cos(theta), 0},
			{0, 0, 1},
		};

		return FMatrix3x3(temp);
	}

	static const FMatrix3x3 Identity;
};

struct FMatrix4x4
{
public:
	float mat[4][4];

	FMatrix4x4()
	{
		*this = FMatrix4x4::Identity;
	}

	explicit FMatrix4x4(float inMat[4][4])
	{
		memcpy(mat, inMat, 16 * sizeof(float));
	}

	// Cast to a const float pointer
	const float* GetAsFloatPtr() const
	{
		return reinterpret_cast<const float*>(&mat[0][0]);
	}

	// Matrix multiplication (a * b)
	friend FMatrix4x4 operator*(const FMatrix4x4& a, const FMatrix4x4& b)
	{
		FMatrix4x4 retVal;
		// row 0
		retVal.mat[0][0] =
			a.mat[0][0] * b.mat[0][0] +
			a.mat[0][1] * b.mat[1][0] +
			a.mat[0][2] * b.mat[2][0] +
			a.mat[0][3] * b.mat[3][0];

		retVal.mat[0][1] =
			a.mat[0][0] * b.mat[0][1] +
			a.mat[0][1] * b.mat[1][1] +
			a.mat[0][2] * b.mat[2][1] +
			a.mat[0][3] * b.mat[3][1];

		retVal.mat[0][2] =
			a.mat[0][0] * b.mat[0][2] +
			a.mat[0][1] * b.mat[1][2] +
			a.mat[0][2] * b.mat[2][2] +
			a.mat[0][3] * b.mat[3][2];

		retVal.mat[0][3] =
			a.mat[0][0] * b.mat[0][3] +
			a.mat[0][1] * b.mat[1][3] +
			a.mat[0][2] * b.mat[2][3] +
			a.mat[0][3] * b.mat[3][3];

		// row 1
		retVal.mat[1][0] =
			a.mat[1][0] * b.mat[0][0] +
			a.mat[1][1] * b.mat[1][0] +
			a.mat[1][2] * b.mat[2][0] +
			a.mat[1][3] * b.mat[3][0];

		retVal.mat[1][1] =
			a.mat[1][0] * b.mat[0][1] +
			a.mat[1][1] * b.mat[1][1] +
			a.mat[1][2] * b.mat[2][1] +
			a.mat[1][3] * b.mat[3][1];

		retVal.mat[1][2] =
			a.mat[1][0] * b.mat[0][2] +
			a.mat[1][1] * b.mat[1][2] +
			a.mat[1][2] * b.mat[2][2] +
			a.mat[1][3] * b.mat[3][2];

		retVal.mat[1][3] =
			a.mat[1][0] * b.mat[0][3] +
			a.mat[1][1] * b.mat[1][3] +
			a.mat[1][2] * b.mat[2][3] +
			a.mat[1][3] * b.mat[3][3];

		// row 2
		retVal.mat[2][0] =
			a.mat[2][0] * b.mat[0][0] +
			a.mat[2][1] * b.mat[1][0] +
			a.mat[2][2] * b.mat[2][0] +
			a.mat[2][3] * b.mat[3][0];

		retVal.mat[2][1] =
			a.mat[2][0] * b.mat[0][1] +
			a.mat[2][1] * b.mat[1][1] +
			a.mat[2][2] * b.mat[2][1] +
			a.mat[2][3] * b.mat[3][1];

		retVal.mat[2][2] =
			a.mat[2][0] * b.mat[0][2] +
			a.mat[2][1] * b.mat[1][2] +
			a.mat[2][2] * b.mat[2][2] +
			a.mat[2][3] * b.mat[3][2];

		retVal.mat[2][3] =
			a.mat[2][0] * b.mat[0][3] +
			a.mat[2][1] * b.mat[1][3] +
			a.mat[2][2] * b.mat[2][3] +
			a.mat[2][3] * b.mat[3][3];

		// row 3
		retVal.mat[3][0] =
			a.mat[3][0] * b.mat[0][0] +
			a.mat[3][1] * b.mat[1][0] +
			a.mat[3][2] * b.mat[2][0] +
			a.mat[3][3] * b.mat[3][0];

		retVal.mat[3][1] =
			a.mat[3][0] * b.mat[0][1] +
			a.mat[3][1] * b.mat[1][1] +
			a.mat[3][2] * b.mat[2][1] +
			a.mat[3][3] * b.mat[3][1];

		retVal.mat[3][2] =
			a.mat[3][0] * b.mat[0][2] +
			a.mat[3][1] * b.mat[1][2] +
			a.mat[3][2] * b.mat[2][2] +
			a.mat[3][3] * b.mat[3][2];

		retVal.mat[3][3] =
			a.mat[3][0] * b.mat[0][3] +
			a.mat[3][1] * b.mat[1][3] +
			a.mat[3][2] * b.mat[2][3] +
			a.mat[3][3] * b.mat[3][3];

		return retVal;
	}

	FMatrix4x4& operator*=(const FMatrix4x4& right)
	{
		*this = *this * right;
		return *this;
	}

	void Invert();

	FVector3 GetTranslation() const
	{
		return FVector3(mat[3][0], mat[3][1], mat[3][2]);
	}

	//forward
	FVector3 GetXAxis() const
	{
		return FVector3::Normalize(FVector3(mat[0][0], mat[0][1], mat[0][2]));
	}

	//left
	FVector3 GetYAxis() const
	{
		return FVector3::Normalize(FVector3(mat[1][0], mat[1][1], mat[1][2]));
	}

	//up
	FVector3 GetZAxis() const
	{
		return FVector3::Normalize(FVector3(mat[2][0], mat[2][1], mat[2][2]));
	}

	FVector3 GetScale() const
	{
		FVector3 retVal;
		retVal.x = FVector3(mat[0][0], mat[0][1], mat[0][2]).Length();
		retVal.y = FVector3(mat[1][0], mat[1][1], mat[1][2]).Length();
		retVal.z = FVector3(mat[2][0], mat[2][1], mat[2][2]).Length();
		return retVal;
	}

	static FMatrix4x4 TransformScaleToMatrix(float xScale, float yScale, float zScale)
	{
		float temp[4][4] =
		{
			{ xScale, 0.0f, 0.0f, 0.0f },
			{ 0.0f, yScale, 0.0f, 0.0f },
			{ 0.0f, 0.0f, zScale, 0.0f },
			{ 0.0f, 0.0f, 0.0f, 1.0f }
		};
		return FMatrix4x4(temp);
	}

	static FMatrix4x4 TransformScaleToMatrix(const FVector3& scaleVector)
	{
		return TransformScaleToMatrix(scaleVector.x, scaleVector.y, scaleVector.z);
	}

	static FMatrix4x4 TransformScaleToMatrix(float scale)
	{
		return TransformScaleToMatrix(scale, scale, scale);
	}

	static FMatrix4x4 CreateRotationX(float theta)
	{
		float temp[4][4] =
		{
			{ 1.0f, 0.0f, 0.0f , 0.0f },
			{ 0.0f, Math::Cos(theta), Math::Sin(theta), 0.0f },
			{ 0.0f, -Math::Sin(theta), Math::Cos(theta), 0.0f },
			{ 0.0f, 0.0f, 0.0f, 1.0f },
		};
		return FMatrix4x4(temp);
	}

	static FMatrix4x4 CreateRotationY(float theta)
	{
		float temp[4][4] =
		{
			{ Math::Cos(theta), 0.0f, -Math::Sin(theta), 0.0f },
			{ 0.0f, 1.0f, 0.0f, 0.0f },
			{ Math::Sin(theta), 0.0f, Math::Cos(theta), 0.0f },
			{ 0.0f, 0.0f, 0.0f, 1.0f },
		};
		return FMatrix4x4(temp);
	}

	static FMatrix4x4 CreateRotationZ(float theta)
	{
		float temp[4][4] =
		{
			{ Math::Cos(theta), Math::Sin(theta), 0.0f, 0.0f },
			{ -Math::Sin(theta), Math::Cos(theta), 0.0f, 0.0f },
			{ 0.0f, 0.0f, 1.0f, 0.0f },
			{ 0.0f, 0.0f, 0.0f, 1.0f },
		};
		return FMatrix4x4(temp);
	}

	static FMatrix4x4 TransformQuaternionToMatrix(const struct FQuaternion& q);

	static FMatrix4x4 TransformTranslationToMatrix(const FVector3& trans)
	{
		float temp[4][4] =
		{
			{ 1.0f, 0.0f, 0.0f, 0.0f },
			{ 0.0f, 1.0f, 0.0f, 0.0f },
			{ 0.0f, 0.0f, 1.0f, 0.0f },
			{ trans.x, trans.y, trans.z, 1.0f }
		};
		return FMatrix4x4(temp);
	}

	static FMatrix4x4 CreateLookAt(const FVector3& eye, const FVector3& target, const FVector3& up)
	{
		FVector3 zaxis = FVector3::Normalize(target - eye);
		FVector3 xaxis = FVector3::Normalize(FVector3::Cross(up, zaxis));
		FVector3 yaxis = FVector3::Normalize(FVector3::Cross(zaxis, xaxis));
		FVector3 trans;
		trans.x = -FVector3::Dot(xaxis, eye);
		trans.y = -FVector3::Dot(yaxis, eye);
		trans.z = -FVector3::Dot(zaxis, eye);

		float temp[4][4] =
		{
			{ xaxis.x, yaxis.x, zaxis.x, 0.0f },
			{ xaxis.y, yaxis.y, zaxis.y, 0.0f },
			{ xaxis.z, yaxis.z, zaxis.z, 0.0f },
			{ trans.x, trans.y, trans.z, 1.0f }
		};
		return FMatrix4x4(temp);
	}

	static FMatrix4x4 CreateOrtho(float width, float height, float near, float far)
	{
		float temp[4][4] =
		{
			{ 2.0f / width, 0.0f, 0.0f, 0.0f },
			{ 0.0f, 2.0f / height, 0.0f, 0.0f },
			{ 0.0f, 0.0f, 1.0f / (far - near), 0.0f },
			{ 0.0f, 0.0f, near / (near - far), 1.0f }
		};
		return FMatrix4x4(temp);
	}

	static FMatrix4x4 CreatePerspectiveFOV(float fovY, float width, float height, float near, float far)
	{
		float yScale = Math::Cot(fovY / 2.0f);
		float xScale = yScale * height / width;
		float temp[4][4] =
		{
			{ xScale, 0.0f, 0.0f, 0.0f },
			{ 0.0f, yScale, 0.0f, 0.0f },
			{ 0.0f, 0.0f, far / (far - near), 1.0f },
			{ 0.0f, 0.0f, -near * far / (far - near), 0.0f }
		};
		return FMatrix4x4(temp);
	}

	static const FMatrix4x4 Identity;
};

/*
u is unit vector
u = (u_x, u_y, u_z)

q = (cos(θ/2), u_x * sin(θ/2), u_y * sin(θ/2), u_z * sin(θ/2))
q = (w, x, y, z)
*/
struct FQuaternion
{
public:
	float x;
	float y;
	float z;
	float w;

	FQuaternion()
	{
		*this = FQuaternion::Identity;
	}

	// This directly sets the quaternion components --
	// don't use for axis/angle
	explicit FQuaternion(float inX, float inY, float inZ, float inW)
	{
		Set(inX, inY, inZ, inW);
	}

	explicit FQuaternion(const FVector3& axis, float angle)
	{
		/*
		axis_norm = axis.normalize();
		q = [cos(angle/2), axis_norm * sin(angle/2)]
		*/
		float scalar = Math::Sin(angle / 2.0f);
		x = axis.x * scalar;
		y = axis.y * scalar;
		z = axis.z * scalar;
		w = Math::Cos(angle / 2.0f);
	}

	void Set(float inX, float inY, float inZ, float inW)
	{
		x = inX;
		y = inY;
		z = inZ;
		w = inW;
	}

	void Conjugate()
	{
		x *= -1.0f;
		y *= -1.0f;
		z *= -1.0f;
	}

	float LengthSq() const
	{
		return (x * x + y * y + z * z + w * w);
	}

	float Length() const
	{
		return Math::Sqrt(LengthSq());
	}

	void Normalize()
	{
		float length = Length();
		x /= length;
		y /= length;
		z /= length;
		w /= length;
	}

	// Normalize the provided quaternion
	static FQuaternion Normalize(const FQuaternion& q)
	{
		FQuaternion retVal = q;
		retVal.Normalize();
		return retVal;
	}

	// Linear interpolation
	static FQuaternion Lerp(const FQuaternion& a, const FQuaternion& b, float f)
	{
		FQuaternion retVal;
		retVal.x = Math::Lerp(a.x, b.x, f);
		retVal.y = Math::Lerp(a.y, b.y, f);
		retVal.z = Math::Lerp(a.z, b.z, f);
		retVal.w = Math::Lerp(a.w, b.w, f);
		retVal.Normalize();
		return retVal;
	}

	static float Dot(const FQuaternion& a, const FQuaternion& b)
	{
		return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
	}

	/*
	result =
    sin((1-f)ω) / sin(ω) × a
	+ sin(fω) / sin(ω) × b
	*/
	static FQuaternion Slerp(const FQuaternion& a, const FQuaternion& b, float f)
	{
		//Angle between a and b
		float tempCosAngle = FQuaternion::Dot(a, b);

		float cosAngle = tempCosAngle >0 ? tempCosAngle  : -tempCosAngle;

		float scale0, scale1;

		if (cosAngle < 0.9999f)
		{
			const float omega = Math::Acos(cosAngle);
			const float invSin = 1 / Math::Sin(omega);
			scale0 = sin((1.f - f) * omega) * invSin;
			scale1 = (f * omega) * invSin;
		}
		else
		{
			//if the quaternions are collinear
			scale0 = 1.0f - f;
			scale1 = f;
		}

		if (tempCosAngle < 0.0f)
		{
			scale1 = -scale1;
		}

		FQuaternion retVal;
		retVal.x = scale0 * a.x + scale1 * b.x;
		retVal.y = scale0 * a.y + scale1 * b.y;
		retVal.z = scale0 * a.z + scale1 * b.z;
		retVal.w = scale0 * a.w + scale1 * b.w;
		retVal.Normalize();
		return retVal;
	}

	/*Quaternion x Quaternion*/
	static FQuaternion Concatenate(const FQuaternion& q, const FQuaternion& p)
	{
		FQuaternion retVal;

		FVector3 qv(q.x, q.y, q.z);
		FVector3 pv(p.x, p.y, p.z);
		FVector3 newVec = p.w * qv + q.w * pv + FVector3::Cross(pv, qv);
		retVal.x = newVec.x;
		retVal.y = newVec.y;
		retVal.z = newVec.z;

		retVal.w = p.w * q.w - FVector3::Dot(pv, qv);

		return retVal;
	}

	static const FQuaternion Identity;
};

// UE FTransform
struct FTransform
{
public:
	FQuaternion Rotation;
	FVector3 Translation;
	FVector3 Scale3D;

	FTransform()
		: Rotation(FQuaternion::Identity)
		, Translation(FVector3::Zero)
		, Scale3D(FVector3(1.0f, 1.0f, 1.0f))
	{
	}

	FTransform(const FQuaternion& InRotation, const FVector3& InTranslation, const FVector3& InScale3D = FVector3(1.0f, 1.0f, 1.0f))
		: Rotation(InRotation)
		, Translation(InTranslation)
		, Scale3D(InScale3D)
	{
	}

	FMatrix4x4 ToMatrix() const;

	static FTransform Multiply(const FTransform& A, const FTransform& B);

	static const FTransform Identity;
};

namespace Color
{
	static const FVector3 Black(0.0f, 0.0f, 0.0f);
	static const FVector3 White(1.0f, 1.0f, 1.0f);
	static const FVector3 Red(1.0f, 0.0f, 0.0f);
	static const FVector3 Green(0.0f, 1.0f, 0.0f);
	static const FVector3 Blue(0.0f, 0.0f, 1.0f);
	static const FVector3 Yellow(1.0f, 1.0f, 0.0f);
	static const FVector3 LightYellow(1.0f, 1.0f, 0.88f);
	static const FVector3 LightBlue(0.68f, 0.85f, 0.9f);
	static const FVector3 LightPink(1.0f, 0.71f, 0.76f);
	static const FVector3 LightGreen(0.56f, 0.93f, 0.56f);
}
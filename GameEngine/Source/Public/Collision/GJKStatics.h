#pragma once
#include "Collision.h"
#include "../Core/CoreMinimal.h"
#include "../Core/PCH.h"

#include <functional>

/*
두 볼록 다각형의 중첩 검사
각 도형을 A, B라고 할 때 A, B는 점 집합으로 간주 가능하며 중첩 시 어느 A, B의 점 a, b에 대해서 a - b가 0이 됨
그 '어느 점'을 찾는 방법은 못 찾았지만 특정 구역에 존재 여부를 판별하는 방법을 찾아냈고 그게 GJK
각 도형의 특정 방향에서 가장 먼 점을 찾는 함수 support(볼록 다각형, 방향)가 필요함
support(A, b) - support(B, -d)를 통해 해당 방향에서 가장 먼 벡터를 얻는 방식을 반복해 얻은 벡터들로 생성된 다면체 안에 영벡터 포함 여부를 검사하면 됨
*/
struct GJKStatics
{
private:
	using SupportFunc = std::function<FVector3(const FVector3&)>;


public:
	static bool Intersect(const FCollision& A, const FCollision& B);

	static bool SweepIntersect(const FCollision& A, const FVector3& Displacement, const FCollision& B, float& outT);

	static bool RunGJK(const SupportFunc& MinkowskiSupport);

	/*다면체 내부 영벡터 존재 여부 판별용
	points의 인자는 새 벡터를 인덱스 0에 삽입해야 함.*/
	static bool Simplex(TArray<FVector3>& points, FVector3& direction);
	static bool SimplexLine(TArray<FVector3>& points, FVector3& direction);
	static bool SimplexTriangle(TArray<FVector3>& points, FVector3& direction);
	static bool SimplexTetrahedron(TArray<FVector3>& points, FVector3& direction);

	//해당 방향과 벡터 사이 각도가 예각이면 true
	static bool SameDirection(const FVector3& direction, const FVector3& v);

	static FVector3 SafeNormalize(const FVector3& v);

	static const int MaxIterations;
	static const int BisectionIterations;
};
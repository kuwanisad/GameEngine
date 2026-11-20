#pragma once
#include "../Core/CoreMinimal.h"
#include "../Core/Math.h"

struct FTriangleIndices
{
	// [주의 - 좌표가 아니라 인덱스] A/B/C는 Vector2 좌표값이 아니라, 원본
	// Points 배열에서 "몇 번째 점인지"를 가리키는 번호다. 같은 점이 여러
	// 삼각형에 동시에 걸쳐있을 수 있어서(예: 한 점을 공유하는 부채꼴 모양의
	// 삼각형 여러 개), 좌표를 직접 중복 저장하지 않고 인덱스로만 "가리키기"
	// 위함이다.
	int32 A = 0;
	int32 B = 0;
	int32 C = 0;
};

TArray<FTriangleIndices> ComputeDelaunayTriangulation2D(const TArray<FVector2>& Points);

struct FBarycentricWeights
{
	float WeightA = 0.0f;
	float WeightB = 0.0f;
	float WeightC = 0.0f;
};

FBarycentricWeights ComputeBarycentricWeights(const FVector2& P, const FVector2& A, const FVector2& B, const FVector2& C, bool& bOutValid);

bool IsPointInTriangle(const FBarycentricWeights& Weights, float Epsilon = 1e-4f);

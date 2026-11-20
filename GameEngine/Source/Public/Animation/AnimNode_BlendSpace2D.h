#pragma once
#include "AnimNode_Base.h"
#include "AnimNode_SequencePlayer.h"
#include "../Utilities/DelaunayTriangulation2D.h"
#include "../Utilities/BlendSpaceManifest.h"

#include <functional>
#include <algorithm>
#include <cmath>
#include <utility>

// UE의 2D 블렌드 스페이스에 대응(축소판, 원형/각도 감싸기 없음 - 확인된 대로
// 순수 (X, Y) 평면). 27개 같은 샘플 점들을 들로네 삼각분할해두고, 매 프레임 입력
// 좌표가 어느 삼각형 안에 있는지 찾아 그 세 꼭짓점만 무게중심 좌표로 블렌딩함.
//
// [Rate Scaling 없음 - 확정] 블렌드되는 샘플들의 재생 속도를 서로 맞추는 보정은
// 안 함 - 각 샘플이 자기 재생 시각(t)에서 그냥 독립적으로 계산되고 블렌딩됨.
// 발 미끄러짐이 생길 수 있다는 걸 감수하기로 확정됨.
//
// [TODO/README 후보 - 스냅샷 직접 접근] GetBlendInput이 TSnapshot에서 매 프레임
// 직접 값을 꺼내옴 - 전이 조건과 같은 패턴이라 구조는 일관되지만, 비효율적이라는
// 지적이 있었음(스냅샷 자체는 이미 값 복사본이라 매번 새로 꺼내는 비용 자체는
// 크지 않지만, "그래프 노드가 스냅샷 내부 구조를 직접 알아야 한다"는 결합도 문제).
// 나중에 값을 캐싱하거나 별도 인터페이스로 분리하는 걸 고려할 것.
template<typename TSnapshot>
class FAnimNode_BlendSpace2D : public UAnimNode_Base<TSnapshot>
{
public:
	struct FSample
	{
		FVector2 Position;   // 예: (Direction, Speed) - 블렌드 스페이스의 X/Y 좌표
		UAnimNode_Base<TSnapshot>* ChildNode = nullptr;   // 소유권 없음 - 그래프(RegisterNode)가 소유
	};

	TArray<FSample> Samples;
	std::function<FVector2(const TSnapshot&)> GetBlendInput;

	// [그래프 조립 완료 후, 반드시 한 번 호출] Samples를 다 채운 뒤에 불러야
	// 삼각분할이 올바름 - 샘플이 3개 미만이거나 전부 일직선이면 Triangles가 비고,
	// 그 경우 매 프레임 "삼각형 밖" 폴백(가까운 점 3개, 거리 가중치)만 동작함.
	void BuildTriangulation()
	{
		TArray<FVector2> Points;
		Points.reserve(Samples.size());
		MaxSampleRadius = 0.0f;
		for (const FSample& S : Samples)
		{
			Points.push_back(S.Position);
			float Dist = std::sqrt(S.Position.x * S.Position.x + S.Position.y * S.Position.y);
			MaxSampleRadius = std::max(MaxSampleRadius, Dist);
		}
		Triangles = ComputeDelaunayTriangulation2D(Points);
	}

	void Update(float DeltaSeconds, const TSnapshot& Snapshot) override
	{
		ResolveActiveSamples(Snapshot);

		for (const FActiveSample& AS : ActiveSamples)
		{
			UAnimNode_Base<TSnapshot>* Child = Samples[AS.SampleIndex].ChildNode;
			if (Child != nullptr)
			{
				Child->Update(DeltaSeconds, Snapshot);
			}
		}
	}

	void Evaluate(TArray<FBoneTransform>& OutPose, const TSnapshot& Snapshot) override
	{
		OutPose.clear();

		float AccumWeight = 0.0f;
		bool bFirst = true;

		for (const FActiveSample& AS : ActiveSamples)
		{
			if (AS.Weight <= 0.0f || Samples[AS.SampleIndex].ChildNode == nullptr)
			{
				continue;
			}

			TArray<FBoneTransform> SamplePose;
			Samples[AS.SampleIndex].ChildNode->Evaluate(SamplePose, Snapshot);

			if (bFirst)
			{
				OutPose = SamplePose;
				AccumWeight = AS.Weight;
				bFirst = false;
			}
			else
			{
				// [순차/정규화 블렌딩] 결과적으로 전체 샘플의 정확한 가중 평균과
				// 같아짐(이동은 정확히 같고, 회전은 구면 보간이 결합법칙을 안
				// 만족해서 아주 미세하게 순서 의존적일 수 있으나 실무에서 널리
				// 쓰이는 표준적인 근사).
				AccumWeight += AS.Weight;
				float Alpha = AS.Weight / AccumWeight;
				size_t Count = std::min(OutPose.size(), SamplePose.size());
				for (size_t i = 0; i < Count; i++)
				{
					OutPose[i] = FBoneTransform::Interpolate(OutPose[i], SamplePose[i], Alpha);
				}

				SDL_Log("FAnimNode_BlendSpace2D - [진단] Alpha : %f", Alpha);
			}
		}
	}

private:
	struct FActiveSample
	{
		int32 SampleIndex = 0;
		float Weight = 0.0f;
	};

	TArray<FTriangleIndices> Triangles;
	float MaxSampleRadius = 0.0f;   // BuildTriangulation()이 계산 - 원점에서 가장 먼 샘플까지의 거리
	TArray<FActiveSample> ActiveSamples;   // Update에서 채우고 Evaluate가 그대로 씀(같은 프레임 안)

	void ResolveActiveSamples(const TSnapshot& Snapshot)
	{
		ActiveSamples.clear();

		if (!GetBlendInput || Samples.empty())
		{
			return;
		}

		FVector2 Input = GetBlendInput(Snapshot);

		// [범위 밖 입력 클램프 - 방향은 유지, 길이만 자름] 실제 이동 속도가
		// 블렌드 스페이스 샘플들이 덮는 최대 반경(예: 200)보다 클 수 있음(예:
		// 실제 이동 속도 400). 클램프 없이 그냥 먼 지점을 질의하면, 그 먼
		// 거리에서는 샘플 간 "상대적" 거리 차이가 흐려져서(전부 비슷하게
		// 멀어 보여서) 폴백(가까운 점 3개, 거리 가중치)의 방향 구분이 무너짐 -
		// 확인된 증상. 축별로 자르면 방향이 왜곡되니, 벡터 길이만 최대 반경으로
		// 잘라서 방향은 그대로 보존함.
		float InputLength = std::sqrt(Input.x * Input.x + Input.y * Input.y);
		if (MaxSampleRadius > 0.0f && InputLength > MaxSampleRadius)
		{
			float Scale = MaxSampleRadius / InputLength;
			Input.x *= Scale;
			Input.y *= Scale;
		}

		// [1차 - 삼각형 안] 삼각분할된 삼각형들을 훑어서 Input을 담는 삼각형을 찾음.
		for (const FTriangleIndices& Tri : Triangles)
		{
			bool bValid = false;
			FBarycentricWeights W = ComputeBarycentricWeights(
				Input, Samples[Tri.A].Position, Samples[Tri.B].Position, Samples[Tri.C].Position, bValid);

			if (bValid && IsPointInTriangle(W))
			{
				ActiveSamples.push_back({ Tri.A, W.WeightA });
				ActiveSamples.push_back({ Tri.B, W.WeightB });
				ActiveSamples.push_back({ Tri.C, W.WeightC });
				return;
			}
		}

		// [2차 - 폴백: 어떤 삼각형에도 안 들어감] 예: 최고 속도를 넘어선 경우.
		// 가장 가까운 점 3개를 거리 역수 비례로 가중치를 매겨 섞음.
		TArray<std::pair<float, int32>> Distances;   // (거리제곱, 샘플 인덱스)
		Distances.reserve(Samples.size());
		for (int32 i = 0; i < static_cast<int32>(Samples.size()); i++)
		{
			FVector2 Diff = Samples[i].Position - Input;
			Distances.emplace_back(Diff.x * Diff.x + Diff.y * Diff.y, i);
		}
		std::sort(Distances.begin(), Distances.end());

		int32 NearCount = std::min(static_cast<int32>(Distances.size()), 3);
		TArray<float> InverseDistances;
		float TotalInverseDistance = 0.0f;
		for (int32 i = 0; i < NearCount; i++)
		{
			float Dist = std::sqrt(Distances[i].first);
			float InvDist = 1.0f / std::max(Dist, 0.0001f);   // 거리 0에 가까우면(정확히 그 점) 사실상 그 점 100%
			InverseDistances.push_back(InvDist);
			TotalInverseDistance += InvDist;
		}
		for (int32 i = 0; i < NearCount; i++)
		{
			ActiveSamples.push_back({ Distances[i].second, InverseDistances[i] / TotalInverseDistance });
		}
	}
};

// [묘수 - 배치 결과를 바로 Samples로] ImportAndRegisterManifest가 돌려준 28개
// 항목(핸들까지 이미 다 등록된 상태)을 받아서, 각 항목마다
// FAnimNode_SequencePlayer를 하나씩 새로 만들어 (Position, ChildNode) 쌍으로
// 묶어줌 - 호출부는 이 결과를 그대로 BlendSpaceNode->Samples에 대입하고
// BuildTriangulation()만 부르면 됨. 노드 생성은 가볍고(IO 없음) TSnapshot마다
// 다른 타입이 필요해서 템플릿으로 여기 둠(무거운 파일 IO/FBX 파싱은
// ImportAndRegisterManifest 쪽에 이미 다 끝나 있음 - 비템플릿이라 한 번만 컴파일됨).
//
// [주의] 임포트 실패한 항목(bImportSucceeded=false)은 조용히 걸러짐 - 몇 개가
// 빠졌는지는 ImportAndRegisterManifest가 이미 SDL_Log로 남긴 걸로 확인.
template<typename TSnapshot>
TArray<typename FAnimNode_BlendSpace2D<TSnapshot>::FSample> BuildBlendSpaceSamplesFromManifest(
	const TArray<FBlendSpaceManifestEntry>& Entries, FAssetHandle SkeletonHandle)
{
	TArray<typename FAnimNode_BlendSpace2D<TSnapshot>::FSample> Result;
	Result.reserve(Entries.size());

	for (const FBlendSpaceManifestEntry& Entry : Entries)
	{
		if (!Entry.bImportSucceeded)
		{
			continue;
		}

		auto* Player = new FAnimNode_SequencePlayer<TSnapshot>();
		Player->SequenceHandle = Entry.SequenceHandle;
		Player->SkeletonHandle = SkeletonHandle;
		Player->bLoop = true;

		typename FAnimNode_BlendSpace2D<TSnapshot>::FSample Sample;
		Sample.Position = Entry.Position;
		Sample.ChildNode = Player;
		Result.push_back(Sample);
	}

	return Result;
}

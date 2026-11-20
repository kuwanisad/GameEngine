#include "../../Public/Utilities/DelaunayTriangulation2D.h"
#include <algorithm>
#include <SDL_log.h>

namespace
{
	// 알고리즘 진행 중에만 쓰는 내부용 삼각형 표현 - 최종 결과(FTriangleIndices)와
	// 달리, "슈퍼 삼각형"의 가짜 꼭짓점 인덱스도 섞여 들어갈 수 있음(마지막에 걸러냄).
	struct FInternalTriangle
	{
		int32 A = 0, B = 0, C = 0;
	};

	// 삼각형의 변 하나 - 두 점을 잇는 선분. operator==를 "방향 무관하게" 정의해둔
	// 이유: 삼각형 두 개가 같은 변을 "서로 반대 방향"으로 갖고 있어도(한쪽은
	// P0→P4, 다른 쪽은 P4→P0) 같은 변으로 인식해야 "겹치는 변" 판정이 맞기 때문.
	struct FEdge
	{
		int32 A = 0, B = 0;
		bool operator==(const FEdge& Other) const
		{
			return (A == Other.A && B == Other.B) || (A == Other.B && B == Other.A);
		}
	};

	// =============================================================================
	// [부호 있는 넓이 = 2차원 외적] SignedArea2
	// =============================================================================
	//
	// 벡터 AB와 AC의 2차원 외적(cross product)을 계산함:
	//   (B.x-A.x)*(C.y-A.y) - (C.x-A.x)*(B.y-A.y)
	//
	// 이게 왜 "넓이"인가:
	//   AB와 AC가 만드는 평행사변형의 넓이가 정확히 이 값이고(선형대수의 표준
	//   성질), 삼각형 ABC는 그 평행사변형을 대각선으로 자른 절반이므로
	//   "삼각형 넓이 = |SignedArea2| / 2". 함수 이름의 "2"가 이 "넓이의 2배"라는
	//   뜻이다(호출부에서 필요시 0.5를 곱해서 진짜 넓이로 바꿔 씀).
	//
	// 이게 왜 "부호"까지 갖는가 (그리고 왜 이 부호가 중요한가):
	//   외적의 부호는 "A→B로 간 다음, C가 그 진행 방향의 왼쪽(반시계)에
	//   있는지 오른쪽(시계)에 있는지"를 알려준다 - 양수면 반시계, 음수면
	//   시계. 아래 IsPointInCircumcircle의 행렬식 공식이 "A,B,C가 반시계
	//   방향으로 주어졌다"는 전제 하에서만 부호가 정확하기 때문에, 이 함수로
	//   방향을 확인하고 필요하면 뒤집는 게 알고리즘 정확성의 핵심이다.
	float SignedArea2(const FVector2& A, const FVector2& B, const FVector2& C)
	{
		return (B.x - A.x) * (C.y - A.y) - (C.x - A.x) * (B.y - A.y);
	}

	// [방향 정규화] 새 삼각형을 만들 때마다 이걸 거쳐서 항상 반시계 방향으로
	// 통일함. SignedArea2가 음수(시계 방향)로 나오면 B와 C를 맞바꿔서 뒤집는다
	// (두 점을 바꾸면 외적 부호도 자동으로 반전됨) - 이 정규화가 없으면 삼각형마다
	// IsPointInCircumcircle의 판정이 뒤집힐 수 있다.
	FInternalTriangle MakeTriangleCCW(int32 A, int32 B, int32 C, const TArray<FVector2>& Pts)
	{
		if (SignedArea2(Pts[A], Pts[B], Pts[C]) < 0.0f)
		{
			return { A, C, B };
		}
		return { A, B, C };
	}

	// =============================================================================
	// [외접원 판정] IsPointInCircumcircle
	// =============================================================================
	//
	// [왜 애초에 2차원 문제를 3차원으로 바꾸는가]
	//   목표는 "D가 A,B,C를 지나는 원 안에 있는가"인데, 이걸 2차원에서 직접
	//   풀려면 원의 중심과 반지름을 먼저 구하고(나눗셈이 섞인 복잡한 식),
	//   그 다음 D까지의 거리를 재서 반지름과 비교해야 한다 - 중간 계산에서
	//   오차가 누적되기 쉽고 코드도 길어진다.
	//   반면 3차원으로 들어올리면(아래 설명), "원 안/밖"이라는 곡선 판정이
	//   "평면의 위/아래"라는 훨씬 다루기 쉬운 직선(1차식) 판정으로 바뀌고,
	//   이건 나눗셈 없이 곱셈과 덧셈만으로(행렬식 하나로) 한 번에 계산된다 -
	//   원의 중심/반지름을 구하는 중간 단계 자체가 아예 필요 없어진다.
	//
	// [왜 포물면인가] 그 "곡선 -> 직선"으로 바꿔주는 도구가 포물면이다:
	//   (x,y) -> (x, y, x²+y²) 로 들어올리면, 평면 위의 "원"이 3차원에서는
	//   포물면과 어떤 "평면"이 만나는 단면과 정확히 대응된다(아래에서 증명).
	//
	// 원의 일반식:            x² + y² + Dx + Ey + F = 0
	// 위 식을 z로 정리하면:    z = -(Dx + Ey + F)              ... (평면 방정식)
	//   [왜 이게 평면인가] x,y에 대해 1차식(제곱 없음)이므로 정의상 평면이고,
	//   D,E,F가 애초에 A,B,C를 지나도록 정해진 값이라 정확히 A',B',C'를
	//   지나는 그 평면이 된다.
	//
	// f(x,y) = x² + y² + Dx + Ey + F
	//        = (포물면 높이) - (평면 높이)
	//   [왜 이 뺄셈이 의미 있는가] 원의 판정식(f)과 "포물면이 평면보다
	//   높은지 낮은지"가 대수적으로 완전히 같은 식이 되기 때문 - 즉
	//   "원 안/밖"과 "포물면 위/아래"가 동치인 문제로 바뀐다.
	//
	//        f < 0  <=>  원 안쪽   <=>  들어올린 점이 평면보다 아래(포물면이 오목하므로)
	//        f = 0  <=>  원 위     <=>  들어올린 점이 평면 위
	//        f > 0  <=>  원 바깥   <=>  들어올린 점이 평면보다 위
	//
	// [왜 "평면의 위/아래"를 부피로 재는가] SignedArea2가 "두 벡터의 외적으로
	// 직선의 왼쪽/오른쪽을 잰다"는 것과 완전히 같은 논리를 한 차원 위(평면의
	// 위/아래)에서 반복하는 것 - 3차원에서는 외적+내적(=스칼라 삼중곱)이
	// 그 역할을 하고, 이게 곧 사면체의 부호 있는 부피다.
	//
	//     Vol ∝ | Ax  Ay  Ax²+Ay²  1 |
	//           | Bx  By  Bx²+By²  1 |
	//           | Cx  Cy  Cx²+Cy²  1 |
	//           | Dx  Dy  Dx²+Dy²  1 |
	//
	// [왜 4x4를 3x3으로 줄이는가] 계산량을 줄이기 위해 - 행렬식은 "한 행/열에서
	// 다른 행/열의 배수를 빼도 값이 안 바뀐다"는 성질이 있어서, 1~3행에서
	// 4행(D)을 빼고 3번째 열도 마저 정리(2Dx·ax 등 상쇄)하면 마지막 열이
	// (0,0,0,1) 모양이 되고, 여인수 전개 시 D의 행과 그 열이 통째로 사라진다:
	//
	//     ax = Ax - Dx,  ay = Ay - Dy   (B, C도 동일)
	//
	//     det = | ax  ay  ax²+ay² |
	//           | bx  by  bx²+by² |
	//           | cx  cy  cx²+cy² |
	//
	//         = (ax²+ay²)(bx·cy - cx·by)
	//         - (bx²+by²)(ax·cy - cx·ay)
	//         + (cx²+cy²)(ax·by - bx·ay)
	//
	// 부호 해석 (A,B,C가 반시계 방향일 때):
	//     det > 0  ->  D는 원 안
	//     det < 0  ->  D는 원 밖
	//     det = 0  ->  D는 원 위 (네 점 공원)
	//
	// [왜 반시계 방향이 전제인가] "위/아래"를 재는 부호 자체가 A,B,C를 도는
	// 순서(=SignedArea2의 부호)에 따라 뒤집히기 때문 - 순서가 안 맞으면
	// "원 안"과 "원 밖" 판정이 정반대로 나온다. 그래서 호출 전 MakeTriangleCCW로
	// 항상 반시계 방향 정규화가 돼있어야 한다.
	bool IsPointInCircumcircle(const FVector2& A, const FVector2& B, const FVector2& C, const FVector2& D)
	{
		double ax = static_cast<double>(A.x) - D.x, ay = static_cast<double>(A.y) - D.y;
		double bx = static_cast<double>(B.x) - D.x, by = static_cast<double>(B.y) - D.y;
		double cx = static_cast<double>(C.x) - D.x, cy = static_cast<double>(C.y) - D.y;

		double det =
			(ax * ax + ay * ay) * (bx * cy - cx * by) -
			(bx * bx + by * by) * (ax * cy - cx * ay) +
			(cx * cx + cy * cy) * (ax * by - bx * ay);

		return det > 0.0;
	}
}

// =============================================================================
// [본 함수] ComputeDelaunayTriangulation2D - Bowyer-Watson 알고리즘
// =============================================================================
//
// 한 문장 요약: "아주 큰 가짜 삼각형 하나로 시작해서, 점을 하나씩 넣을 때마다
// 그 점 근처의 삼각형들만 국소적으로 허물고 다시 짓는다."
//
// 전체 흐름:
//   [준비] 모든 점을 감싸는 슈퍼 삼각형 하나로 시작
//   [반복, 점마다] 그 점의 외접원 안에 들어오는 기존 삼각형("나쁜 삼각형")들을
//                 찾음 -> 지움 -> 생긴 구멍의 테두리를 그 점과 이어서 다시 채움
//   [마무리] 슈퍼 삼각형 흔적(가짜 꼭짓점이 섞인 삼각형)을 전부 제거
TArray<FTriangleIndices> ComputeDelaunayTriangulation2D(const TArray<FVector2>& Points)
{
	TArray<FTriangleIndices> Result;

	if (Points.size() < 3)
	{
		SDL_Log("ComputeDelaunayTriangulation2D - need at least 3 points, got %d", static_cast<int32>(Points.size()));
		return Result;
	}

	// ── [준비 단계] 슈퍼 삼각형 ──────────────────────────────────────────
	// 이 알고리즘은 "기존 삼각형을 허물고 다시 짓는" 방식이라, 애초에 허물
	// 대상이 하나도 없으면 시작을 못 한다. 그래서 모든 점을 확실히 담을
	// 만큼 아주 큰 가짜 삼각형 하나를 인위적으로 먼저 만들어둔다 - 이건
	// 최종 결과의 일부가 아니라 계산을 시작하기 위한 발판일 뿐이다.
	float MinX = Points[0].x, MaxX = Points[0].x;
	float MinY = Points[0].y, MaxY = Points[0].y;
	for (const FVector2& P : Points)
	{
		MinX = std::min(MinX, P.x); MaxX = std::max(MaxX, P.x);
		MinY = std::min(MinY, P.y); MaxY = std::max(MaxY, P.y);
	}
	float Dx = MaxX - MinX;
	float Dy = MaxY - MinY;
	float DeltaMax = std::max(Dx, Dy) * 10.0f + 1.0f;   // 바운딩 박스보다 충분히 크게 - 모든 점을 확실히 감싸도록
	float MidX = (MinX + MaxX) * 0.5f;
	float MidY = (MinY + MaxY) * 0.5f;

	// 원본 Points는 안 건드리고, 계산용으로 슈퍼 삼각형의 세 꼭짓점을 뒤에
	// 덧붙인 확장 배열(WorkPoints)을 따로 씀 - SuperA/B/C는 그 세 점의 인덱스.
	TArray<FVector2> WorkPoints = Points;
	int32 SuperA = static_cast<int32>(WorkPoints.size());
	WorkPoints.emplace_back(MidX - 2.0f * DeltaMax, MidY - DeltaMax);
	int32 SuperB = static_cast<int32>(WorkPoints.size());
	WorkPoints.emplace_back(MidX, MidY + 2.0f * DeltaMax);
	int32 SuperC = static_cast<int32>(WorkPoints.size());
	WorkPoints.emplace_back(MidX + 2.0f * DeltaMax, MidY - DeltaMax);

	TArray<FInternalTriangle> Triangles;
	Triangles.push_back(MakeTriangleCCW(SuperA, SuperB, SuperC, WorkPoints));

	// ── [반복 단계] 점을 하나씩 추가 ─────────────────────────────────────
	// 원본 점만(슈퍼 삼각형 제외) 순회 - 매번 "지금까지 만든 삼각형들이
	// 이 새 점을 넣은 뒤에도 여전히 들로네 조건을 만족하는지" 국소적으로
	// 다시 정리한다.
	for (int32 PointIndex = 0; PointIndex < static_cast<int32>(Points.size()); PointIndex++)
	{
		const FVector2& NewPoint = WorkPoints[PointIndex];

		// a) [나쁜 삼각형 찾기] 새 점이 외접원 안에 들어오는 삼각형은, 더 이상
		// 들로네 조건("외접원 안에 다른 점이 없어야 함")을 못 지키므로 "나쁨".
		TArray<FInternalTriangle> BadTriangles;
		for (const FInternalTriangle& Tri : Triangles)
		{
			if (IsPointInCircumcircle(WorkPoints[Tri.A], WorkPoints[Tri.B], WorkPoints[Tri.C], NewPoint))
			{
				BadTriangles.push_back(Tri);
			}
		}

		// b) [나쁜 삼각형들이 만드는 "구멍"의 테두리만 골라내기]
		// 나쁜 삼각형들을 다 지우면 다각형 모양의 구멍이 뚫린다. 그 구멍을
		// 다시 메우려면 "바깥쪽 테두리"만 알면 된다 - 나쁜 삼각형끼리 서로
		// 붙어있는 변(구멍 내부의 벽)은 버리고, 딱 하나에만 등장하는 변
		// (바깥 경계)만 진짜 테두리다. 그래서 나쁜 삼각형의 변 하나하나마다,
		// "다른 나쁜 삼각형도 이 변을 갖고 있는지"를 확인해서 안 겹치는
		// 것만 남긴다(이중 루프로 모든 나쁜 삼각형 쌍을 비교).
		TArray<FEdge> Polygon;
		for (size_t ti = 0; ti < BadTriangles.size(); ti++)
		{
			const FInternalTriangle& Tri = BadTriangles[ti];
			FEdge Edges[3] = { {Tri.A, Tri.B}, {Tri.B, Tri.C}, {Tri.C, Tri.A} };

			for (const FEdge& E : Edges)
			{
				bool bShared = false;
				for (size_t oi = 0; oi < BadTriangles.size(); oi++)
				{
					if (oi == ti)   // 자기 자신과는 비교 안 함 - 당연히 같은 변이 나오므로 의미 없음
					{
						continue;
					}
					const FInternalTriangle& Other = BadTriangles[oi];
					FEdge OtherEdges[3] = { {Other.A, Other.B}, {Other.B, Other.C}, {Other.C, Other.A} };
					for (const FEdge& OE : OtherEdges)
					{
						if (E == OE)
						{
							bShared = true;
							break;
						}
					}
					if (bShared)
					{
						break;
					}
				}
				if (!bShared)   // 아무도 이 변을 공유 안 함 -> 진짜 바깥 테두리
				{
					Polygon.push_back(E);
				}
			}
		}

		// c) [나쁜 삼각형 실제 제거]
		TArray<FInternalTriangle> KeptTriangles;
		for (const FInternalTriangle& Tri : Triangles)
		{
			bool bIsBad = false;
			for (const FInternalTriangle& Bad : BadTriangles)
			{
				if (Tri.A == Bad.A && Tri.B == Bad.B && Tri.C == Bad.C)
				{
					bIsBad = true;
					break;
				}
			}
			if (!bIsBad)
			{
				KeptTriangles.push_back(Tri);
			}
		}
		Triangles = KeptTriangles;

		// d) [구멍의 테두리 변 + 새 점으로 다시 채움]
		// 테두리 변 하나하나가, 새 점을 향해 삼각형 하나씩을 만든다. 이렇게
		// 새로 만든 삼각형들은 "방금 그 점을 기준으로" 만든 것이므로, 정의상
		// 그 점에 대한 들로네 조건을 자동으로 만족한다(이 점이 원 안에
		// 들어오지 않도록 새로 지었으니까).
		for (const FEdge& E : Polygon)
		{
			Triangles.push_back(MakeTriangleCCW(E.A, E.B, PointIndex, WorkPoints));
		}
	}

	// ── [마무리 단계] 슈퍼 삼각형 흔적 제거 ──────────────────────────────
	// 알고리즘이 끝나도, 가장 바깥쪽(볼록 껍질)에 있던 점들은 자기보다 더
	// 바깥에 "진짜" 점이 없어서, 계속 슈퍼 삼각형의 가짜 꼭짓점 중 하나를
	// 붙들고 있는 채로 남아있다. 슈퍼 삼각형은 처음부터 계산을 시작하기
	// 위한 발판이었을 뿐 최종 결과의 일부가 아니므로, 세 가짜 꼭짓점
	// (SuperA/B/C) 중 하나라도 걸린 삼각형은 전부 걸러낸다 - 아홉 번의
	// 비교(삼각형의 A/B/C 자리 3곳 × 슈퍼 꼭짓점 3개)로 확인.
	for (const FInternalTriangle& Tri : Triangles)
	{
		if (Tri.A == SuperA || Tri.A == SuperB || Tri.A == SuperC ||
			Tri.B == SuperA || Tri.B == SuperB || Tri.B == SuperC ||
			Tri.C == SuperA || Tri.C == SuperB || Tri.C == SuperC)
		{
			continue;
		}
		Result.push_back({ Tri.A, Tri.B, Tri.C });
	}

	if (Result.empty())
	{
		SDL_Log("ComputeDelaunayTriangulation2D - triangulation produced 0 triangles (points may be degenerate/collinear)");
	}

	return Result;
}

// =============================================================================
// [무게중심 좌표 계산] ComputeBarycentricWeights
// =============================================================================
//
// 표준 무게중심 좌표 공식. 분모(Denom)가 0에 가까우면 삼각형 ABC가 퇴화
// (세 점이 일직선)라는 뜻이라 계산이 성립하지 않음 - bOutValid=false로 알림.
FBarycentricWeights ComputeBarycentricWeights(const FVector2& P, const FVector2& A, const FVector2& B, const FVector2& C, bool& bOutValid)
{
	FBarycentricWeights Result;

	float Denom = (B.y - C.y) * (A.x - C.x) + (C.x - B.x) * (A.y - C.y);
	if (Math::NearZero(Denom))
	{
		bOutValid = false;
		return Result;
	}

	Result.WeightA = ((B.y - C.y) * (P.x - C.x) + (C.x - B.x) * (P.y - C.y)) / Denom;
	Result.WeightB = ((C.y - A.y) * (P.x - C.x) + (A.x - C.x) * (P.y - C.y)) / Denom;
	Result.WeightC = 1.0f - Result.WeightA - Result.WeightB;   // 세 가중치의 합은 정의상 항상 1

	bOutValid = true;
	return Result;
}

// 세 가중치가 전부 [0,1] 범위(오차 허용 Epsilon 포함) 안이면 삼각형 안(경계 포함).
// 가중치 중 하나라도 음수라는 건, P가 그 꼭짓점과 마주보는 변의 "바깥쪽"으로
// 벗어났다는 뜻.
bool IsPointInTriangle(const FBarycentricWeights& Weights, float Epsilon)
{
	return Weights.WeightA >= -Epsilon && Weights.WeightA <= 1.0f + Epsilon
		&& Weights.WeightB >= -Epsilon && Weights.WeightB <= 1.0f + Epsilon
		&& Weights.WeightC >= -Epsilon && Weights.WeightC <= 1.0f + Epsilon;
}

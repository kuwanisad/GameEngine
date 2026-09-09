#include "../../Public/Collision/Collision.h"

FLineSegment::FLineSegment(const FVector3& _Start, const FVector3 _End) :
	Start(_Start),
	End(_End)
{
}

FVector3 FLineSegment::PointOnSegment(float t) const
{
    return Start + (End - Start) * t;
}

float FLineSegment::MinDistSq(const FVector3& _Point)
{
    FVector3 direction = End - Start;
    FVector3 direction_negative = direction * -1.f;
    FVector3 Start_Point = _Point - Start;
    FVector3 End_Point = _Point - End;
    
    if (Math::NearZero(direction.LengthSq()))
    {
        return Start_Point.LengthSq();
    }

    if (FVector3::Dot(direction, Start_Point) < 0)
    {
        return Start_Point.LengthSq();
    }
    else if (FVector3::Dot(direction_negative, End_Point) < 0)
    {
        return End_Point.LengthSq();
    }
    else
    {
        /*
        Q = A + (B - A) * t   <-- d = B - A
        Q = A + d * t
        
        min: d * (_Point - Q) = 0
        d * (_Point - (A + dt)) = 0
        d * (_Point - A) - t(d*d) = 0
        d * (_Point - A) = t(d*d)

        t = (d * (_Point - A)) / (d*d)
        */
        float t = FVector3::Dot(Start_Point, direction) / direction.LengthSq();
        FVector3 p = t * direction;
        
        return (Start_Point - p).LengthSq();
    }
}

float FLineSegment::MinDistSq(const FLineSegment& S1, const FLineSegment& S2)
{
    FVector3 p1, p2;
    return MinDistSq(S1, S2, p1, p2);
}

float FLineSegment::MinDistSq(const FLineSegment& S1, const FLineSegment& S2, FVector3& outPointOnS1, FVector3& outPointOnS2)
{
	/*
	두 선분을 다음과 같은 매개변수 방정식으로 표현

	S1(s) = S1.Start + s * (S1.End - S1.Start)
	S2(t) = S2.Start + t * (S2.End - S2.Start)

	잠시 선분이라는 조건을 잠시 무시하고 두 직선 사이의 최단거리를 구한 다음
	s와 t가 [0,1] 범위 안인지 검사
	*/

	FVector3 s1_s_to_e = S1.End - S1.Start;
	FVector3 s2_s_to_e = S2.End - S2.Start;

	/*
	u = S1.End - S1.Start
	v = S2.End - S2.Start

	S1(s) = S1.Start + s*u
	S2(t) = S2.Start + t*v
	*/

	FVector3 start_s1_to_s2 = S1.Start - S2.Start;

	/*
	w = S1.Start - S2.Start

	S1(s) - S2(t) = (S1.Start + s*u) - (S2.Start + t*v)
	= w + s*u - t*v

	이 벡터 길이가 최소가 되는 s, t를 찾아야 함
	*/

	float a = FVector3::Dot(s1_s_to_e, s1_s_to_e);
	float b = FVector3::Dot(s1_s_to_e, s2_s_to_e);
	float c = FVector3::Dot(s2_s_to_e, s2_s_to_e);
	float d = FVector3::Dot(s1_s_to_e, start_s1_to_s2);
	float e = FVector3::Dot(s2_s_to_e, start_s1_to_s2);

	float Det = a * c - b * b;

	/*
	두 직선 사이의 최단거리를 구하는 연립방정식의 크라메르 법칙을 적용하기 위해 미리 계산

	두 직선 위의 점 차이를

		R(s,t) = w + s*u - t*v

		R * u = 0 --> 첫 번째
		R * v = 0 --> 두 번째

	첫 번째 식

		(w + s*u - t*v) * u = 0
		w*u + s(u*u) - t(v*u) = 0
		d + s*a - t*b = 0
		a*s - b*t = -d

	두 번째 식

		(w + s*u - t*v) * v = 0
		w*v + s(u*v) - t(v*v) = 0
		e + s*b - t*c = 0
		b*s - c*t = -e

	합치면

		[a  -b] [s] = [-d]
		[b  -c] [t]   [-e]

	행렬식
		Det = ac - b^2
	*/


	float s1r = 0, s1n = 0, s1d = Det;
	float s2r = 0, s2n = 0, s2d = Det;

	/*
	s1r = 최종적으로 사용할 S1의 매개변수 s
	s2r = 최종적으로 사용할 S2의 매개변수 t

	s1n / s1d = s
	s2n / s2d = t

	형태로 만들어 둔다.

	처음에는 두 직선이 평행하지 않다고 가정하고 분모를 Det로 사용
	아직 실제로 나누지 않는 이유는
	중간에 s나 t가 선분 범위를 벗어났을 때
	분자 / 분모를 다른 값으로 바꿔서 처리하기 위해서임
	*/

	if (Math::NearZero(Det)) // 거의 평행
	{
		/*
		ac - b^2가 0에 가까우면 두 방향벡터가 사실상 평행
		이 경우에는 크라메르 법칙을 그대로 적용하면 분모가 0에 가까워짐
		따라서 두 직선의 교차하는 수직선을 일반적인 방식으로 계산하지 않고
		S1의 시작점을 기준으로 S2에서 가장 가까운 점을 찾음
		여기서는 S1의 s를 0으로 고정
		*/

		s1n = 0;
		s1d = 1;

		/*
		s = s1n / s1d --> 0 / 1 
		= 0

		S1의 최근접점을 S1.Start로 고정
		*/

		s2n = e;
		s2d = c;

		/*
			S1.Start에서 S2 직선으로 수선을 내리고 그것과 내적해서 값이 0이 되는 지점을 찾음

			S2(t) = S2.Start + t*v

			(S1.Start - S2(t)) * v = 0

			(w - t*v) * v = 0
			e - t*c = 0
			t = e/c
		*/
	}
	else
	{
		/*
		평행하지 않으므로 원래 계획대로 진행
		앞에서 만든 행렬식값을 넘겨서

		s = (b*e - c*d) / (a*c - b^2)
		t = (a*e - b*d) / (a*c - b^2)

		분자만 저장
		*/

		s1n = (b * e - c * d);
		s2n = (a * e - b * d);

		/*
		무한한 직선일 때를 기준으로 했으므로 이제 클리핑을 해야 함
		*/

		if (s1n < 0.f)
		{
			/*
			S1의 최근접점이 S1.Start 밖
			*/

			s1n = 0.f;

			/*
			이제 S1의 점은 고정되었으므로 S1.Start와 S2 사이의 최단거리를 다시 구해야 함

			S1의 s=0을 대입하면

			S1(0) = S1.Start

			S2(t) = S2.Start + tv
			v = S2.End - S2.Start

			S1은 고정됐으니

			S1.Start - S2(t) = S1.Start - (S2.Start + tv)

			위에서 w가 S1.Start - S2.Start라고 했으니

			= w - tv

			S2에 대한 최단지점은 v와 수직이어야 함

			(w - tv)*v = 0 --> v*w - tv*v

			v*w가 e, v*v가 c

			e - tc = 0
			t = e / c
			*/

			s2n = e;
			s2d = c;
		}
		else if (s1n > s1d)
		{
			/*
			직선에서의 최근접점이 S1.End 밖
			s를 1로 고정해야 함

			s1d가 Det이므로 1로 만들기 위해

			s1n = s1d
			s = s1n / s1d = s1d / s1d
			= 1
			*/

			s1n = s1d;

			/*
			s1n < 0.f와 같은 방식으로 전개해서
			*/

			s2n = e + b;
			s2d = c;
		}
	}


	/*
		여기까지의 과정에서 중요한 점은

			"두 직선의 최단점 공식"

		만으로 끝내면 안 된다는 것이다.

		그 공식은 무한히 뻗은 직선을 대상으로 한다.

		예를 들어 계산 결과가

			s = -0.5

		라고 나왔다면

			S1.Start에서 반대 방향으로 0.5만큼 간 지점

		이 최근접점이라는 뜻이다.

		하지만 선분에는 그런 점이 존재하지 않는다.

		따라서 그 경우 S1.Start에 고정하고
		나머지 선분과의 최단점을 다시 계산해야 한다.
	*/

	if (s2n < 0)
	{
		/*
		S2.Start를 고정하고 위처럼 수선의 발을 내리는 식으로

		P(s) = S1.Start + su <-- u = S1.End - S1.Start

		S2.Start는 고정되어 있으므로 두 점 사이의 벡터는
		
		P(s) - S2.Start = S1.Start + su - S2.Start

		w를 쓰면
		
		w + su

		이 직선과 수직인 위치를 찾음
		(w + su) * u = 0
		w*u + su*u = 0
		d + sa = 0
		s = -d / a
		*/

		s2n = 0;

		if (-d < 0)
		{
			/*
			S1에서도 최근접점이 Start 밖 Start에 고정

			s = 0
			*/

			s1n = 0;
		}
		else if (-d > a)
		{
			/*
			s > 1
			S1의 직선에서 구한 최근접점이 S1.End 밖이므로 고정
			else if (s1n > s1d)처럼 처리
			*/

			s1n = s1d;
		}
		else
		{
			/*
			범위 안에 존재하므로 그냥 할당
			*/

			s1n = -d;
			s1d = a;
		}
	}
	else if (s2n > s2d)
	{
		/*
		t > 1
		*/

		s2n = s2d;

		/*
		S2의 End를 고정했으므로 S1에서 S2.End와 가장 가까운 점을 찾dma

		S1(s) - S2.End

		수직 조건을 적용하면

		s = (b - d) / a

		따라서 분자/분모를

		s1n = b - d
		s1d = a
		*/

		if ((b - d) < 0)
		{
			/*
				(b-d)/a < 0
				S1에서도 최근접점이 Start 밖
			*/

			s1n = 0;
		}
		else if ((b - d) > a)
		{
			/*
				(b-d)/a > 1
				S1의 End 밖
			*/

			s1n = s1d;
		}
		else
		{
			/*
			범위 안이므로 그냥 할당
			*/

			s1n = b - d;
			s1d = a;
		}
	}


	/*
	이제 s와 t의 분자/분모가 모두 결정됐음
	지금까지 분자와 분모를 나눈 이유는 조건 검사 편의성 때문임
	*/


	if (Math::NearZero(s1n))
	{
		/*
		오류 방지를 위해 0에 근접하면 0으로 처리
		*/

		s1r = 0;
	}
	else
	{
		s1r = s1n / s1d;
	}


	if (Math::NearZero(s2n))
	{
		/*
		위와 동일
		*/

		s2r = 0;
	}
	else
	{
		s2r = s2n / s2d;
	}

	outPointOnS1 = S1.PointOnSegment(s1r);
	outPointOnS2 = S2.PointOnSegment(s2r);

	FVector3 result = outPointOnS1 - outPointOnS2;

	return result.LengthSq();
}

FAABB FLineSegment::GetBoundingBox() const
{
	FVector3 min(Math::Min(Start.x, End.x), Math::Min(Start.y, End.y), Math::Min(Start.z, End.z));
	FVector3 max(Math::Max(Start.x, End.x), Math::Max(Start.y, End.y), Math::Max(Start.z, End.z));
	return FAABB(min, max);
}

FVector3 FLineSegment::GetCenter() const
{
	return PointOnSegment(0.5f);
}

FVector3 FLineSegment::Support(const FVector3& Direction) const
{
	// 선분은 "반지름이 0인 캡슐"이라, 두 끝점 중 Direction 방향으로 더 멀리 있는 쪽이 서포트 점
	return (FVector3::Dot(Start, Direction) >= FVector3::Dot(End, Direction)) ? Start : End;
}

FVector3 FLineSegment::GetStart() const
{
	return Start;
}

FVector3 FLineSegment::GetEnd() const
{
	return End;
}

Plane::Plane(const FVector3& _Normal, float _D) :
	Normal(_Normal),
	D(_D)
{
}

Plane::Plane(const FVector3& a, const FVector3& b, const FVector3& c)
{
	FVector3 ab = b - a;
	FVector3 ac = c - a;
	Normal = FVector3::Cross(ab, ac);
	Normal.Normalize();
	D = -FVector3::Dot(a, Normal);
}

float Plane::SignedDist(const FVector3& point) const
{
	/*
	평면 위 점 P, 평면에서 떨진 점 X가 있을 때
	P = X - tn
	
	P는 평면 위에 있으므로
	n*P + D = 0

	n*(X - tn) + D = 0
	n*X - t + D = 0 <--n은 노멀
	t = n*X + D
	*/

	float dist = FVector3::Dot(GetNormal(), point) + D;

	return dist;
}

FVector3 Plane::GetNormal() const
{
	return Normal;
}

FSphere::FSphere(const FVector3& _center, float _radius) :
	Center(_center),
	Radius(_radius)
{
	Type = ECollisionType::Sphere;
}

bool FSphere::Contains(const FVector3& _point) const
{
	float dist = (GetCenter() - _point).LengthSq();


	return dist <= Radius * Radius;
}

FAABB FSphere::GetBoundingBox() const
{
	FVector3 r(GetRadius(), GetRadius(), GetRadius());
	return FAABB(GetCenter() - r, GetCenter() + r);
}

FVector3 FSphere::GetCenter() const
{
	return Center;
}

FVector3 FSphere::Support(const FVector3& Direction) const
{
	FVector3 Tmp_Direction = Direction;
	Tmp_Direction.Normalize();

	return GetCenter() + Tmp_Direction * GetRadius();
}

float FSphere::GetRadius() const
{
	return Radius;
}

FAABB::FAABB() :
	Center(FVector3::Zero),
	Extent(FVector3::Zero)
{
}

FAABB::FAABB(const FVector3& _Min, const FVector3& _Max)
{
	Center = (_Min + _Max) * 0.5f;
	Extent = (_Min - _Max) * 0.5f;
}

void FAABB::UpdateMinMax(const FVector3& _point)
{
	FVector3 min = GetMin();
	FVector3 max = GetMax();

	min.x = Math::Min(_point.x, min.x);
	min.y = Math::Min(_point.y, min.y);
	min.z = Math::Min(_point.z, min.z);

	max.x = Math::Max(_point.x, max.x);
	max.y = Math::Max(_point.y, max.y);
	max.z = Math::Max(_point.z, max.z);

	Center = (min + max) * 0.5f;
	Extent = (max - min) * 0.5f;
}

void FAABB::Rotate(const FQuaternion& q)
{
	FVector3 axisX = FVector3::Transform(FVector3::UnitX, q);
	FVector3 axisY = FVector3::Transform(FVector3::UnitY, q);
	FVector3 axisZ = FVector3::Transform(FVector3::UnitZ, q);

	//Extent must not negative.
	axisX.x = Math::Abs(axisX.x);
	axisX.y = Math::Abs(axisX.y);
	axisX.z = Math::Abs(axisX.z);

	axisY.x = Math::Abs(axisY.x);
	axisY.y = Math::Abs(axisY.y);
	axisY.z = Math::Abs(axisY.z);

	axisZ.x = Math::Abs(axisZ.x);
	axisZ.y = Math::Abs(axisZ.y);
	axisZ.z = Math::Abs(axisZ.z);

	FVector3 newExtent;

	//E' = |R| * E
	newExtent.x = FVector3::Dot(FVector3(axisX.x, axisY.x, axisZ.x), Extent);
	newExtent.y = FVector3::Dot(FVector3(axisX.y, axisY.y, axisZ.y), Extent);
	newExtent.z = FVector3::Dot(FVector3(axisX.z, axisY.z, axisZ.z), Extent);

}

bool FAABB::Contains(const FVector3& _point) const
{
	FVector3 diff = _point - Extent;

	return Math::Abs(diff.x) <= Extent.x &&
		Math::Abs(diff.y) <= Extent.y &&
		Math::Abs(diff.z) <= Extent.z;
}

float FAABB::MinDistSq(const FVector3& point) const
{
	FVector3 diff = point - Extent;

	//The expansion is equal on both sides of the center.
	float x = Math::Max(Math::Abs(diff.x - Extent.x), 0.f);
	float y = Math::Max(Math::Abs(diff.y - Extent.y), 0.f);
	float z = Math::Max(Math::Abs(diff.z - Extent.z), 0.f);

	return x * x + y * y + z * z;
}

FVector3 FAABB::GetMin() const
{
	return Center - Extent;
}

FVector3 FAABB::GetMax() const
{
	return Center + Extent;
}

void FAABB::SetMin(const FVector3& Value)
{
	FVector3 max = GetMax();
	Center = (Value + max) * 0.5f;
	Extent = (max - Value) * 0.5f;
}

void FAABB::SetMax(const FVector3& Value)
{
	FVector3 min = GetMin();
	Center = (min + Value) * 0.5f;
	Extent = (Value - min) * 0.5f;
}

FVector3 FAABB::GetCenter() const
{
	return Center;
}

FVector3 FAABB::GetExtent() const
{
	return Extent;
}

float FAABB::GetSurfaceArea() const
{
	//Extent is half
	FVector3 e = Extent * 2.0f;

	return 2.0f * //Because of hexahedron
		(e.x * e.y + e.y * e.z + e.z * e.x);
}

FCollision::FCollision() :
	Type(ECollisionType::None)
{
}

FCollision::~FCollision()
{
}

FCapsule::FCapsule(const FVector3& _Start, const FVector3& _End, float _Radius) :
	Segment(_Start, _End),
	Radius(_Radius)
{
	Type = ECollisionType::Capsule;
}

FCapsule::~FCapsule()
{
}

FVector3 FCapsule::PointOnSegment(float t)
{
	return Segment.PointOnSegment(t);
}

bool FCapsule::Contains(const FVector3& point)
{
	float dist = Segment.MinDistSq(point);

	return dist * dist <= Radius * Radius;
}

FLineSegment FCapsule::GetSegment() const
{
	return Segment;
}

float FCapsule::GetRadius() const
{
	return Radius;
}

FAABB FCapsule::GetBoundingBox() const
{
	FVector3 r(Radius, Radius, Radius);
	FVector3 tmp_start = Segment.GetStart();
	FVector3 tmp_end = Segment.GetEnd();

	FVector3 min_vec(Math::Min(tmp_start.x, tmp_end.x), Math::Min(tmp_start.y, tmp_end.y), Math::Min(tmp_start.y, tmp_end.y));
	FVector3 max_vec(Math::Max(tmp_start.x, tmp_end.x), Math::Max(tmp_start.y, tmp_end.y), Math::Max(tmp_start.y, tmp_end.y));

	return FAABB(min_vec - r, max_vec + r);
}

FVector3 FCapsule::GetCenter() const
{
	return Segment.GetCenter();
}

FVector3 FCapsule::Support(const FVector3& Direction) const
{
	FVector3 start = Segment.GetStart();
	start.Normalize();
	FVector3 end = Segment.GetEnd();
	end.Normalize();

	FVector3 offset;

	if (FVector3::Dot(start, Direction) <= FVector3::Dot(end, Direction))
	{
		offset = end;
	}
	else
	{
		offset = start;
	}

	return offset + Direction * Radius;
}

FOBB::FOBB(const FVector3& _Center, const FQuaternion& _Rotation, const FVector3& _Extents) :
	Center(_Center),
	Rotation(_Rotation),
	Extents(_Extents)
{
	Type == ECollisionType::OBB;
}

FOBB::~FOBB()
{
}

FAABB FOBB::GetBoundingBox() const
{
	FVector3 world_axisX = FVector3::Transform(FVector3::UnitX, Rotation);
	FVector3 world_axisY = FVector3::Transform(FVector3::UnitY, Rotation);
	FVector3 world_axisZ = FVector3::Transform(FVector3::UnitZ, Rotation);

	world_axisX.x = Math::Abs(world_axisX.x);
	world_axisX.y = Math::Abs(world_axisX.y);
	world_axisX.z = Math::Abs(world_axisX.z);

	world_axisY.x = Math::Abs(world_axisY.x);
	world_axisY.y = Math::Abs(world_axisY.y);
	world_axisY.z = Math::Abs(world_axisY.z);

	world_axisZ.x = Math::Abs(world_axisZ.x);
	world_axisZ.y = Math::Abs(world_axisZ.y);
	world_axisZ.z = Math::Abs(world_axisZ.z);

	//E' = |R| * E
	FVector3 world_extents;
	world_extents.x = world_axisX.x * Extents.x + world_axisY.x * Extents.y + world_axisZ.x * Extents.z;
	world_extents.y = world_axisX.y * Extents.x + world_axisY.y * Extents.y + world_axisZ.y * Extents.z;
	world_extents.z = world_axisX.z * Extents.x + world_axisY.z * Extents.y + world_axisZ.z * Extents.z;

	return FAABB(Center - world_extents, Center + world_extents);
}

FVector3 FOBB::GetCenter() const
{
	return Center;
}

FQuaternion FOBB::GetRotation() const
{
	return Rotation;
}

void FOBB::Rotate(const FQuaternion& InRotation)
{
	Rotation = FQuaternion::Concatenate(Rotation, InRotation);
}

FVector3 FOBB::GetExtent() const
{
	return Extents;
}

FVector3 FOBB::Support(const FVector3& Direction) const
{
	FVector3 axisX = FVector3::Transform(FVector3::UnitX, Rotation);
	FVector3 axisY = FVector3::Transform(FVector3::UnitY, Rotation);
	FVector3 axisZ = FVector3::Transform(FVector3::UnitZ, Rotation);

	FVector3 point = Center;
	point += axisX * (Extents.x * ((FVector3::Dot(Direction, axisX) >= 0) ? 1.f : -1.f));
	point += axisY * (Extents.x * ((FVector3::Dot(Direction, axisY) >= 0) ? 1.f : -1.f));
	point += axisZ * (Extents.x * ((FVector3::Dot(Direction, axisZ) >= 0) ? 1.f : -1.f));

	return point;
}

void FOBB::SetCenter(const FVector3& InCenter)
{
	Center = InCenter;
}

void FOBB::SetExtent(const FVector3& InExtent)
{
	Extents = InExtent;
}

void FOBB::SetRotation(const FQuaternion& InRotation)
{
	Rotation = InRotation;
}

bool CollisionStatics::Intersect(const FAABB& a, const FAABB& b, FHitResult& OutResult)
{
	OutResult.BlockingHit = false;

	FVector3 a_min = a.GetMin();
	FVector3 a_max = a.GetMax();

	FVector3 b_min = b.GetMin();
	FVector3 b_max = b.GetMax();

	bool isSeparated =
		a_max.x < b_min.x ||
		a_max.y < b_min.y ||
		a_max.z < b_min.z ||

		b_max.x < a_min.x ||
		b_max.y < a_min.y ||
		b_max.z < a_min.z;

	if (isSeparated)
	{
		return false;
	}

	FVector3 overlapMin = Math::Max(a_min, b_min);
	FVector3 overlapMax = Math::Min(a_max, b_max);
	FVector3 overlap = overlapMax - overlapMin;

	FVector3 dir = GetNormalDirection(a.GetCenter(), b.GetCenter());

	float depth = 0;

	OutResult.Normal = FVector3();

	if (overlap.x <= overlap.y && overlap.x <= overlap.z)
	{
		OutResult.Normal.x = (dir.x >= 0.f) ? 1.f : -1.f;
		depth = overlap.x;
	}
	else if (overlap.y <= overlap.x && overlap.y <= overlap.z)
	{
		OutResult.Normal.y = (dir.y >= 0.f) ? 1.f : -1.f;
		depth = overlap.y;
	}
	else
	{
		OutResult.Normal.z = (dir.z >= 0.f) ? 1.f : -1.f;
		depth = overlap.z;
	}

	OutResult.ImpactPoint = (overlapMax + overlapMin) * 0.5f;
	OutResult.PenetrationDepth = depth;

	return true;
}

bool CollisionStatics::Intersect(const FOBB& O1, const FOBB& O2, FHitResult& OutResult)
{
	OutResult.BlockingHit = false;

	//SAT(Separating Axis Theorem)

	FVector3 O1_world_axis[3]{
		FVector3::Transform(FVector3::UnitX, O1.GetRotation()),
		FVector3::Transform(FVector3::UnitY, O1.GetRotation()),
		FVector3::Transform(FVector3::UnitZ, O1.GetRotation()),
	};

	FVector3 O2_world_axis[3]{
		FVector3::Transform(FVector3::UnitX, O2.GetRotation()),
		FVector3::Transform(FVector3::UnitY, O2.GetRotation()),
		FVector3::Transform(FVector3::UnitZ, O2.GetRotation()),
	};

	FVector3 O1_Extents = O1.GetExtent();
	FVector3 O2_Extents = O2.GetExtent();

	FVector3 center_diff = GetNormalDirection(O1.GetCenter(), O2.GetCenter());

	float overlap_min = Math::Infinity;
	FVector3 axis_min = FVector3::Zero;

	auto TestAxis = [&](const FVector3& axis) -> bool
		{
			float axis_length = axis.LengthSq();

			if (Math::NearZero(axis_length))
			{
				return true;
			}

			const FVector3 axis_norm = axis / Math::Sqrt(axis_length);

			float O1_radius = O1_Extents[0] * Math::Abs(FVector3::Dot(axis_norm, O1_world_axis[0]))
							+ O1_Extents[1] * Math::Abs(FVector3::Dot(axis_norm, O1_world_axis[1]))
							+ O1_Extents[2] * Math::Abs(FVector3::Dot(axis_norm, O1_world_axis[2]));

			float O2_radius = O2_Extents[0] * Math::Abs(FVector3::Dot(axis_norm, O2_world_axis[0]))
							+ O2_Extents[1] * Math::Abs(FVector3::Dot(axis_norm, O2_world_axis[1]))
							+ O2_Extents[2] * Math::Abs(FVector3::Dot(axis_norm, O2_world_axis[2]));

			float center_dist = FVector3::Dot(axis_norm, center_diff);
			float overlap_dist = (O1_radius + O2_radius) - Math::Abs(center_dist);

			if (overlap_dist < 0)
			{
				return false;
			}

			if (overlap_dist < overlap_min)
			{
				overlap_min = overlap_dist;
				axis_min = (center_dist >= 0) ? axis_norm : axis_norm * -1.f;
			}

			return true;
		};

	for (int32 i = 0; i < 3; ++i)
	{
		if (TestAxis(O1_world_axis[i]) == false)
		{
			return false;
		}

		if (TestAxis(O2_world_axis[i]) == false)
		{
			return false;
		}
	}

	for (int32 i = 0; i < 3; ++i)
	{
		for (int32 j = 0; j < 3; ++j)
		{
			if (TestAxis(FVector3::Cross(O1_world_axis[i], O2_world_axis[j])) == false)
			{
				return false;
			}
		}
	}

	OutResult.BlockingHit = true;
	OutResult.Normal = axis_min;

	float O1_radius_last = O1_Extents[0] * Math::Abs(FVector3::Dot(axis_min, O1_world_axis[0]))
						 + O1_Extents[1] * Math::Abs(FVector3::Dot(axis_min, O1_world_axis[1]))
						 + O1_Extents[2] * Math::Abs(FVector3::Dot(axis_min, O1_world_axis[2]));

	float O2_radius_last = O2_Extents[0] * Math::Abs(FVector3::Dot(axis_min, O2_world_axis[0]))
						 + O2_Extents[1] * Math::Abs(FVector3::Dot(axis_min, O2_world_axis[1]))
						 + O2_Extents[2] * Math::Abs(FVector3::Dot(axis_min, O2_world_axis[2]));

	//second - first
	FVector3 SurfaceOnO1 = O1.GetCenter() - O1_radius_last * axis_min;
	FVector3 SurfaceOnO2 = O2.GetCenter() + O2_radius_last * axis_min;

	//This choice was inevitable...
	//TODO: must use more precise solution.
	OutResult.ImpactPoint = (SurfaceOnO1 + SurfaceOnO2) * 0.5f;

	OutResult.PenetrationDepth = overlap_min;

	return true;
}

bool CollisionStatics::Intersect(const FCapsule& a, const FCapsule& b, FHitResult& OutResult)
{
	OutResult.BlockingHit = false;

	FVector3 PointOnA;
	FVector3 PointOnB;
	float distSq = FLineSegment::MinDistSq(a.GetSegment(), b.GetSegment(), PointOnA, PointOnB);
	float radius = a.GetRadius();

	if (radius * radius < distSq)
	{
		return false;
	}

	OutResult.BlockingHit = true;

	float dist = Math::Sqrt(distSq);
	FVector3 normal_primary = GetNormalDirection(PointOnA, PointOnB);
	FVector3 normal = Math::NearZero(dist) ? FVector3::Zero : normal_primary / dist;

	OutResult.Normal = normal;
	OutResult.ImpactPoint = PointOnB - normal * a.GetRadius();
	OutResult.PenetrationDepth = radius - dist;

	return true;
}

bool CollisionStatics::Intersect(const FCapsule& c, const FOBB& o, FHitResult& OutResult)
{
	OutResult.BlockingHit = false;

	//capsule --> OBB local space;
	FQuaternion invRot = o.GetRotation();
	invRot.Conjugate();

	FVector3 o_center = o.GetCenter();
	FVector3 o_extent = o.GetExtent();

	FLineSegment seg = c.GetSegment();

	/*
	target * R + translation = p'
	target * R = p' - translation
	target = (p' - translation) * R^-1
	*/
	FVector3 seg_local_start = FVector3::Transform(seg.GetStart() - o_center, invRot);
	FVector3 seg_local_end = FVector3::Transform(seg.GetEnd() - o_center, invRot);

	FVector3 seg_direction = seg_local_end - seg_local_start;
	float seg_direction_lenSq = seg_direction.LengthSq();

	// =========================================================
	// this commend was aidded by Claude
	// 
	// Find the actual closest-point pair (p, q) between the capsule centerline and the box
	//
	// Since both the line segment and the box are convex shapes, repeatedly
	// projecting onto the other shape (alternating projection) is guaranteed
	// to converge to the true closest-point pair (the pair that minimizes the
	// global distance). The distance never increases during each iteration.
	//   - p: current candidate point on the line segment
	//   - q: point obtained by clamping p to the box surface
	// =========================================================
	FVector3 p = seg_local_start; //Start point
	FVector3 q = FVector3::Zero;

	const int32 IterationCount = 8;

	for (int32 i = 0; i < IterationCount; ++i)
	{
		q.x = Math::Clamp(p.x, -o_extent.x, o_extent.x);
		q.y = Math::Clamp(p.y, -o_extent.y, o_extent.y);
		q.z = Math::Clamp(p.z, -o_extent.z, o_extent.z);

		if (Math::NearZero(seg_direction_lenSq))
		{
			// The capsule degenerates to a sphere.
			break;
		}

		/*
		this p is a point. not local variable p.
		p, proj
		(p - proj) * d = 0
		(p - (a + dt)) * d = 0
		pd - ad - d^2 * t = 0
		d^2 * t = pd - ad
		t = (pd - ad) / d^2
		*/
		float t = Math::Clamp(FVector3::Dot(q - seg_local_start, seg_direction) / seg_direction_lenSq, 0.f, 1.f);
		p = seg_local_start + t * seg_direction;
	}


	FVector3 diff_p_q = GetNormalDirection(p, q);
	float dist_p_q = diff_p_q.LengthSq();

	float c_radius = c.GetRadius();

	if (dist_p_q > c_radius * c_radius)
	{
		return false;
	}

	OutResult.BlockingHit = true;
	OutResult.ImpactPoint = FVector3::Transform(q, o.GetRotation()) + o_center;

	FVector3 normal_local;


	//inside
	if (Math::NearZero(dist_p_q))
	{
		FVector3 p_abs(Math::Abs(p.x), Math::Abs(p.y), Math::Abs(p.z));
		float dx = o_extent.x - p_abs.x;
		float dy = o_extent.y - p_abs.y;
		float dz = o_extent.z - p_abs.z;

		if (dx < dy && dx < dz)
		{
			normal_local = FVector3(((p.x > 0) ? 1.f : -1.f), 0, 0);
		}
		else if (dy < dz)
		{
			normal_local = FVector3(0, ((p.y > 0) ? 1.f : -1.f), 0);
		}
		else
		{
			normal_local = FVector3(0, 0, ((p.z > 0) ? 1.f : -1.f));
		}
	}
	else
	{
		normal_local = diff_p_q / Math::Sqrt(dist_p_q);
	}

	OutResult.Normal = FVector3::Transform(normal_local, o.GetRotation());
	OutResult.PenetrationDepth = c_radius - Math::Sqrt(dist_p_q);

	return true;
}

bool CollisionStatics::Intersect(const FCapsule& c, const FAABB& box, FHitResult& OutResult)
{
	return false;
}

bool CollisionStatics::Intersect(const FSphere& s, const FAABB& box, FHitResult& OutResult)
{
	return false;
}

bool CollisionStatics::Intersect(const FSphere& a, const FSphere& b, FHitResult& OutResult)
{
	return false;
}

bool CollisionStatics::Intersect(const FSphere& s, const FOBB& o, FHitResult& OutResult)
{
	return false;
}

bool CollisionStatics::Intersect(const FLineSegment& l, const FSphere& s, FHitResult& OutResult)
{
	return false;
}

bool CollisionStatics::Intersect(const FLineSegment& l, const Plane& p, FHitResult& OutResult)
{
	return false;
}

bool CollisionStatics::Intersect(const FLineSegment& l, const FAABB& b, FHitResult& OutResult)
{
	return false;
}

bool CollisionStatics::Intersect(const FLineSegment& l, const FCapsule& C, FHitResult& OutResult)
{
	return false;
}

bool CollisionStatics::Intersect(const FLineSegment& l, const FOBB& O, FHitResult& OutResult)
{
	return false;
}

bool CollisionStatics::TestSidePlane(float start, float end, float d, const FVector3& norm, std::vector<std::pair<float, FVector3>>& out)
{
	return false;
}

FVector3 CollisionStatics::GetNormalDirection(const FVector3& Caller, const FVector3& Target)
{

	return Caller - Target;
}

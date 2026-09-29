#include "../../Public/Collision/GJKStatics.h"

const int GJKStatics::MaxIterations = 64;
const int GJKStatics::BisectionIterations = 24;

bool GJKStatics::Intersect(const FCollision& A, const FCollision& B)
{
    return false;
}

bool GJKStatics::SweepIntersect(const FCollision& A, const FVector3& Displacement, const FCollision& B, float& outT)
{
    return false;
}

bool GJKStatics::RunGJK(const SupportFunc& MinkowskiSupport)
{
    return false;
}

bool GJKStatics::Simplex(TArray<FVector3>& points, FVector3& direction)
{
    switch (points.size())
    {
    case 2:
        return SimplexLine(points, direction);
    case 3:
        return SimplexTriangle(points, direction);
    case 4:
        return SimplexTetrahedron(points, direction);
    }

    return false;
}

bool GJKStatics::SimplexLine(TArray<FVector3>& points, FVector3& direction)
{
    /*
    points 크기가 2일 때 인덱스 0을 a, 1을 b라고 할 때
    원점의 최근접점은 <a, a~b, b< 역역 중 하나에 존재함

    b에 최근접점이 가까운 경우는 <a, a~b에 속하지 않게 하기 위해 선분 t가 1 초과여야 함
    a는 방향 -b에서 가장 멀리 간 점이므로 
    a * -b >= b * -b
    a * -b >= -|b|^2
    a * b <= |b|^2 <-- 1번

    원점과 선분 사이 최소 거리가 되는 t를 구하면
    f(t) = |a + (b-a)t|^2
    
    f'(t) = 2(a + (b-a)t)(b-a) = 2a*(b-a) + 2(b-a)^2t = 0
    a*(b-a) + (b-a)^2t = 0
    t = -a*(b-a) / |b-a|^2

    b가 최근접점이 되려면 t > 1이어야 함
    -a*(b-a) > (b-a)^2
    -a*b + |a|^2 >= |b|^2 - 2b*a + |a|^2
    -a*b > |b|^2 - 2b*a
    0 > |b|^2 - b*a
    b*a > |b|^2 <-- 2번

    1번과 2번이 모순이므로 불가능
    */

    FVector3 a = points[0];
    FVector3 b = points[1];
    FVector3 ab = b - a;
    FVector3 ao = -a;

    if (SameDirection(ab, ao))
    {
        //원점의 최근접점이 선분 ab 위에 존재해서 그쪽으로 다음 방향을 잡음
        direction = FVector3::Cross(FVector3::Cross(ab, ao), ab);
    }
    else
    {
        //a에 가까워서 심플렉스 형성을 위해 불필요해진 b를 날림
        points.pop_back();
        direction = ao;
    }

    //선분은 부피가 없어서 내부 존재 판별이 불가능
    //선분과 원점이 완벽히 겹칠 수 있지 않을까 싶지만 그럴 가능성은 거의 없을 것 같음...
    return false;
}

bool GJKStatics::SimplexTriangle(TArray<FVector3>& points, FVector3& direction)
{
    return false;
}

bool GJKStatics::SimplexTetrahedron(TArray<FVector3>& points, FVector3& direction)
{
    return false;
}

bool GJKStatics::SameDirection(const FVector3& direction, const FVector3& v)
{
    return FVector3::Dot(direction, v) > 0.f;
}

FVector3 GJKStatics::SafeNormalize(const FVector3& v)
{
    float lenSq = v.Length();
    return Math::NearZero(lenSq) ? FVector3::UnitX : v / Math::Sqrt(lenSq);
}

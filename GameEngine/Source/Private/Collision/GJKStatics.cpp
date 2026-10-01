#include "../../Public/Collision/GJKStatics.h"

const int GJKStatics::MaxIterations = 64;
const int GJKStatics::BisectionIterations = 24;

bool GJKStatics::Intersect(const FCollision& A, const FCollision& B)
{
    return RunGJK([&A, &B](const FVector3& d)
        {
            // d, -d는 크기가 같으므로 정규화를 여기서 딱 한 번만 하고 양쪽에 재사용
            FVector3 n = SafeNormalize(d);
            return A.Support(n) - B.Support(n * -1.0f);
        });
}

bool GJKStatics::SweepIntersect(const FCollision& A, const FVector3& Displacement, const FCollision& B, float& outT)
{
    //민코프스키 차에 이동값을 더해서 궤적을 훌어서 겹치긴 하는지 검사
    bool sweep_overlap = RunGJK([&A, &B , &Displacement](const FVector3& d)
        {
            FVector3 n = SafeNormalize(d);
            FVector3 support = (FVector3::Dot(Displacement, n) >= 0.0f) ? Displacement : FVector3::Zero;
            return A.Support(n) + support - B.Support(-n);
        }
    );

    //아무것도 없음
    if (sweep_overlap == false)
    {
        return false;
    }

    //시작 전 이미 겹치나
    bool overlaps_at_start = RunGJK([&A, &B](const FVector3& d)
        {
            FVector3 n = SafeNormalize(d);
            return A.Support(n) - B.Support(-n);
        });

    if (overlaps_at_start)
    {
        outT = 0;
        return true;
    }

    //겹침이 확인됐으므로 이분탐색으로 충돌 시점을 좁힘
    float a = 0.f;
    float b = 1.f;

    for (int32 i = 0; i < BisectionIterations; ++i)
    {
        float mid = (a + b) * 0.5f;
        FVector3 offset = Displacement * mid;

        bool overlap_mid = RunGJK([&A, &B, &offset](const FVector3& d)
            {
                FVector3 n = SafeNormalize(d);
                return A.Support(n) + offset - B.Support(n * -1.0f);
            }
        );

        if (overlap_mid)
        {
            b = mid;
        }
        else
        {
            a = mid;
        }
    }

    outT = b;

    return true;
}

bool GJKStatics::RunGJK(const SupportFunc& MinkowskiSupport)
{
    FVector3 direction = FVector3::UnitX;

    FVector3 support = MinkowskiSupport(direction);

    TArray<FVector3> points;
    points.insert(points.begin(), support);

    direction *= -1.f;

    for (int iter = 0; iter < MaxIterations; ++iter)
    {
        support = MinkowskiSupport(direction);

        // [증명] 0 in A⊖B(=겹침)라고 가정하면, 그 0도 A⊖B의 한 원소니까 서포트(최댓값)
        // 정의에 의해 support·direction >= 0·direction = 0 이 항상 성립해야 함
        if (FVector3::Dot(support, direction) < 0.f)
        {
            return false;
        }

        points.insert(points.begin(), support);

        if (Simplex(points, direction))
        {
            return true;
        }
    }

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
    원점의 최근접점은 <a, a~b, b< 영역 중 하나에 존재함

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
        points = { a };
        direction = ao;
    }

    //선분은 부피가 없어서 내부 존재 판별이 불가능
    //선분과 원점이 완벽히 겹칠 수 있지 않을까 싶지만 그럴 가능성은 거의 없을 것 같음...
    return false;
}

bool GJKStatics::SimplexTriangle(TArray<FVector3>& points, FVector3& direction)
{
    FVector3 a = points[0];
    FVector3 b = points[1];
    FVector3 c = points[2];

    FVector3 ab = b - a;
    FVector3 ac = c - a;
    FVector3 ao = -a;

    FVector3 abc = FVector3::Cross(ab, ac);

    //원점의 최근접점이 ac, a쪽에 존재
    //이게 호출될 시점에서는 bc는 처리됐음
    if (SameDirection(FVector3::Cross(abc, ac), ao))
    {
        //ac쪽에 존재
        if (SameDirection(ac, ao))
        {
            points = {a, c};
            direction = FVector3::Cross(FVector3::Cross(ac, ao), ac);
        }
        else //a쪽에 존재
        {
            points = { a, b };
            return SimplexLine(points, direction);
        }
    }
    else
    {
        //a, ab쪽에 존재
        if (SameDirection(FVector3::Cross(ab, abc), ao))
        {
            points = { a, b };

            //a, ab 중 어디인지는 얘가 대신 판정
            return SimplexLine(points, direction);
        }
        else
        {
            if (SameDirection(abc, ao))
            {
                //면 안
                direction = abc;
            }
            else
            {
                //면 안인데 반대편
                points = { a, c, b };
                direction = -abc;
            }
        }
    }

    //아직 삼각형 평면에 해당하는지라 원점을 감쌌다고 판정할 수 없음
    return false;
}

bool GJKStatics::SimplexTetrahedron(TArray<FVector3>& points, FVector3& direction)
{
    //사면체는 삼각형 4개이므로 SimplexTriangle 4회 호출과 차이가 없는데 삼각형에서 하나 검사헀을 테니 3번 호출하면 됨

    FVector3 a = points[0];
    FVector3 b = points[1];
    FVector3 c = points[2];
    FVector3 d = points[3];

    FVector3 ab = b - a;
    FVector3 ac = c - a;
    FVector3 ad = d - a;
    FVector3 ao = a * -1.0f;

    FVector3 abc = FVector3::Cross(ab, ac);
    FVector3 acd = FVector3::Cross(ac, ad);
    FVector3 adb = FVector3::Cross(ad, ab);
    //bcd가 이전 삼각형

    if (SameDirection(abc, ao))
    {
        points = { a,b,c };
        return SimplexTriangle(points, direction);
    }
    if (SameDirection(acd, ao))
    {
        points = { a,c,d };
        return SimplexTriangle(points, direction);
    }
    if (SameDirection(adb, ao))
    {
        points = { a,d,b };
        return SimplexTriangle(points, direction);
    }
    
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

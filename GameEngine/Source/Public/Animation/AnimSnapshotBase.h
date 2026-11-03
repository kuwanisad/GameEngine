#pragma once

//애니메이션 랜더링이 비동기라 중간에 값이 바뀌면 결과가 일그러지므로 스냅샷을 찍어 랜더링측에 넘겨야 함
struct FAnimSnapshotBase
{
};
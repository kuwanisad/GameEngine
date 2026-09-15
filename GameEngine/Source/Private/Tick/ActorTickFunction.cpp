#include "../../Public/Tick/ActorTickFunction.h"
#include "../../Public/Actor/Actor.h"

void FActorTickFunction::ExecuteTick(float DeltaTime)
{
	if (Target != nullptr)
	{
		Target->Tick(DeltaTime);
	}
}


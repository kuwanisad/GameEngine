#include "../../Public/Tick/ActorTickFunction.h"
#include "../Actors/AActor.h"


void FActorTickFunction::ExecuteTick(float DeltaTime)
{
	if (Target != nullptr)
	{
		Target->Tick(DeltaTime);
	}
}


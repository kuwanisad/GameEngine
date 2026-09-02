#include "../../Public/Tick/ActorComponentTickFunction.h"
#include "../../Component/UActorComponent.h"

void FActorComponentTickFunction::ExecuteTick(float DeltaTime)
{
	if (Target != nullptr)
	{
		Target->Tick(DeltaTime);
	}
}
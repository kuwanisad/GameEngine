#include "../../Public/Tick/ActorComponentTickFunction.h"
#include "../../Public/Component/ActorComponent.h"

void FActorComponentTickFunction::ExecuteTick(float DeltaTime)
{
	if (Target != nullptr)
	{
		Target->Tick(DeltaTime);
	}
}
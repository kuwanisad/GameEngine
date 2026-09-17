#include "../../Public/Component/ShapeComponent.h"
#include "../../Public/Collision/Collision.h"
#include "../../Public/Actor/Actor.h"

UShapeComponent::UShapeComponent(AActor* _Owner) : 
    UPrimitiveComponent(_Owner),
    _Collision(nullptr)
{
    Type = EComponentType::Shape;
}

UShapeComponent::~UShapeComponent()
{
    delete _Collision;
}

void UShapeComponent::SetCollision(FCollision* InCollision)
{
    if (_Collision != InCollision)
    {
        delete _Collision;
    }

    _Collision = InCollision;

    if (_Collision)
    {
        _Collision->SetOwner(Owner);
        GetOwner()->RegisterPhysicsBodyIfNeeded();
    }
}

FCollision* UShapeComponent::GetCollision() const
{
    return nullptr;
}

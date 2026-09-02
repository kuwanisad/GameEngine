#include "../../Public/Component/AttachmentTransformRules.h"

const FAttachmentTransformRules FAttachmentTransformRules::KeepRelativeTransform(EAttachmentRule::KeepRelative);
const FAttachmentTransformRules FAttachmentTransformRules::KeepWorldTransform(EAttachmentRule::KeepWorld);
const FAttachmentTransformRules FAttachmentTransformRules::SnapToTargetTransform(EAttachmentRule::SnapToTarget);

const FDetachmentTransformRules FDetachmentTransformRules::KeepRelativeTransform(EDetachmentRule::KeepRelative);
const FDetachmentTransformRules FDetachmentTransformRules::KeepWorldTransform(EDetachmentRule::KeepWorld);
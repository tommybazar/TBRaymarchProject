// Copyright 2021 Tomas Bartipan and Technical University of Munich.
// Licensed under MIT license - See License.txt for details.
// Special credits go to : Temaran (compute shader tutorial), TheHugeManatee (original concept, supervision) and Ryan Brucks
// (original raymarching code).

#include "Actor/VR/Grabbable.h"

void IGrabbable::OnGrabbed(USceneComponent* Grabber)
{
	AActor* ThisAsActor = Cast<AActor>(this);
	if (ThisAsActor)
	{
		ThisAsActor->AttachToComponent(Grabber, FAttachmentTransformRules::KeepWorldTransform);
	}
}

void IGrabbable::OnReleased()
{
	AActor* ThisAsActor = Cast<AActor>(this);
	if (ThisAsActor)
	{
		ThisAsActor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	}
}

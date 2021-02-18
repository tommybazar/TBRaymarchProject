// Created by Tommy Bazar. No rights reserved :)
// Special credits go to : Temaran (compute shader tutorial), TheHugeManatee (original concept, supervision)
// and Ryan Brucks (original raymarching code).

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

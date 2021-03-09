// Copyright 2021 Tomas Bartipan and Technical University of Munich.
// Licensed under MIT license - See License.txt for details.
// Special credits go to : Temaran (compute shader tutorial), TheHugeManatee (original concept, supervision) and Ryan Brucks
// (original raymarching code).

#include "Actor/VR/VRMenu/VRMenuPanel.h"

AVRMenuPanel::AVRMenuPanel()
{
	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>("Static Mesh");
	SetRootComponent(StaticMeshComponent);

	WidgetComponent = CreateDefaultSubobject<UWidgetComponent>("Menu Widget");
	WidgetComponent->SetupAttachment(RootComponent);
}

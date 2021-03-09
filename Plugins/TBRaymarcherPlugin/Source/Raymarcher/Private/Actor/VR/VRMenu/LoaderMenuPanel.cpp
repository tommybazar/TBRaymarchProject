// Copyright 2021 Tomas Bartipan and Technical University of Munich.
// Licensed under MIT license - See License.txt for details.
// Special credits go to : Temaran (compute shader tutorial), TheHugeManatee (original concept, supervision) and Ryan Brucks
// (original raymarching code).

#pragma once

#include "Actor/VR/VRMenu/LoaderMenuPanel.h"

#include "Actor/RaymarchVolume.h"
#include "Widget/VolumeLoadMenu.h"

void ALoaderMenuPanel::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (LoaderMenuClass && WidgetComponent)
	{
		WidgetComponent->SetWidgetClass(LoaderMenuClass);
		LoaderMenu = Cast<UVolumeLoadMenu>(WidgetComponent->GetUserWidgetObject());
		if (!LoaderMenu)
		{
			WidgetComponent->InitWidget();
			LoaderMenu = Cast<UVolumeLoadMenu>(WidgetComponent->GetUserWidgetObject());
			if (!LoaderMenu)
			{
				return;
			}
		}
	}
}

void ALoaderMenuPanel::BeginPlay()
{
	Super::BeginPlay();
	if (!LoaderMenu)
	{
		return;
	}

	LoaderMenu->ListenerVolumes.Empty();
	for (ARaymarchVolume* Volume : ListenerVolumes)
	{
		LoaderMenu->AddListenerVolume(Volume);
	}
}

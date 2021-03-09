// Copyright 2021 Tomas Bartipan and Technical University of Munich.
// Licensed under MIT license - See License.txt for details.
// Special credits go to : Temaran (compute shader tutorial), TheHugeManatee (original concept, supervision) and Ryan Brucks
// (original raymarching code).

#pragma once

#include "Actor/VR/VRMenu/TFMenuPanel.h"
#include "Widget/TransferFuncMenu.h"
#include "Actor/RaymarchVolume.h"

void ATFMenuPanel::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (TransferFuncMenuClass && WidgetComponent)
	{
		// Force initialization of widget on WidgetComponent.
		WidgetComponent->SetWidgetClass(TransferFuncMenuClass);
		TransferFuncMenu = Cast<UTransferFuncMenu>(WidgetComponent->GetUserWidgetObject());
		if (!TransferFuncMenu)
		{
			WidgetComponent->InitWidget();
			TransferFuncMenu = Cast<UTransferFuncMenu>(WidgetComponent->GetUserWidgetObject());
			if (!TransferFuncMenu)
			{
				return;
			}
		}
	}
}

void ATFMenuPanel::BeginPlay()
{
	Super::BeginPlay();
	if (TransferFuncMenuClass && WidgetComponent)
	{
		// Force initialization of widget on WidgetComponent.
		WidgetComponent->SetWidgetClass(TransferFuncMenuClass);
		TransferFuncMenu = Cast<UTransferFuncMenu>(WidgetComponent->GetUserWidgetObject());
		if (!TransferFuncMenu)
		{
			WidgetComponent->InitWidget();
			TransferFuncMenu = Cast<UTransferFuncMenu>(WidgetComponent->GetUserWidgetObject());
			if (!TransferFuncMenu)
			{
				return;
			}
		}
	}

	if (!TransferFuncMenu)
	{
		return;
	}

	// Set volumes to the underlying menu.
	TransferFuncMenu->ListenerVolumes.Empty();
	TransferFuncMenu->SetRangeProviderVolume(ProviderVolume);
	for (ARaymarchVolume* Volume : ListenerVolumes)
	{
		TransferFuncMenu->AddListenerVolume(Volume);
	}
}

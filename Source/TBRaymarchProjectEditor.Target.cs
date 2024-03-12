// Copyright 2021 Tomas Bartipan and Technical University of Munich.
// Licensed under MIT license - See License.txt for details.
// Special credits go to : Temaran (compute shader tutorial), TheHugeManatee (original concept, supervision) and Ryan Brucks (original raymarching code).

using UnrealBuildTool;
using System.Collections.Generic;

public class TBRaymarchProjectEditorTarget : TargetRules
{
	public TBRaymarchProjectEditorTarget( TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		// TODO update to these after stuff works on 5.3
		// DefaultBuildSettings = BuildSettingsVersion.V4;
		// IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		DefaultBuildSettings = BuildSettingsVersion.V2;
		ExtraModuleNames.AddRange( new string[] { "TBRaymarchProject" } );
	}
}

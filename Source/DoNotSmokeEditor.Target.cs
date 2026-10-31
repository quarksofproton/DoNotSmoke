// Copyright Do Not Smoke Team. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class DoNotSmokeEditorTarget : TargetRules
{
	public DoNotSmokeEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("DoNotSmoke");
	}
}

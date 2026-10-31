// Copyright Do Not Smoke Team. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class DoNotSmokeTarget : TargetRules
{
	public DoNotSmokeTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("DoNotSmoke");
	}
}

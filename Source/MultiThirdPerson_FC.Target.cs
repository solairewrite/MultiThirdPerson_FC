// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.Collections.Generic;

public class MultiThirdPerson_FCTarget : TargetRules
{
	public MultiThirdPerson_FCTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;

		ExtraModuleNames.AddRange( new string[] { "MultiThirdPerson_FC" } );
	}
}

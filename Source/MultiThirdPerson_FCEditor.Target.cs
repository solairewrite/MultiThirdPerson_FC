// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.Collections.Generic;

public class MultiThirdPerson_FCEditorTarget : TargetRules
{
	public MultiThirdPerson_FCEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;

		ExtraModuleNames.AddRange( new string[] { "MultiThirdPerson_FC" } );
	}
}

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#define SURFACE_FLESH_DEFAULT			SurfaceType1
#define SURFACE_FLESH_VULNERABLE		SurfaceType2

// 设置武器碰撞频道,避免击中玩家的胶囊体,只允许击中Mesh
#define COLLISION_WEAPON						ECC_GameTraceChannel1 
﻿// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "SGameState.generated.h"


UENUM(BlueprintType)
enum class EWaveState :uint8
{
	WaitingToStart,
	WaveInProgress,
	// 一波Bot生成完成,等待玩家杀死Bot
	WaitingToComplete,
	WaveComplete,
	GameOver,
};
/**
 *
 */
UCLASS()
class MULTITHIRDPERSON_FC_API ASGameState : public AGameStateBase
{
	GENERATED_BODY()

protected:

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_WaveState, Category = "GameState")
		EWaveState WaveState;

	UFUNCTION()
		void OnRep_WaveState(EWaveState OldState);

	UFUNCTION(BlueprintImplementableEvent, Category = "GameState")
		void WaveStateChanged(EWaveState NewState, EWaveState OldState);

public:

	void SetWaveState(EWaveState NewState);

};

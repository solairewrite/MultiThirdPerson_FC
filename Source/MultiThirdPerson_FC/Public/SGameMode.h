// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SGameMode.generated.h"

// 杀死敌人代理
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnActorKilled, AActor*, VictimActor, AActor*, KillerActor, AController*, KillerController);

enum class EWaveState :uint8;

/**
 *
 */
UCLASS()
class MULTITHIRDPERSON_FC_API ASGameMode : public AGameModeBase
{
	GENERATED_BODY()

protected:

	FTimerHandle TimerHandle_BotSpawner;

	int32 NrOfBotsToSpawn;

	int32 WaveCount;

	UPROPERTY(EditDefaultsOnly, Category = "GameMode")
		float TimeBetweenWaves;

	UFUNCTION(BlueprintImplementableEvent, Category = "GameMode")
		void SpawnNewBot();

	// 被绑定的生成Bot的定时任务
	void SpawnBotTimerElapsed(); // 逝去

	void StartWave();

	void EndWave();

	void PrepareForNextWave();

	FTimerHandle TimerHandle_NextWaveStart;

	// 检查是否还有Bot存活
	void CheckWaveState();

	void CheckAnyPlayerAlive();
	void GameOver();

	void SetWaveState(EWaveState NewState);

public:

	ASGameMode();

	virtual void StartPlay() override;

	virtual void Tick(float DeltaSeconds) override;

	// 在蓝图中绑定函数Bind Event
	UPROPERTY(BlueprintAssignable, Category = "GameMode")
		FOnActorKilled onActorKilled;
};

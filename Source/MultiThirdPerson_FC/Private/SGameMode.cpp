// Fill out your copyright notice in the Description page of Project Settings.

#include "Public/SGameMode.h"
#include "Public/Components/SHealthComponent.h"
#include "Public/SGameState.h"
#include "Public/SPlayerState.h"


ASGameMode::ASGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 1.0f;

	TimeBetweenWaves = 2.0f;

	// GameStateClass: 绑定到这个GameMode的GameState的Class
	// GameMode只在服务器上存在
	// 创建GameStateBase同步游戏状态到客户端
	GameStateClass = ASGameState::StaticClass();
	PlayerStateClass = ASPlayerState::StaticClass();
}

void ASGameMode::StartPlay()
{
	Super::StartPlay();
	PrepareForNextWave();
}

void ASGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	CheckWaveState();
	CheckAnyPlayerAlive();
}

// 等待生成下一波,开启定时任务
void ASGameMode::PrepareForNextWave()
{
	SetWaveState(EWaveState::WaitingToStart);
	//FTimerHandle TimerHandle_NextWaveStart;
	GetWorldTimerManager().SetTimer(TimerHandle_NextWaveStart, this, &ASGameMode::StartWave, TimeBetweenWaves, false);

	RestartDeadPlayers();
}

void ASGameMode::CheckWaveState()
{
	bool bIsPreparingForWave = GetWorldTimerManager().IsTimerActive(TimerHandle_NextWaveStart);
	if (NrOfBotsToSpawn > 0 || bIsPreparingForWave)
	{
		//UE_LOG(LogTemp, Warning, TEXT("返回位置A"));
		return;
	}
	bool bIsAnyBotAlive = false;
	for (FConstPawnIterator It = GetWorld()->GetPawnIterator(); It; ++It)
	{
		APawn* TestPawn = It->Get();
		if (TestPawn == nullptr || TestPawn->IsPlayerControlled())
		{
			continue;
		}
		USHealthComponent* HealthComp = Cast<USHealthComponent>(TestPawn->GetComponentByClass(USHealthComponent::StaticClass()));
		if (HealthComp&&HealthComp->GetHealth() > 0.0f)
		{
			//UE_LOG(LogTemp, Warning, TEXT("返回位置B"));
			bIsAnyBotAlive = true;
			break;
		}
	}
	if (!bIsAnyBotAlive)
	{
		SetWaveState(EWaveState::WaveComplete);
		PrepareForNextWave();
	}
}

void ASGameMode::CheckAnyPlayerAlive()
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; It++)
	{
		APlayerController* PC = It->Get();
		if (PC&&PC->GetPawn())
		{
			APawn* MyPawn = PC->GetPawn();
			USHealthComponent* HealthComp = Cast<USHealthComponent>(MyPawn->GetComponentByClass(USHealthComponent::StaticClass()));
			// ensure类似断点,确保HealthComp存在
			if (ensure(HealthComp) && HealthComp->GetHealth() > 0.0f)
			{
				// 有玩家存活
				return;
			}
		}
	}
	// 没有玩家存活
	GameOver();
}

void ASGameMode::GameOver()
{
	SetWaveState(EWaveState::GameOver);
	EndWave();
	// TODO 结束游戏 显示GameOver
	//UE_LOG(LogTemp, Warning, TEXT("GameOver! Players Died"));
}

void ASGameMode::SetWaveState(EWaveState NewState)
{
	ASGameState* GS = GetGameState<ASGameState>();
	if (ensureAlways(GS))
	{
		GS->SetWaveState(NewState);
	}
}

void ASGameMode::RestartDeadPlayers()
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (PC&&PC->GetPawn() == nullptr)
		{
			RestartPlayer(PC);
		}
	}
}

void ASGameMode::StartWave()
{
	SetWaveState(EWaveState::WaveInProgress);
	WaveCount++;
	NrOfBotsToSpawn = WaveCount * 2;
	GetWorldTimerManager().SetTimer(TimerHandle_BotSpawner, this, &ASGameMode::SpawnBotTimerElapsed, 1.0f, true, 0.0f);
}

void ASGameMode::SpawnBotTimerElapsed()
{
	SpawnNewBot();
	NrOfBotsToSpawn--;
	if (NrOfBotsToSpawn <= 0)
	{
		EndWave();
	}
}

void ASGameMode::EndWave()
{
	SetWaveState(EWaveState::WaitingToComplete);
	GetWorldTimerManager().ClearTimer(TimerHandle_BotSpawner);
	//PrepareForNextWave();
}
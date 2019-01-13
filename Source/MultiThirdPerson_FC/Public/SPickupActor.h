// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SPickupActor.generated.h"


class USphereComponent;
class UDecalComponent;
class ASPowerupActor;

UCLASS()
class MULTITHIRDPERSON_FC_API ASPickupActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ASPickupActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Components")
		USphereComponent* SphereComp;
	UPROPERTY(VisibleAnywhere, Category = "Components")
		UDecalComponent* DecalComp;

	// 用于生成实例的类
	UPROPERTY(EditInstanceOnly, Category = "PickupActor")
		TSubclassOf<ASPowerupActor> PowerupClass;
	// 根据类生成的实例
	ASPowerupActor* PowerupInstance;

	UPROPERTY(EditDefaultsOnly, Category = "PickupActor")
		float CooldownDuration;

	FTimerHandle TimerHandle_RespawnTimer;

	void Respawn();

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

};

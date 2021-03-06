﻿// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class ASWeapon;
class USHealthComponent;


UCLASS()
class MULTITHIRDPERSON_FC_API ASCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ASCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void MoveForward(float value);
	void MoveRight(float value);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
		UCameraComponent* CameraComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
		USpringArmComponent* SpringArmComp;

	void BeginCrouch();
	void EndCrouch();

	virtual FVector GetPawnViewLocation() const override;

	bool bWantsToZoom;

	UPROPERTY(EditDefaultsOnly, Category = "Player")
		float ZoomedFOV;

	UPROPERTY(EditDefaultsOnly, Category = "Player", meta = (ClampMin = 0.1, ClampMax = 100.0))
		float ZoomInterpSpeed;

	float DefaultFOV;

	void BeginZoom();
	void EndZoom();

	UPROPERTY(EditDefaultsOnly, Category = "Player", meta = (ClampMin = 1.0, ClampMax = 2.0))
		float SpeedUpRate;

	float DefaultWalkSpeed;
	float DefaultCrouchWalkSpeed;

	void BeginSpeedUp();
	void EndSpeedUp();


	UPROPERTY(Replicated, BlueprintReadWrite, Category = "Components")
		ASWeapon* CurrentWeapon;

	UPROPERTY(EditDefaultsOnly, Category = "Player")
		TSubclassOf<ASWeapon> FirstWeaponClass;

	UPROPERTY(EditDefaultsOnly, Category = "Player")
		TSubclassOf<ASWeapon> SecondWeaponClass;

	UPROPERTY(VisibleDefaultsOnly, Category = "Player")
		FName WeaponAttackSocketName;

	void EquipWeapon(TSubclassOf<ASWeapon> WeaponClass);
	void EquipFirstWeapon();
	void EquipSecondWeapon();

	// VisibleAnywhere:可以在属性窗口查看
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player")
		USHealthComponent* HealthComp;

	UFUNCTION()
		void OnHealthChanged(USHealthComponent* HealthComponent, float Health, float HealthDelta, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);

	// 动画蓝图中控制死亡的变量
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Player")
		bool bDied;

	UFUNCTION(BlueprintImplementableEvent)
		void OnDied();

	// 控制换枪动画
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Player")
		bool bPressedEquip;

	UPROPERTY(EditDefaultsOnly, Category = "Player")
		float EquipAnimLength;

	void StopEquipAnim();

	// 死亡后多久消失
	UPROPERTY(EditDefaultsOnly, Category = "Player")
		float DestroyTime;

	void DestroyWeapon();

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintCallable, Category = "Player")
		void StartFire();
	UFUNCTION(BlueprintCallable, Category = "Player")
		void StopFire();

};

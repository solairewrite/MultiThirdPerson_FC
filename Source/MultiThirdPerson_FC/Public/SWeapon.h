// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SWeapon.generated.h"

class USkeletalMeshComponent;
class UParticleSystem;
class UPhysicalMaterial;
class USoundBase;

USTRUCT()
struct FHitScanTrace
{
	GENERATED_BODY()
public:
	UPROPERTY()
		FVector_NetQuantize TraceFrom; // 简化数据
	UPROPERTY()
		FVector_NetQuantize TraceTo;
	UPROPERTY()
		FRotator HitPointRotation;
	UPROPERTY()
		UPhysicalMaterial* HitPhysMaterial;
};

UCLASS()
class MULTITHIRDPERSON_FC_API ASWeapon : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ASWeapon();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// VisibleAnywhere: 属性窗口可见
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
		USkeletalMeshComponent* MeshComp;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
		TSubclassOf<UDamageType> DamageType;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
		FName MuzzleSocketName; // 枪口
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
		FName TracerTargetName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
		UParticleSystem* MuzzleEffect;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
		UParticleSystem* TracerEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
		UParticleSystem* DefaultImpactEffect;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
		UParticleSystem* FleshImpactEffect;

	// 根据物理皮肤,播放中枪效果
	void PlayImpactEffect(FHitScanTrace Hit);

	void PlayFireEffects(FVector HitPoint);

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
		TSubclassOf<UCameraShake> FireCamShake;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
		float BaseDamage;

	FTimerHandle TimerHandle_TimeBetweenShots;
	float LastFireTime;
	// 每分钟射击次数
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
		float RateOfFire;
	float TimeBetweenShots;

	UFUNCTION(BlueprintCallable, Category = "Weapon")
		virtual void Fire();

	// Server: 不在客户端执行方法,而是请求服务器
	UFUNCTION(Server, Reliable, WithValidation)
		void ServerFire();

	// 每次复制,执行OnRep_HitScanTrace
	UPROPERTY(ReplicatedUsing = OnRep_HitScanTrace)
		FHitScanTrace HitScanTrace;
	UFUNCTION()
		void OnRep_HitScanTrace();

	UPROPERTY(EditDefaultsOnly, Category = "Sound")
		USoundBase* FireSound;

	UPROPERTY(EditDefaultsOnly, Category = "SOund")
		float FireSoundVolume;

	// 子弹在随机椎体内,锥体角度
	UPROPERTY(EditDefaultsOnly, Category = "Weapon", meta = (ClampMin = 0.0f))
		float BulletSpread;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void StartFire();
	void StopFire();
};

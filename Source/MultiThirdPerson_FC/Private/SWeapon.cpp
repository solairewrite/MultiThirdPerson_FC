// Fill out your copyright notice in the Description page of Project Settings.

#include "Public/SWeapon.h"
#include "Components/SkeletalMeshComponent.h"
#include "CollisionQueryParams.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "MultiThirdPerson_FC.h"
#include "TimerManager.h"

// 自定义控制台命令
static int32 DebugWeaponDrawing = 0;
FAutoConsoleVariableRef CVARDebugWeaponDrawing(
	TEXT("DebugWeapons"), // 调用控制台命令
	DebugWeaponDrawing,
	TEXT("武器绘制调试射线"),
	ECVF_Cheat // 发布后无法在控制台调用
);

// Sets default values
ASWeapon::ASWeapon()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComp"));
	SetRootComponent(MeshComp);
	//MeshComp->SetupAttachment(RootComponent);

	MuzzleSocketName = "MuzzleSocket";
	TracerTargetName = "Target";

	BaseDamage = 20.0f;

	RateOfFire = 300;
}

// Called when the game starts or when spawned
void ASWeapon::BeginPlay()
{
	Super::BeginPlay();

	TimeBetweenShots = 60 / RateOfFire;
}

void ASWeapon::Fire()
{
	// 记录上一次开火的时间,时间冷却内无法射击
	LastFireTime = GetWorld()->TimeSeconds;

	AActor* MyOwner = GetOwner();
	if (MyOwner)
	{
		FVector EyeLocation;
		FRotator EyeRotation;
		// virtual void GetActorEyesViewPoint( FVector& OutLocation, FRotator& OutRotation ) const;
		// 使用引用参数,用于改变实参的值
		MyOwner->GetActorEyesViewPoint(EyeLocation, EyeRotation);
		FVector TraceEnd = EyeLocation + (EyeRotation.Vector() * 10000);

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(MyOwner);
		QueryParams.AddIgnoredActor(this);
		QueryParams.bTraceComplex = true;
		QueryParams.bReturnPhysicalMaterial = true;
		FHitResult Hit;

		// 返回bool是否碰撞,COLLISION_WEAPON是自定义的碰撞频道
		if (GetWorld()->LineTraceSingleByChannel(Hit, EyeLocation, TraceEnd, COLLISION_WEAPON, QueryParams))
		{
			// 爆头伤害加倍
			float ActualDamage = BaseDamage;
			EPhysicalSurface SurfaceType = UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get());
			if (SurfaceType == SURFACE_FLESH_VULNERABLE)
			{
				ActualDamage *= 4.0f;
			}

			// 伤害处理
			AActor* HitActor = Hit.GetActor();
			UGameplayStatics::ApplyPointDamage(HitActor, ActualDamage, EyeLocation, Hit, MyOwner->GetInstigatorController(), this, DamageType);

			PlayFireEffects(Hit, EyeLocation);
		}
	}
}

void ASWeapon::StartFire()
{
	// 射击开始时间是 上一次开火时间+冷却时间-当前时间
	// 按下开火,抬起停火
	float FirstDelay = FMath::Max(LastFireTime + TimeBetweenShots - GetWorld()->TimeSeconds, 0.0f);
	GetWorldTimerManager().SetTimer(TimerHandle_TimeBetweenShots, this, &ASWeapon::Fire, TimeBetweenShots, true, FirstDelay);
}

void ASWeapon::StopFire()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_TimeBetweenShots);
}

void ASWeapon::PlayFireEffects(FHitResult Hit, FVector EyeLocation)
{
	FVector HitPointLocation = Hit.ImpactPoint;

	// 镜头抖动
	APawn* MyOwner = Cast<APawn>(GetOwner());
	if (MyOwner)
	{
		APlayerController* PC = Cast<APlayerController>(MyOwner->GetController());
		if (PC)
		{
			PC->ClientPlayCameraShake(FireCamShake);
		}
	}
	// 中枪位置效果
	PlayImpactEffect(Hit);

	// 子弹轨迹射线
	if (DebugWeaponDrawing > 0)
	{
		DrawDebugLine(GetWorld(), EyeLocation, HitPointLocation, FColor::Red, false, 1.0f, 0, 1.0f);
	}
	// 枪口特效
	if (MuzzleEffect)
	{
		UGameplayStatics::SpawnEmitterAttached(MuzzleEffect, MeshComp, MuzzleSocketName);
		if (DebugWeaponDrawing > 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("播放枪口特效"))
		}
	}
	// 子弹轨迹射线
	if (TracerEffect)
	{
		FVector MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);
		UParticleSystemComponent* TracerComp = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), TracerEffect, MuzzleLocation);
		if (TracerComp)
		{
			TracerComp->SetVectorParameter(TracerTargetName, HitPointLocation);
		}
	}
}


void ASWeapon::PlayImpactEffect(FHitResult Hit)
{
	EPhysicalSurface SurfaceType = UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get());
	UParticleSystem* SelectedEffect = nullptr;
	switch (SurfaceType)
	{
	case SURFACE_FLESH_DEFAULT:
	case SURFACE_FLESH_VULNERABLE:
		SelectedEffect = FleshImpactEffect;
		break;
	default:
		SelectedEffect = DefaultImpactEffect;
		break;
	}

	if (SelectedEffect)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), SelectedEffect, Hit.ImpactPoint, Hit.ImpactNormal.Rotation());
	}
}

// Called every frame
void ASWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}


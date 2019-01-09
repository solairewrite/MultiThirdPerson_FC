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
#include "Net/UnrealNetwork.h"

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

	SetReplicates(true);

	// 客户端同步更新频率
	NetUpdateFrequency = 66.0f;
	MinNetUpdateFrequency = 33.0f;
}

// Called when the game starts or when spawned
void ASWeapon::BeginPlay()
{
	Super::BeginPlay();

	TimeBetweenShots = 60 / RateOfFire;
}

void ASWeapon::Fire()
{
	// 如果不是服务器,发送请求到服务器,执行这段代码
	if (Role < ROLE_Authority)
	{
		ServerFire();
	}

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

			// 中枪位置效果
			PlayFireEffects(Hit.ImpactPoint);
			if (Role == ROLE_Authority)
			{
				HitScanTrace.TraceFrom = EyeLocation;
				HitScanTrace.TraceTo = Hit.ImpactPoint;
				HitScanTrace.HitPhysMaterial = Hit.PhysMaterial.Get();
				HitScanTrace.HitPointRotation = Hit.ImpactNormal.Rotation();
			}

			// 记录上一次开火的时间,时间冷却内无法射击
			LastFireTime = GetWorld()->TimeSeconds;
		}
	}
}

void ASWeapon::OnRep_HitScanTrace()
{
	PlayFireEffects(HitScanTrace.TraceTo);
}

// 实现服务器方法,在后面加上 _Implementation
void ASWeapon::ServerFire_Implementation()
{
	Fire();
}
// 服务器方法,一般return true即可
bool ASWeapon::ServerFire_Validate()
{
	return true;
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

void ASWeapon::PlayFireEffects(FVector HitPoint)
{

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

	// 被击中点的特效
	PlayImpactEffect(HitScanTrace);

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
	if (DebugWeaponDrawing > 0)
	{
		DrawDebugLine(GetWorld(), HitScanTrace.TraceFrom, HitPoint, FColor::Red, false, 1.0f, 0, 1.0f);
	}

	// 子弹轨迹射线
	if (TracerEffect)
	{
		FVector MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);
		UParticleSystemComponent* TracerComp = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), TracerEffect, MuzzleLocation);
		if (TracerComp)
		{
			TracerComp->SetVectorParameter(TracerTargetName, HitPoint);
		}
	}
}

void ASWeapon::PlayImpactEffect(FHitScanTrace Hit)
{
	EPhysicalSurface SurfaceType = UPhysicalMaterial::DetermineSurfaceType(Hit.HitPhysMaterial);
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
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), SelectedEffect, Hit.TraceTo, Hit.HitPointRotation);
	}
}

// Called every frame
void ASWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ASWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 避免发起的客户端复制
	DOREPLIFETIME_CONDITION(ASWeapon, HitScanTrace, COND_SkipOwner);
}
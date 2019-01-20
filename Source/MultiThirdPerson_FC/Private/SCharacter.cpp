// Fill out your copyright notice in the Description page of Project Settings.

#include "Public/SCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Public/SWeapon.h"
#include "Engine/World.h"
#include "Components/InputComponent.h"
#include "Components/CapsuleComponent.h"
#include "MultiThirdPerson_FC.h"
#include "SHealthComponent.h"
#include "Net/UnrealNetwork.h"

static int32 DebugLogLevel = 0;
FAutoConsoleVariableRef CVARDebugLogLevel(
	TEXT("DebugLogLevel"),
	DebugLogLevel,
	TEXT("是否可以输出,默认0,大于0可以输出"),
	ECVF_Cheat
);

// Sets default values
ASCharacter::ASCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComp"));
	SpringArmComp->bUsePawnControlRotation = true;
	SpringArmComp->SetupAttachment(RootComponent);

	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
	CameraComp->SetupAttachment(SpringArmComp);

	GetMovementComponent()->GetNavAgentPropertiesRef().bCanCrouch = true;

	ZoomedFOV = 65.0f;
	ZoomInterpSpeed = 20;

	SpeedUpRate = 1.2;

	WeaponAttackSocketName = "WeaponSocket";

	// 禁止子弹打到胶囊体,只允许打到Mesh
	// 可以观察PlayerPawn -. CapsuleComponent -> Collision Presets -> Weapon: Block
	GetCapsuleComponent()->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Ignore);

	// 添加血量组件
	HealthComp = CreateDefaultSubobject<USHealthComponent>(TEXT("HealthComp"));
	bDied = false;
	bPressedEquip = false;
	EquipAnimLength = 1.5f;

	DestroyTime = 5.0f;
}

// Called when the game starts or when spawned
void ASCharacter::BeginPlay()
{
	Super::BeginPlay();

	DefaultFOV = CameraComp->FieldOfView;
	DefaultWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
	DefaultCrouchWalkSpeed = GetCharacterMovement()->MaxWalkSpeedCrouched;

	// 生成武器
	EquipFirstWeapon();

	// 绑定掉血方法
	HealthComp->OnHealthChanged.AddDynamic(this, &ASCharacter::OnHealthChanged);
}

void ASCharacter::MoveForward(float value)
{
	AddMovementInput(GetActorForwardVector()*value);
}

void ASCharacter::MoveRight(float value)
{
	AddMovementInput(GetActorRightVector()*value);
}

void ASCharacter::BeginCrouch()
{
	Crouch();
}

void ASCharacter::EndCrouch()
{
	UnCrouch();
}

FVector ASCharacter::GetPawnViewLocation() const
{
	if (CameraComp)
	{
		return CameraComp->GetComponentLocation();
	}
	return Super::GetPawnViewLocation();
}

void ASCharacter::BeginZoom()
{
	bWantsToZoom = true;
}

void ASCharacter::EndZoom()
{
	bWantsToZoom = false;
}

void ASCharacter::BeginSpeedUp()
{
	GetCharacterMovement()->MaxWalkSpeed = DefaultWalkSpeed * SpeedUpRate;
	GetCharacterMovement()->MaxWalkSpeedCrouched = DefaultCrouchWalkSpeed * SpeedUpRate;
}

void ASCharacter::EndSpeedUp()
{
	GetCharacterMovement()->MaxWalkSpeed = DefaultWalkSpeed;
	GetCharacterMovement()->MaxWalkSpeedCrouched = DefaultCrouchWalkSpeed;
}

void ASCharacter::EquipWeapon(TSubclassOf<ASWeapon> WeaponClass)
{
	UE_LOG(LogTemp, Warning, TEXT("装备武器  %s %s"), *(GetName()), *(WeaponClass->GetFName().ToString()));
	// TODO 播放Equip动画,在学习死亡动画的时候添加
	if (!WeaponClass || CurrentWeapon->StaticClass == WeaponClass->StaticClass)
	{

		if (DebugLogLevel > 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("未设置武器或无需切换武器  %s"), *(WeaponClass->GetFName().ToString()));
		}
		return;
	}

	DestroyWeapon();

	bPressedEquip = true;
	FTimerHandle TimerHandle_StopEquipAnim;
	GetWorldTimerManager().SetTimer(TimerHandle_StopEquipAnim, this, &ASCharacter::StopEquipAnim, EquipAnimLength
		, false);

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	CurrentWeapon = GetWorld()->SpawnActor<ASWeapon>(WeaponClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

	if (CurrentWeapon)
	{
		CurrentWeapon->SetOwner(this); // 对应SWeapon::Fire()的GetOwner
		CurrentWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponAttackSocketName);
	}
}

void ASCharacter::EquipFirstWeapon()
{
	EquipWeapon(FirstWeaponClass);
}

void ASCharacter::EquipSecondWeapon()
{
	EquipWeapon(SecondWeaponClass);
}

void ASCharacter::StartFire()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->StartFire();
	}
}

void ASCharacter::StopFire()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->StopFire();
	}
}

void ASCharacter::OnHealthChanged(USHealthComponent* HealthComponent, float Health, float HealthDelta, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser)
{
	if (Health <= 0.0f && !bDied) // 死亡
	{
		// 控制播放死亡动画,在蓝图中通过bDied设置
		bDied = true;
		// 销毁武器
		FTimerHandle TimerHandle_DestroyWeapon;
		GetWorldTimerManager().SetTimer(TimerHandle_DestroyWeapon, this, &ASCharacter::DestroyWeapon, DestroyTime, false);
		OnDied();
		// 停止开火
		StopFire();
		// 立刻停止移动
		GetMovementComponent()->StopMovementImmediately();
		// 禁用胶囊体,禁用碰撞
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		// 使Pawn与控制器分离,等待销毁
		DetachFromControllerPendingDestroy();
		// 定时销毁
		SetLifeSpan(DestroyTime);
	}
}

void ASCharacter::StopEquipAnim()
{
	bPressedEquip = false;
}

void ASCharacter::DestroyWeapon()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->Destroy();
	}
}

// Called every frame
void ASCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	float TargetFOV = bWantsToZoom ? ZoomedFOV : DefaultFOV;
	// 差值计算
	float NewFOV = FMath::FInterpTo(CameraComp->FieldOfView, TargetFOV, DeltaTime, ZoomInterpSpeed);
	CameraComp->SetFieldOfView(NewFOV);
}

// Called to bind functionality to input
void ASCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{

	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &ASCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ASCharacter::MoveRight);

	PlayerInputComponent->BindAxis("LookUp", this, &ASCharacter::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("Turn", this, &ASCharacter::AddControllerYawInput);

	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &ASCharacter::BeginCrouch);
	PlayerInputComponent->BindAction("Crouch", IE_Released, this, &ASCharacter::EndCrouch);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ASCharacter::Jump);

	PlayerInputComponent->BindAction("Zoom", IE_Pressed, this, &ASCharacter::BeginZoom);
	PlayerInputComponent->BindAction("Zoom", IE_Released, this, &ASCharacter::EndZoom);

	PlayerInputComponent->BindAction("SpeedUp", IE_Pressed, this, &ASCharacter::BeginSpeedUp);
	PlayerInputComponent->BindAction("SpeedUp", IE_Released, this, &ASCharacter::EndSpeedUp);

	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ASCharacter::StartFire);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &ASCharacter::StopFire);

	PlayerInputComponent->BindAction("FirstWeapon", IE_Pressed, this, &ASCharacter::EquipFirstWeapon);
	PlayerInputComponent->BindAction("SecondWeapon", IE_Pressed, this, &ASCharacter::EquipSecondWeapon);
}

void ASCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASCharacter, CurrentWeapon);
	DOREPLIFETIME(ASCharacter, bDied);
	DOREPLIFETIME(ASCharacter, bPressedEquip);
}
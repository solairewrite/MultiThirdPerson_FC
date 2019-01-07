// Fill out your copyright notice in the Description page of Project Settings.

#include "Public/Components/SHealthComponent.h"


// Sets default values for this component's properties
USHealthComponent::USHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	DefaultHealth = 100.0;
}


// Called when the game starts
void USHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	Health = DefaultHealth;

	AActor* MyOwner = GetOwner();
	if (MyOwner)
	{
		MyOwner->OnTakeAnyDamage.AddDynamic(this, &USHealthComponent::HandleTakeAnyDanage);
	}

}


void USHealthComponent::HandleTakeAnyDanage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser)
{
	if (Damage <= 0.0f)
	{
		return;
	}
	Health = FMath::Clamp(Health - Damage, 0.0f, DefaultHealth);
	FString OwnerName = GetOwner()->GetFName().ToString();
	UE_LOG(LogTemp, Warning, TEXT("血量改变: %s %s"), *OwnerName, *FString::SanitizeFloat(Health));

	OnHealthChanged.Broadcast(this, Health, Damage, DamageType, InstigatedBy, DamageCauser);
}

// Called every frame
void USHealthComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}


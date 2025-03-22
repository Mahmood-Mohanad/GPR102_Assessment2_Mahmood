// CG Spectrum 2025

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Turret.generated.h"

class ATurretProjectile;
class ATargetProjectile;


UCLASS()
class TURRETMASTER_API ATurret : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATurret();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Pointer to the current target projectile
	ATargetProjectile* CurrentTargetProjectile = nullptr;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION()
	void Fire() const;


	UFUNCTION()
	void SetYaw(float TargetYaw) const;
	
	UFUNCTION()
	void SetPitch(float TargetPitch) const;

	// Function that will handle the aiming calculations.
	// It takes the projectile information as parameters.
	UFUNCTION()
	void AimingMath(float ProjectileSpeed, FVector ProjectileLocation, FVector ProjectileVelocity, float ProjectileTime);

	// Add a function that will be bound to the launcher’s delegate.
	// This function will receive projectile info.
	UFUNCTION()
	void OnTargetProjectileLaunched(float ProjectileSpeed, FVector ProjectileLocation, FVector ProjectileVelocity, float ProjectileTime, ATargetProjectile* TargetProjectilePtr);

protected:
	// Components
	UPROPERTY(EditDefaultsOnly)
	UStaticMeshComponent* BaseMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	USceneComponent* YawRotator;

	UPROPERTY(EditDefaultsOnly)
	UStaticMeshComponent* ArmMesh;

	UPROPERTY(EditDefaultsOnly, Blueprintable)
	USceneComponent* PitchRotator;
	
	UPROPERTY(EditDefaultsOnly)
	UStaticMeshComponent* CannonMesh;

	UPROPERTY(EditDefaultsOnly, Blueprintable)
	USceneComponent* CentreMuzzle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	USceneComponent* ProjectileSpawnPoint;
	
	// Variables
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<ATurretProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly)
	float TurnSpeed = 5.0f;

	// Store latest target info
	float LastTargetProjectileSpeed = 0.0f;
	FVector LastTargetProjectileLocation = FVector::ZeroVector;
	FVector LastTargetProjectileVelocity = FVector::ZeroVector;
	float LastTargetProjectileTime = 0.0f;

private:
	// Delay before firing once aim is computed.
	UPROPERTY(EditDefaultsOnly, Category = "Firing")
	float FiringDelay = 0.2f;

	// Timer handle used to schedule firing.
	FTimerHandle FiringTimerHandle;
};

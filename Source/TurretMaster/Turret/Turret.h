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

	// Fire the turret projectile
	UFUNCTION()
	void Fire();

	// Update turret yaw rotation.
	UFUNCTION()
	void SetYaw(float TargetYaw);

	// Update turret pitch rotation.
	UFUNCTION()
	void SetPitch(float TargetPitch);

	// Handle the aiming calculations using target position and velocity.
	UFUNCTION()
	void AimingMath(FVector TargetPos, FVector TargetVel);

	// Function bound to the launcherís delegate to receive projectile info.
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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	USceneComponent* PitchRotator;

	UPROPERTY(EditDefaultsOnly)
	UStaticMeshComponent* CannonMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	USceneComponent* CentreMuzzle;

	// Variables
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<ATurretProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly)
	float TurnSpeed = 500000.0f;

	// Store latest target info
	float LastTargetProjectileSpeed = 0.0f;
	FVector LastTargetProjectileLocation = FVector::ZeroVector;
	FVector LastTargetProjectileVelocity = FVector::ZeroVector;
	float LastTargetProjectileTime = 0.0f;

private:
	// Delay before firing once aim is computed.
	UPROPERTY(EditDefaultsOnly, Category = "Firing")
	float FiringDelay = 0.07f;

	UPROPERTY(EditDefaultsOnly, Category = "Firing")

	float TurretProjectileSpeed = 40000.0f;
	float LastInterceptTime = 0.0f;

	// Timer handle used to schedule firing.
	FTimerHandle FiringTimerHandle;
};
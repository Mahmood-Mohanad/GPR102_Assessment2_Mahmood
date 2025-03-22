// CG Spectrum 2025

// This file header
#include "TargetLauncher.h"

// Other includes
#include "TargetProjectile.h"
#include "Components/ArrowComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"


// Sets default values
ATargetLauncher::ATargetLauncher()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
	RootComponent = BaseMesh;

	ForwardArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("ForwardArrow"));
	ForwardArrow->SetupAttachment(BaseMesh);
}

// Called when the game starts or when spawned
void ATargetLauncher::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ATargetLauncher::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ATargetLauncher::LaunchTarget()
{
	
	UWorld* World = GetWorld();
	if (!World) return;

	// Randomize Yaw and Pitch within cone
	float RandomYaw = FMath::RandRange(-MaxSpreadAngle, MaxSpreadAngle);
	float RandomPitch = FMath::RandRange(-MaxSpreadAngle, MaxSpreadAngle);

	// Apply random spread to the rotation
	FRotator AdjustedRotation = ForwardArrow->GetComponentRotation();
	AdjustedRotation.Yaw += RandomYaw;
	AdjustedRotation.Pitch += RandomPitch;

	// Move the spawn location forward
	FVector AdjustedLocation = ForwardArrow->GetComponentLocation() + (ForwardArrow->GetForwardVector() * 30.0f);

	// Spawn
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	// Spawn the projectile
	AActor* SpawnedProjectile = World->SpawnActor<AActor>(ProjectileClass, AdjustedLocation, AdjustedRotation, SpawnParams);
	if (ATargetProjectile* Projectile = Cast<ATargetProjectile>(SpawnedProjectile))
	{
		float ProjectileSpeed = 0.0f;
		FVector ProjectileVelocity = FVector::ZeroVector;
		float ProjectileTime = GetWorld()->TimeSeconds; // Placeholder

		if (UProjectileMovementComponent* PM = Projectile->ProjectileMovement)
		{
			ProjectileSpeed = PM->InitialSpeed;
			ProjectileVelocity = PM->Velocity;
		}

		// Get the projectile's initial location
		FVector ProjectileLocation = SpawnedProjectile->GetActorLocation();

		// Broadcast the event, now passing the pointer as the fifth parameter.
		OnTargetProjectileLaunched.Broadcast(ProjectileSpeed, ProjectileLocation, ProjectileVelocity, ProjectileTime, Projectile);
	}
}


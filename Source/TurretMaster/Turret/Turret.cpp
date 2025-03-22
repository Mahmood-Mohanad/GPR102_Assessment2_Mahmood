// CG Spectrum 2025

#include "Turret.h"
#include "TargetProjectile.h"      // Contains our ATargetProjectile forward declaration and pointer
#include "GameFramework/ProjectileMovementComponent.h" // Provides full definition of UProjectileMovementComponent
#include "TurretProjectile.h"
#include "TargetLauncher.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"

// Sets default values
ATurret::ATurret()
{
	PrimaryActorTick.bCanEverTick = true;

	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
	RootComponent = BaseMesh;

	YawRotator = CreateDefaultSubobject<USceneComponent>(TEXT("RotationPoint"));
	YawRotator->SetupAttachment(RootComponent);

	ArmMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ArmMesh"));
	ArmMesh->SetupAttachment(YawRotator);

	PitchRotator = CreateDefaultSubobject<USceneComponent>(TEXT("RotationPointCannon"));
	PitchRotator->SetupAttachment(ArmMesh);
	PitchRotator->SetRelativeLocation(FVector(0.0f, 0.0f, -20.0f));

	CannonMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CannonMesh"));
	CannonMesh->SetupAttachment(PitchRotator);

	CentreMuzzle = CreateDefaultSubobject<USceneComponent>(TEXT("CentreMuzzle"));
	CentreMuzzle->SetupAttachment(CannonMesh);
}

// Called when the game starts or when spawned
void ATurret::BeginPlay()
{
	Super::BeginPlay();

	// Bind this turret to the launcher’s delegate.
	for (TActorIterator<ATargetLauncher> It(GetWorld()); It; ++It)
	{
		ATargetLauncher* Launcher = *It;
		if (Launcher)
		{
			Launcher->OnTargetProjectileLaunched.AddDynamic(this, &ATurret::OnTargetProjectileLaunched);
			break; // Bind to the first launcher found.
		}
	}
}

// Called every frame
void ATurret::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Update target data if the current target projectile is valid.
	if (CurrentTargetProjectile && CurrentTargetProjectile->IsValidLowLevel())
	{
		LastTargetProjectileLocation = CurrentTargetProjectile->GetActorLocation();
		if (CurrentTargetProjectile->ProjectileMovement)
		{
			LastTargetProjectileVelocity = CurrentTargetProjectile->ProjectileMovement->Velocity;
			LastTargetProjectileSpeed = CurrentTargetProjectile->ProjectileMovement->InitialSpeed;
		}
	}

	// Use valid target data to update aiming.
	if (LastTargetProjectileSpeed > 0.0f)
	{
		AimingMath(LastTargetProjectileLocation, LastTargetProjectileVelocity);
	}
}

void ATurret::Fire()
{
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    ATurretProjectile* TurretProj = GetWorld()->SpawnActor<ATurretProjectile>(
        ProjectileClass,
        CentreMuzzle->GetComponentLocation(),
        CentreMuzzle->GetComponentRotation(),
        SpawnParams
    );

    if (TurretProj)
    {
        TurretProj->SetLifeSpan(2.0f);

        if (UProjectileMovementComponent* PM = TurretProj->GetProjectileMovement())
        {
            FVector NewVelocity = CentreMuzzle->GetForwardVector() * TurretProjectileSpeed;
            PM->Velocity = NewVelocity;
            PM->Activate();
            UE_LOG(LogTemp, Warning, TEXT("Turret Projectile velocity set to: %s"), *NewVelocity.ToString());
        }
       
    }
  
}

void ATurret::SetYaw(float TargetYaw)
{
    FRotator CurrentRotation = YawRotator->GetComponentRotation();
    FRotator NewRotation(CurrentRotation.Pitch, TargetYaw, CurrentRotation.Roll);
    YawRotator->SetWorldRotation(NewRotation);
}

void ATurret::SetPitch(float TargetPitch)
{
    FRotator CurrentRelRotation = PitchRotator->GetRelativeRotation();
    FRotator NewRelRotation(TargetPitch, CurrentRelRotation.Yaw, CurrentRelRotation.Roll);
    PitchRotator->SetRelativeRotation(NewRelRotation);
}

void ATurret::OnTargetProjectileLaunched(float ProjectileSpeed, FVector ProjectileLocation, FVector ProjectileVelocity, float ProjectileTime, ATargetProjectile* TargetProjectilePtr)
{

	CurrentTargetProjectile = TargetProjectilePtr;
	LastTargetProjectileSpeed = ProjectileSpeed;
	LastTargetProjectileLocation = ProjectileLocation;
	LastTargetProjectileVelocity = ProjectileVelocity;
	LastTargetProjectileTime = ProjectileTime;
}

void ATurret::AimingMath(FVector TargetPos, FVector TargetVel)
{
    // Firing origin
    FVector S = CentreMuzzle->GetComponentLocation();

    // Compute relative target position
    FVector T_relative = TargetPos - S;

    // Solve for Δt:
    float A_val = TargetVel.SizeSquared() - TurretProjectileSpeed * TurretProjectileSpeed;
    float B = 2.0f * FVector::DotProduct(T_relative, TargetVel);
    float C = T_relative.SizeSquared();

    float delta_t = 0.0f;
    bool bValidTime = false;

    if (FMath::Abs(A_val) < KINDA_SMALL_NUMBER)
    {
        if (FMath::Abs(B) > KINDA_SMALL_NUMBER)
        {
            delta_t = -C / B;
            bValidTime = (delta_t > 0);
        }
    }

    else
    {
        float discriminant = B * B - 4.0f * A_val * C;
        if (discriminant >= 0)
        {
            float sqrtDisc = FMath::Sqrt(discriminant);
            float t1 = (-B + sqrtDisc) / (2.0f * A_val);
            float t2 = (-B - sqrtDisc) / (2.0f * A_val);
            if (t1 > 0 && t2 > 0)
                delta_t = FMath::Min(t1, t2);
            else if (t1 > 0)
                delta_t = t1;
            else if (t2 > 0)
                delta_t = t2;
            bValidTime = (delta_t > 0);
        }
    }
    LastInterceptTime = delta_t;

    // Gravity correction: projectile drop over time delta_t.
    float g = -GetWorld()->GetGravityZ();
    FVector GravityCorrection(0.0f, 0.0f, 0.5f * g * delta_t * delta_t);

    // Predict target position at intercept time (ignoring gravity on target).
    FVector PredictedTargetPos = TargetPos + TargetVel * delta_t;
    // Correct for the projectile's gravity drop.
    FVector CorrectedImpact = PredictedTargetPos + GravityCorrection;

    // Adding an offset so the turret fires in front of the impact point
    float OffsetDistance = 1000.0f;
    FVector Offset = TargetVel.GetSafeNormal() * OffsetDistance;
    FVector AdjustedImpact = CorrectedImpact + Offset;
    
    // Compute the firing direction from the turret muzzle to the adjusted impact point.
    FVector AimVector = AdjustedImpact - S;
    AimVector.Normalize();

    // Convert the aim direction to a rotation (((Chat GPT help me here))).
    FRotator DesiredRotation = FRotationMatrix::MakeFromX(AimVector).Rotator();

    // Update turret rotations.
    SetYaw(DesiredRotation.Yaw);
    SetPitch(DesiredRotation.Pitch);

    // Draw a debug line from the muzzle (firing origin) to the adjusted impact point.
    //DrawDebugLine(GetWorld(), S, AdjustedImpact, FColor::Blue, false, 0.1f, 0, 3.0f);

	if (!GetWorld()->GetTimerManager().IsTimerActive(FiringTimerHandle))
	{
		GetWorld()->GetTimerManager().SetTimer(FiringTimerHandle, this, &ATurret::Fire, FiringDelay, false);
	}

}

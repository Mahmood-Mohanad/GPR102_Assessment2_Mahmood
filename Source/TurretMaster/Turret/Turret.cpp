// CG Spectrum 2025

// This file's header

#include "Turret.h"
#include "TargetProjectile.h"      // Contains our ATargetProjectile forward declaration and pointer
#include "GameFramework/ProjectileMovementComponent.h" // Provides the full definition of UProjectileMovementComponent
#include "TurretProjectile.h"
#include "TargetLauncher.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"



// Sets default values
ATurret::ATurret()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>("BaseMesh");
	RootComponent = BaseMesh;
	
    YawRotator = CreateDefaultSubobject<USceneComponent>("RotationPoint");
    YawRotator->SetupAttachment(RootComponent);
	
    ArmMesh = CreateDefaultSubobject<UStaticMeshComponent>("ArmMesh");
    ArmMesh->SetupAttachment(YawRotator);

    PitchRotator = CreateDefaultSubobject<USceneComponent>("RotationPointCannon");
    PitchRotator->SetupAttachment(ArmMesh);
    PitchRotator->SetRelativeLocation(FVector(0.0f, 0.0f, -20.0f));

    CannonMesh = CreateDefaultSubobject<UStaticMeshComponent>("CannonMesh");
    CannonMesh->SetupAttachment(PitchRotator);

    CentreMuzzle = CreateDefaultSubobject<USceneComponent>("CentreMuzzle");
    CentreMuzzle->SetupAttachment(CannonMesh);

}

// Called when the game starts or when spawned
void ATurret::BeginPlay()
{
	Super::BeginPlay();

	// Step 6: Bind this turret to the launcher’s delegate.
	// For this example, we'll search the world for a TargetLauncher.
	for (TActorIterator<ATargetLauncher> It(GetWorld()); It; ++It)
	{
		ATargetLauncher* Launcher = *It;
		if (Launcher)
		{
			// Bind our OnTargetProjectileLaunched function to the launcher delegate.
			Launcher->OnTargetProjectileLaunched.AddDynamic(this, &ATurret::OnTargetProjectileLaunched);
			break; // Binding to the first found launcher for simplicity.
		}
	}
	
}

// Called every frame
void ATurret::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // If we have a valid target projectile pointer, update its current location and velocity.
    if (CurrentTargetProjectile && CurrentTargetProjectile->IsValidLowLevel())
    {
        // Update stored target data with current values.
        LastTargetProjectileLocation = CurrentTargetProjectile->GetActorLocation();
        if (CurrentTargetProjectile->ProjectileMovement)
        {
            LastTargetProjectileVelocity = CurrentTargetProjectile->ProjectileMovement->Velocity;
            LastTargetProjectileSpeed = CurrentTargetProjectile->ProjectileMovement->InitialSpeed; // or update as needed
        }
    }

    // If we have valid target data, update aiming continuously.
    if (LastTargetProjectileSpeed > 0.0f)
    {
        AimingMath(LastTargetProjectileSpeed, LastTargetProjectileLocation, LastTargetProjectileVelocity, LastTargetProjectileTime);
    }
}

void ATurret::Fire() const
{
    // Set up spawn parameters
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    // Spawn the turret projectile at the CentreMuzzle's location and rotation.
    ATurretProjectile* TurretProj = GetWorld()->SpawnActor<ATurretProjectile>(
        ProjectileClass,
        CentreMuzzle->GetComponentLocation(),
        CentreMuzzle->GetComponentRotation(),
        SpawnParams
    );

    if (TurretProj)
    {
        UE_LOG(LogTemp, Warning, TEXT("Turret Projectile spawned."));

        // Optionally set a lifespan so the projectile is cleaned up after some time.
        TurretProj->SetLifeSpan(5.0f);

        // Get the projectile's movement component (make sure it's accessible).
        UProjectileMovementComponent* PM = TurretProj->GetProjectileMovement();
        if (PM)
        {
            // Use the CentreMuzzle's forward vector (which should now be pointing toward the target 
            // because the turret's yaw and pitch have been updated by AimingMath)
            FVector NewVelocity = CentreMuzzle->GetForwardVector() * PM->InitialSpeed;
            PM->Velocity = NewVelocity;
            PM->Activate(); // Ensure the movement component is active.
            UE_LOG(LogTemp, Warning, TEXT("Turret Projectile velocity set to: %s"), *NewVelocity.ToString());
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("No ProjectileMovement component found on turret projectile."));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to spawn turret projectile."));
    }
}

void ATurret::SetYaw(float TargetYaw) const
{
    // Get current rotation of the base.
    FRotator CurrentRotation = YawRotator->GetComponentRotation();

    // Interpolate the yaw value.
    float NewYaw = FMath::FInterpTo(CurrentRotation.Yaw, TargetYaw, GetWorld()->GetDeltaSeconds(), TurnSpeed * 4);

    // Create a new rotation keeping pitch unchanged.
    FRotator NewRotation(CurrentRotation.Pitch, NewYaw, CurrentRotation.Roll);
    YawRotator->SetWorldRotation(NewRotation);
}

void ATurret::SetPitch(float TargetPitch) const
{
  // Get the current relative rotation of the PitchRotator.
  FRotator CurrentRelRotation = PitchRotator->GetRelativeRotation();

  // Interpolate to the target pitch.
  float NewPitch = FMath::FInterpTo(CurrentRelRotation.Pitch, TargetPitch, GetWorld()->GetDeltaSeconds(), TurnSpeed * 4);

  // Keep other components of rotation unchanged.
  FRotator NewRelRotation(NewPitch, CurrentRelRotation.Yaw, CurrentRelRotation.Roll);
  PitchRotator->SetRelativeRotation(NewRelRotation);
}

// Implement the function that will be called when the launcher broadcasts.
void ATurret::OnTargetProjectileLaunched(float ProjectileSpeed, FVector ProjectileLocation, FVector ProjectileVelocity, float ProjectileTime, ATargetProjectile* TargetProjectilePtr)
{
    UE_LOG(LogTemp, Warning, TEXT("Turret received target info: Speed: %.2f, Location: %s, Velocity: %s, Time: %.2f"),
        ProjectileSpeed,
        *ProjectileLocation.ToString(),
        *ProjectileVelocity.ToString(),
        ProjectileTime);

    // Store the pointer to the current target projectile.
    CurrentTargetProjectile = TargetProjectilePtr;

    // Also update the stored data.
    LastTargetProjectileSpeed = ProjectileSpeed;
    LastTargetProjectileLocation = ProjectileLocation;
    LastTargetProjectileVelocity = ProjectileVelocity;
    LastTargetProjectileTime = ProjectileTime;
}

void ATurret::AimingMath(float ProjectileSpeed, FVector ProjectileLocation, FVector ProjectileVelocity, float ProjectileTime)
{
    UE_LOG(LogTemp, Warning, TEXT("AimingMath (gravity) called with: Speed: %.2f, Location: %s, Velocity: %s, Time: %.2f"),
        ProjectileSpeed,
        *ProjectileLocation.ToString(),
        *ProjectileVelocity.ToString(),
        ProjectileTime);

    // Define the firing origin.
    FVector S = CentreMuzzle->GetComponentLocation();

    // Let R be the initial relative position vector from S to the target's spawn position.
    FVector R = ProjectileLocation - S;

    // Get gravity vector.
    FVector g = FVector(0.0f, 0.0f, GetWorld()->GetGravityZ());

    // B is the turret projectile's speed (I assume it's the same as the target's reported speed).
    float B = ProjectileSpeed;

    // We need to solve for time t such that:
    //   f(t) = |R + V*t + 0.5*g*t^2| - B*t = 0
    // Define f(t) below.
    auto f = [&](float t) -> float {
        FVector Q = R + ProjectileVelocity * t + 0.5f * g * t * t;
        return Q.Size() - B * t;
        };  // Chat GPT help me with this one

    // f(t) = F(t) - B*t, where F(t) = |Q(t)|
    // f'(t) = (Q(t) dot Q'(t)) / |Q(t)| - B, with Q'(t) = d/dt [R + V*t + 0.5*g*t^2] = V + g*t.
    auto fPrime = [&](float t) -> float {
        FVector Q = R + ProjectileVelocity * t + 0.5f * g * t * t;
        float QSize = Q.Size(); // Chat GPT help me with this one

        // Avoid division by zero.
        if (FMath::IsNearlyZero(QSize))
        {
            return -B;
        }
        FVector QPrime = ProjectileVelocity + g * t;
        return FVector::DotProduct(Q, QPrime) / QSize - B;
        };

    // t_initial = |R| / B.
    float t = (R.Size() / B);
    const int MaxIterations = 20;
    const float Tolerance = 0.01f; //will help us later
    bool bConverged = false;

    for (int i = 0; i < MaxIterations; ++i)
    {
        float ft = f(t);
        float fpt = fPrime(t);

        if (FMath::Abs(ft) < Tolerance)
        {
            bConverged = true;
            break;
        }

        // Prevent division by zero.
        if (FMath::IsNearlyZero(fpt))
        {
            break;
        }

        t = t - ft / fpt;
        // Ensure t stays positive.
        if (t < 0.0f)
        {
            t = Tolerance;
        }
    }

    if (!bConverged)
    {
        UE_LOG(LogTemp, Warning, TEXT("Newton's method did not converge. Using last t = %.2f"), t);
    }

    float impactTime = t + FiringDelay;
    // Now compute the predicted impact point, which is the target's position under gravity at time t.
    FVector ImpactPoint = ProjectileLocation + ProjectileVelocity * impactTime + 0.5f * g * impactTime * impactTime;

    // The required aim direction is from S to ImpactPoint, its like solving fo Unit vector.
    FVector AimVector = ImpactPoint - S;
    AimVector.Normalize();

    // Convert the aim vector into a rotation Chat GPT help me here.
    FRotator DesiredRotation = FRotationMatrix::MakeFromX(AimVector).Rotator();
    float desiredYaw = DesiredRotation.Yaw;
    float desiredPitch = DesiredRotation.Pitch;


    // Update turret rotations.
    SetYaw(desiredYaw);
    SetPitch(desiredPitch);

    // debug line for viz.
    DrawDebugLine(
        GetWorld(),
        YawRotator->GetComponentLocation(),   // Start point
        ImpactPoint,                          // End point
        FColor::Red,                          // Color
        false,                                // Not persistent
        1.5f,                                 // Lifetime
        5,                                    // Depth Priority
        2.0f                                  // Thickness
    );

    // Schedule firing after a delay.
    if (!GetWorld()->GetTimerManager().IsTimerActive(FiringTimerHandle))
    {
        GetWorld()->GetTimerManager().SetTimer(FiringTimerHandle, this, &ATurret::Fire, FiringDelay, false);
    }
}


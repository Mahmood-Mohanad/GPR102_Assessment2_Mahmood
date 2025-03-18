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
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    AActor* SpawnedProjectile = GetWorld()->SpawnActor<AActor>(ProjectileClass, CentreMuzzle->GetComponentLocation(), CentreMuzzle->GetComponentRotation(), SpawnParams);
    if (SpawnedProjectile)
    {
        UE_LOG(LogTemp, Warning, TEXT("Turret Projectile spawned."));
    }
}

void ATurret::SetYaw(float TargetYaw) const
{
    // Get current rotation of the base.
    FRotator CurrentRotation = YawRotator->GetComponentRotation();

    // Interpolate the yaw value.
    float NewYaw = FMath::FInterpTo(CurrentRotation.Yaw, TargetYaw, GetWorld()->GetDeltaSeconds(), TurnSpeed);

    // Create a new rotation keeping pitch unchanged.
    FRotator NewRotation(CurrentRotation.Pitch, NewYaw, CurrentRotation.Roll);
    YawRotator->SetWorldRotation(NewRotation);
}

void ATurret::SetPitch(float TargetPitch) const
{
  // Get the current relative rotation of the PitchRotator.
  FRotator CurrentRelRotation = PitchRotator->GetRelativeRotation();

  // Interpolate to the target pitch.
  float NewPitch = FMath::FInterpTo(CurrentRelRotation.Pitch, TargetPitch, GetWorld()->GetDeltaSeconds(), TurnSpeed);

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
    // Log the received parameters for debugging.
    UE_LOG(LogTemp, Warning, TEXT("AimingMath called with: Speed: %.2f, Location: %s, Velocity: %s, Time: %.2f"),
        ProjectileSpeed,
        *ProjectileLocation.ToString(),
        *ProjectileVelocity.ToString(),
        ProjectileTime);

    // Get the turret's current location (S).
    //FVector S = GetActorLocation(); //Doesn't work as intended
    FVector S = YawRotator->GetComponentLocation();

    // Compute the relative target position (R = T - S), where T is the projectile's initial location.
    FVector R = ProjectileLocation - S;

    // For the formulas, we use:
    // S: turret location
    // T: Projectile Location
    // V: Projectile Velocity
    // B: Projectile Speed
    // Δt: time to impact

    //  Set up the quadratic equation:
    //    |R + V * Δt| = B * Δt
    // Squaring both sides gives:
    //    (R + V*Δt) • (R + V*Δt) = (B * Δt)²
    // Expanding results in:
    //    |R|² + 2*(R • V)*Δt + |V|²*(Δt)² = B²*(Δt)²
    // Rearranging, we get a quadratic of the form:
    //    a * (Δt)² + b * Δt + c = 0, where:

    float B = ProjectileSpeed;
    float a = ProjectileVelocity.SizeSquared() - (B * B);  // a = |V|² - B²
    float b = 2.0f * FVector::DotProduct(R, ProjectileVelocity); // b = 2 * (R • V)
    float c = R.SizeSquared();  // c = |R|²

    // Solve the quadratic equation for Δt.
    float discriminant = b * b - 4.0f * a * c;
    if (discriminant < 0.0f)
    {
        UE_LOG(LogTemp, Warning, TEXT("No solution for impact time (discriminant < 0)"));
        return;
    }

    float sqrtDiscriminant = FMath::Sqrt(discriminant);
    // Two possible solutions (we should choose one later):
    float t1 = (-b + sqrtDiscriminant) / (2.0f * a);
    float t2 = (-b - sqrtDiscriminant) / (2.0f * a);
    // Choose the smallest positive Δt.
    float impactTime = -1.0f;

    if (t1 > 0.0f && t2 > 0.0f)
    {
        impactTime = FMath::Min(t1, t2);
    }

    else if (t1 > 0.0f)
    {
        impactTime = t1;
    }

    else if (t2 > 0.0f)
    {
        impactTime = t2;
    }

    else
    {
        UE_LOG(LogTemp, Warning, TEXT("No positive time solution for impact."));
        return;
    }

    // Predict the impact point:
    // The target will be at: T + V * Δt
    FVector ImpactPoint = ProjectileLocation + ProjectileVelocity * impactTime;

    // Determine the aiming direction vector from the turret to the impact point.
    FVector AimVector = ImpactPoint - S;
    AimVector.Normalize();

    // Convert the aim vector to a rotation using UE build in function.
    FRotator DesiredRotation = FRotationMatrix::MakeFromX(AimVector).Rotator(); //I used (Chat GPT) for this

    // Extract yaw and pitch from the rotation.
    float desiredYaw = DesiredRotation.Yaw;
    float desiredPitch = DesiredRotation.Pitch;

    // Log the computed values.
    UE_LOG(LogTemp, Warning, TEXT("ImpactTime: %.2f, ImpactPoint: %s, AimVector: %s"), //I used (Chat GPT) for this
    impactTime, *ImpactPoint.ToString(), *AimVector.ToString());

    // Adjust the turret's rotation.
    SetYaw(desiredYaw);
    SetPitch(desiredPitch);
    //Fire();

    // Draw a debug line from the turret's pivot (RotationPoint) to the predicted ImpactPoint.
    DrawDebugLine(
        GetWorld(),
        YawRotator->GetComponentLocation(),   // start point
        ImpactPoint,                              // end point
        FColor::Red,                              // line color
        false,                                    // do not persist indefinitely
        1.5f,                                     // duration (in seconds)
        0,                                        // depth priority
        5.0f                                      // thickness
    );

    // Now schedule firing after a delay.
    if (!GetWorld()->GetTimerManager().IsTimerActive(FiringTimerHandle))
    {
        GetWorld()->GetTimerManager().SetTimer(FiringTimerHandle, this, &ATurret::Fire, FiringDelay, false);
    }
}


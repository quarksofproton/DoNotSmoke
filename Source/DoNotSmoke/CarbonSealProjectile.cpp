// Copyright Do Not Smoke Team. All Rights Reserved.

#include "CarbonSealProjectile.h"
#include "GameplayInterfaces.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "UObject/ConstructorHelpers.h"

ACarbonSealProjectile::ACarbonSealProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->InitSphereRadius(36.0f);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Overlap);
	CollisionSphere->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CollisionSphere->SetGenerateOverlapEvents(true);
	SetRootComponent(CollisionSphere);

	CapsuleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CapsuleMesh"));
	CapsuleMesh->SetupAttachment(CollisionSphere);
	CapsuleMesh->SetRelativeScale3D(FVector(0.25f, 0.25f, 0.25f));
	CapsuleMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SealCapMeshAsset(TEXT("/Game/Game/Meshes/Rev3/SM_Seal_Single/StaticMeshes/SM_Seal_Single.SM_Seal_Single"));
	if (SealCapMeshAsset.Succeeded())
	{
		CapsuleMesh->SetStaticMesh(SealCapMeshAsset.Object);
	}

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = CollisionSphere;
	ProjectileMovement->InitialSpeed = LaunchSpeed;
	ProjectileMovement->MaxSpeed = LaunchSpeed;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->ProjectileGravityScale = ProjectileGravityScale;
	ProjectileMovement->bInitialVelocityInLocalSpace = false;
}

void ACarbonSealProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (CapsuleMesh && (!CapsuleMesh->GetStaticMesh() || !CapsuleMesh->GetStaticMesh()->GetPathName().Contains(TEXT("Rev3"))))
	{
		if (UStaticMesh* Rev3Seal = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Game/Meshes/Rev3/SM_Seal_Single/StaticMeshes/SM_Seal_Single.SM_Seal_Single")))
		{
			CapsuleMesh->SetStaticMesh(Rev3Seal);
		}
	}

	SetLifeSpan(MaxLifeSeconds);

	if (CollisionSphere)
	{
		CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &ACarbonSealProjectile::HandleCollisionOverlap);
		CollisionSphere->OnComponentHit.AddDynamic(this, &ACarbonSealProjectile::HandleCollisionHit);
	}
}

void ACarbonSealProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Subtle ~10 deg magnetic homing steering toward the exact locked smokestack rim mouth
	AActor* TargetActor = LockedHomingTarget.Get();
	if (TargetActor && TargetActor->Implements<UBPI_SealableTarget>() && ProjectileMovement)
	{
		if (IBPI_SealableTarget::Execute_IsTargetSealed(TargetActor))
		{
			LockedHomingTarget.Reset();
			return;
		}

		FVector TargetRimLoc = LockedHomingRimLocation;
		if (TargetRimLoc.IsNearlyZero())
		{
			TargetRimLoc = IBPI_SealableTarget::Execute_GetTargetRimLocation(TargetActor);
		}
		else
		{
			// Verify the specific locked mouth on a Clustered stack hasn't already been sealed
			const TArray<FVector> UnsealedRims = IBPI_SealableTarget::Execute_GetUnsealedRimLocations(TargetActor);
			bool bStillUnsealed = false;
			for (const FVector& OpenRim : UnsealedRims)
			{
				if (FVector::DistSquared(OpenRim, TargetRimLoc) < FMath::Square(50.0f))
				{
					bStillUnsealed = true;
					break;
				}
			}
			if (!bStillUnsealed && UnsealedRims.Num() > 0)
			{
				LockedHomingTarget.Reset();
				return;
			}
		}

		const FVector DesiredDir = (TargetRimLoc - GetActorLocation()).GetSafeNormal();
		const FVector CurrentDir = ProjectileMovement->Velocity.GetSafeNormal();

		if (!DesiredDir.IsNearlyZero() && !CurrentDir.IsNearlyZero())
		{
			const FVector SteeredDir = FMath::VInterpNormalRotationTo(CurrentDir, DesiredDir, DeltaTime, MagneticSteerInterpSpeed * 15.0f);
			ProjectileMovement->Velocity = SteeredDir * ProjectileMovement->Velocity.Size();
		}
	}
}

void ACarbonSealProjectile::InitializeLaunch(const FVector& LaunchDirection, AActor* HomingTargetActor, FVector HomingRimLocation)
{
	LockedHomingTarget = HomingTargetActor;
	LockedHomingRimLocation = HomingRimLocation;

	const FVector SafeDir = LaunchDirection.GetSafeNormal();
	if (ProjectileMovement && !SafeDir.IsNearlyZero())
	{
		ProjectileMovement->Velocity = SafeDir * LaunchSpeed;
		ProjectileMovement->UpdateComponentVelocity();
	}
}

void ACarbonSealProjectile::HandleCollisionOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!OtherActor || OtherActor == this || OtherActor == GetInstigator())
	{
		return;
	}

	if (OtherActor->Implements<UBPI_SealableTarget>())
	{
		// ASSUMPTION: Only seal when overlapping the non-lethal rim target sphere, not the chimney body or plume.
		const bool bIsHazardComp = OtherActor->Implements<UBPI_FlightHazard>() &&
			IBPI_FlightHazard::Execute_IsLethalCrashHazard(OtherActor, OtherComp);

		if (!bIsHazardComp)
		{
			// Pass 'this' projectile as SealInstigator so Clustered smokestacks seal the exact mouth that was hit
			const bool bDidSeal = IBPI_SealableTarget::Execute_TrySealTarget(OtherActor, this);
			if (bDidSeal)
			{
				BP_OnSealImpact(GetActorLocation(), true);
				Destroy();
			}
		}
	}
}

void ACarbonSealProjectile::HandleCollisionHit(
	UPrimitiveComponent* HitComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	FVector NormalImpulse,
	const FHitResult& Hit)
{
	if (!OtherActor || OtherActor == GetInstigator())
	{
		return;
	}

	BP_OnSealImpact(Hit.ImpactPoint, false);
	Destroy();
}

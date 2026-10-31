// Copyright Do Not Smoke Team. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CarbonSealProjectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;

/**
 * Eco-capsule projectile launched by the Solar Glider at 5000 u/s.
 * Supports subtle magnetic homing (~10 deg cone) toward open smokestack rims.
 */
UCLASS(Blueprintable)
class DONOTSMOKE_API ACarbonSealProjectile : public AActor
{
	GENERATED_BODY()

public:
	ACarbonSealProjectile();

	virtual void Tick(float DeltaTime) override;

	/**
	 * Initializes launch velocity and optional magnetic homing target acquired within the ~10 deg assist cone.
	 * @param LaunchDirection Normalized world launch direction (already clamped to 45 deg glider forward cone).
	 * @param HomingTargetActor Optional actor implementing IBPI_SealableTarget.
	 * @param HomingRimLocation Exact world-space location of the locked chimney mouth.
	 */
	UFUNCTION(BlueprintCallable, Category = "Projectile|Launch")
	void InitializeLaunch(const FVector& LaunchDirection, AActor* HomingTargetActor, FVector HomingRimLocation = FVector::ZeroVector);

protected:
	virtual void BeginPlay() override;

	/** Blueprint cosmetic hook when the projectile successfully seals a smokestack rim. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Projectile|Cosmetics", meta = (DisplayName = "On Seal Impact"))
	void BP_OnSealImpact(const FVector& ImpactLocation, bool bDidSealTarget);

private:
	// ==========================================
	// COMPONENTS
	// ==========================================
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> CollisionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> CapsuleMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	// ==========================================
	// DESIGNER TUNING / CONFIG
	// ==========================================
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Config", meta = (AllowPrivateAccess = "true"))
	float LaunchSpeed = 5000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Config", meta = (AllowPrivateAccess = "true"))
	float ProjectileGravityScale = 0.25f;

	/** Angular homing interpolation speed (deg/sec) steering the projectile gently toward the locked rim. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Config", meta = (AllowPrivateAccess = "true"))
	float MagneticSteerInterpSpeed = 6.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Config", meta = (AllowPrivateAccess = "true"))
	float MaxLifeSeconds = 4.0f;

	// ==========================================
	// INTERNAL STATE
	// ==========================================
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> LockedHomingTarget;

	FVector LockedHomingRimLocation = FVector::ZeroVector;

	UFUNCTION()
	void HandleCollisionOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void HandleCollisionHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& Hit);
};

// Copyright Do Not Smoke Team. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayInterfaces.generated.h"

class UPrimitiveComponent;

/** What killed the glider; drives the crash screen icon. */
UENUM(BlueprintType)
enum class EGliderCrashCause : uint8
{
	Ground		UMETA(DisplayName = "Ground Impact"),
	Smokestack	UMETA(DisplayName = "Smokestack Collision"),
	ToxicPlume	UMETA(DisplayName = "Toxic Plume")
};

/**
 * Contract for pollution sources (e.g., industrial smokestacks) that can be
 * targeted by magnetic aim-assist and sealed by a Carbon Seal projectile.
 */
UINTERFACE(MinimalAPI, BlueprintType)
class UBPI_SealableTarget : public UInterface
{
	GENERATED_BODY()
};

class DONOTSMOKE_API IBPI_SealableTarget
{
	GENERATED_BODY()

public:
	/** Attempt to seal this pollution source. Returns true if state transitioned from Open to Sealed. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "DoNotSmoke|Target")
	bool TrySealTarget(AActor* SealInstigator);

	/** Returns true if this target has already been sealed. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "DoNotSmoke|Target")
	bool IsTargetSealed() const;

	/** Returns the world-space center of the primary open chimney rim. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "DoNotSmoke|Target")
	FVector GetTargetRimLocation() const;

	/** Returns the world-space centers of all currently unsealed chimney rims on this actor (1 for Single/Heavy, 2-3 for Clustered). */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "DoNotSmoke|Target")
	TArray<FVector> GetUnsealedRimLocations() const;
};

/**
 * Contract for aerial & structural hazards (chimney bodies, Type-C lethal smoke pillars,
 * and VIP Private Jet wake turbulence volumes).
 */
UINTERFACE(MinimalAPI, BlueprintType)
class UBPI_FlightHazard : public UInterface
{
	GENERATED_BODY()
};

class DONOTSMOKE_API IBPI_FlightHazard
{
	GENERATED_BODY()

public:
	/** Returns true if colliding with or overlapping the given component triggers an immediate Crash. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "DoNotSmoke|Hazard")
	bool IsLethalCrashHazard(const UPrimitiveComponent* HazardComponent) const;

	/** Returns downward wake turbulence speed (units/sec) if overlapping a non-lethal wake volume, or 0.f. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "DoNotSmoke|Hazard")
	float GetWakeDownwashSpeed(const UPrimitiveComponent* HazardComponent) const;
};

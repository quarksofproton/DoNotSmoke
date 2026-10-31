// Copyright Do Not Smoke Team. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GameplayInterfaces.h"
#include "GliderPawn.generated.h"

class USphereComponent;
class USkeletalMeshComponent;
class UStaticMeshComponent;
class USpringArmComponent;
class UCameraComponent;
class UNiagaraComponent;
class ACarbonSealProjectile;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPullUpWarningChangedSignature, bool, bIsWarningActive);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGliderCrashedSignature, AActor*, CrashedGlider);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCarbonSealFiredSignature);

/**
 * Kinematic Solar Glider Pawn featuring:
 * - Strict net-loss gliding (15:1 glide ratio, MaxAllowedGlideZ ceiling cap unless Solar Boosting)
 * - Decoupled SpringArm camera look & 135-degree cone clamped projectile aiming with ~10-degree magnetic assist
 * - Periodic 0.2s vertical sunlight LineTrace (binary 12.5%/s charge vs 0%/s under smog)
 * - Altitude thresholds: 200u Kill Floor, 400u Smog Bed, 600u "PULL UP!" warning, 2600u Max Ceiling
 */
UCLASS(Blueprintable)
class DONOTSMOKE_API AGliderPawn : public APawn
{
	GENERATED_BODY()

public:
	AGliderPawn();

	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// ==========================================
	// DISPATCHERS (DIP - Observer Pattern)
	// ==========================================
	UPROPERTY(BlueprintAssignable, Category = "Glider|Events")
	FOnPullUpWarningChangedSignature OnPullUpWarningChanged;

	UPROPERTY(BlueprintAssignable, Category = "Glider|Events")
	FOnGliderCrashedSignature OnGliderCrashed;

	UPROPERTY(BlueprintAssignable, Category = "Glider|Events")
	FOnCarbonSealFiredSignature OnCarbonSealFired;

	// ==========================================
	// PUBLIC API / HUD TELEMETRY GETTERS
	// ==========================================
	UFUNCTION(BlueprintPure, Category = "Glider|Telemetry")
	float GetCurrentSpeed() const { return CurrentSpeed; }

	UFUNCTION(BlueprintPure, Category = "Glider|Telemetry")
	float GetCurrentAltitude() const { return GetActorLocation().Z; }

	UFUNCTION(BlueprintPure, Category = "Glider|Telemetry")
	float GetSolarChargePercent() const { return SolarCharge; }

	UFUNCTION(BlueprintPure, Category = "Glider|Telemetry")
	float GetSealCooldownRemaining() const { return SealCooldownRemaining; }

	/** Returns 0.0 when ready to fire, up to 1.0 immediately after firing (for radial HUD progress). */
	UFUNCTION(BlueprintPure, Category = "Glider|Telemetry")
	float GetSealCooldownNormalized() const;

	UFUNCTION(BlueprintPure, Category = "Glider|Telemetry")
	bool IsInSunlight() const { return bIsInSunlight; }

	UFUNCTION(BlueprintPure, Category = "Glider|Telemetry")
	bool IsBoosting() const { return bIsSolarBoosting; }

	UFUNCTION(BlueprintPure, Category = "Glider|Telemetry")
	bool IsPullUpWarningActive() const { return bIsPullUpWarningActive; }

	/** Valid after OnGliderCrashed has fired. */
	UFUNCTION(BlueprintPure, Category = "Glider|Telemetry")
	EGliderCrashCause GetLastCrashCause() const { return LastCrashCause; }

	/** Applies external downward wake turbulence impulse (e.g., from passing VIP Private Jet). */
	UFUNCTION(BlueprintCallable, Category = "Glider|Hazards")
	void ApplyWakeTurbulence(float DownwashSpeed, float DurationSeconds);

	UFUNCTION(BlueprintCallable, Category = "Glider|Debug")
	void SetShowDebugHUD(bool bShow) { bShowDebugHUD = bShow; }

	UFUNCTION(BlueprintPure, Category = "Glider|Debug")
	bool GetShowDebugHUD() const { return bShowDebugHUD; }

	/** Muzzle / projectile launch origin offset from glider nose. */
	UFUNCTION(BlueprintPure, Category = "Glider|Combat")
	FVector GetProjectileSpawnLocation() const { return GetActorLocation() + GetActorForwardVector() * 140.0f - FVector(0.0f, 0.0f, 30.0f); }

	UFUNCTION(BlueprintPure, Category = "Glider|Combat")
	FVector GetAimWorldTargetLocation() const { return CachedAimWorldTarget; }

	UFUNCTION(BlueprintPure, Category = "Glider|Combat")
	bool IsAimConeClamped() const { return bIsAimClampedToCone; }

	UFUNCTION(BlueprintPure, Category = "Glider|Combat")
	bool HasMagneticLock() const { return bHasMagneticRimLock; }

	// ==========================================
	// INPUT ACTIONS (Zero-Setup C++ & Blueprint Callable)
	// ==========================================
	UFUNCTION(BlueprintCallable, Category = "Glider|Input")
	void InputPitch(float AxisValue);

	UFUNCTION(BlueprintCallable, Category = "Glider|Input")
	void InputRoll(float AxisValue);

	UFUNCTION(BlueprintCallable, Category = "Glider|Input")
	void InputLookYaw(float AxisValue);

	UFUNCTION(BlueprintCallable, Category = "Glider|Input")
	void InputLookPitch(float AxisValue);

	UFUNCTION(BlueprintCallable, Category = "Glider|Input")
	void StartSolarBoost();

	UFUNCTION(BlueprintCallable, Category = "Glider|Input")
	void StopSolarBoost();

	UFUNCTION(BlueprintCallable, Category = "Glider|Input")
	void FireCarbonSeal();

protected:
	virtual void BeginPlay() override;

	/** Blueprint cosmetic hook when Solar Boost starts or stops. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Glider|Cosmetics", meta = (DisplayName = "On Solar Boost State Changed"))
	void BP_OnSolarBoostStateChanged(bool bBoostActive);

	/** Blueprint cosmetic hook when Carbon Seal is fired (for 0.1s camera kick & SFX_Seal_Fire). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Glider|Cosmetics", meta = (DisplayName = "On Carbon Seal Fired"))
	void BP_OnCarbonSealFired(const FVector& LaunchDirection);

	/** Blueprint cosmetic hook when wake turbulence is experienced. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Glider|Cosmetics", meta = (DisplayName = "On Wake Turbulence Hit"))
	void BP_OnWakeTurbulenceHit();

private:
	// ==========================================
	// COMPONENTS
	// ==========================================
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Glider|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> GliderCollisionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Glider|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> GliderMesh;

	/** Fallback greybox glider visual automatically hidden once a SkeletalMesh is assigned to GliderMesh. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Glider|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> GreyboxGliderMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Glider|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Glider|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Glider|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNiagaraComponent> SolarBoostVFX;

	// ==========================================
	// ATOMIC FLIGHT & ENERGY PARAMETERS (GDD Sec 3)
	// ==========================================
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Glider|Config|Flight", meta = (AllowPrivateAccess = "true"))
	float BaseVelocity = 1800.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Glider|Config|Flight", meta = (AllowPrivateAccess = "true"))
	float MaxDiveVelocity = 3800.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Glider|Config|Flight", meta = (AllowPrivateAccess = "true"))
	float StallVelocity = 900.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Glider|Config|Flight", meta = (AllowPrivateAccess = "true"))
	float PitchRateDegPerSec = 105.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Glider|Config|Flight", meta = (AllowPrivateAccess = "true"))
	float RollYawRateDegPerSec = 125.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Glider|Config|Flight", meta = (AllowPrivateAccess = "true"))
	float GlideRatio = 25.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Glider|Config|Energy", meta = (AllowPrivateAccess = "true"))
	float SolarChargeRatePerSec = 12.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Glider|Config|Energy", meta = (AllowPrivateAccess = "true"))
	float SolarBoostDrainRatePerSec = 25.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Glider|Config|Energy", meta = (AllowPrivateAccess = "true"))
	float SolarBoostThrust = 2200.0f;

	// ==========================================
	// ALTITUDE STACK THRESHOLDS (GDD Sec 3.3)
	// ==========================================
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Glider|Config|Altitude", meta = (AllowPrivateAccess = "true"))
	float KillFloorAltitude = 200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Glider|Config|Altitude", meta = (AllowPrivateAccess = "true"))
	float SmogBedCeilingAltitude = 400.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Glider|Config|Altitude", meta = (AllowPrivateAccess = "true"))
	float PullUpWarningAltitude = 600.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Glider|Config|Altitude", meta = (AllowPrivateAccess = "true"))
	float GliderMaxCeilingAltitude = 3900.0f;

	/** Chimney rim altitude (1350u) to which Private Jet wake turbulence rapidly pushes the glider down. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Glider|Config|Altitude", meta = (AllowPrivateAccess = "true"))
	float WakeTargetFloorAltitude = 1350.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Glider|Config|Altitude", meta = (AllowPrivateAccess = "true"))
	float SunlightTraceInterval = 0.2f;

	// ==========================================
	// CAMERA & TARGETING PARAMETERS (GDD Sec 2.2 & 3.2)
	// ==========================================
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Glider|Config|Camera", meta = (AllowPrivateAccess = "true"))
	float BaseFOV = 90.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Glider|Config|Camera", meta = (AllowPrivateAccess = "true"))
	float BoostOrDiveFOV = 96.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Glider|Config|Camera", meta = (AllowPrivateAccess = "true"))
	float MouseSensitivity = 1.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Glider|Config|Camera", meta = (AllowPrivateAccess = "true"))
	float CameraRotationLagSpeed = 5.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Glider|Config|Camera", meta = (AllowPrivateAccess = "true"))
	float CameraArmLength = 880.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Glider|Config|Camera", meta = (AllowPrivateAccess = "true"))
	float CameraSocketHeightOffset = 175.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Glider|Config|Combat", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<ACarbonSealProjectile> CarbonSealProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Glider|Config|Combat", meta = (AllowPrivateAccess = "true"))
	float SealCooldownDuration = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Glider|Config|Combat", meta = (AllowPrivateAccess = "true"))
	float MaxAimConeHalfAngleDeg = 135.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Glider|Config|Combat", meta = (AllowPrivateAccess = "true"))
	float AimAssistConeHalfAngleDeg = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Glider|Config|Combat", meta = (AllowPrivateAccess = "true"))
	float AimAssistMaxRange = 20000.0f;

	/** Displays live C++ debug telemetry and 135-degree aim indicator on screen before UMG HUD is wired. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Glider|Debug", meta = (AllowPrivateAccess = "true"))
	bool bShowDebugHUD = true;

	// ==========================================
	// INTERNAL RUNTIME STATE
	// ==========================================
	float CurrentSpeed = 1800.0f;
	float SolarCharge = 100.0f;
	float MaxAllowedGlideZ = 1800.0f;
	float SealCooldownRemaining = 0.0f;

	float PitchInput = 0.0f;
	float RollInput = 0.0f;
	float SmoothedPitchInput = 0.0f;
	float SmoothedRollInput = 0.0f;
	float CameraYawOffset = 0.0f;
	float CameraPitchOffset = -10.0f;
	FRotator SmoothedCameraWorldRot = FRotator::ZeroRotator;

	float ActiveDownwashSpeed = 0.0f;
	float DownwashTimeRemaining = 0.0f;

	mutable FVector CachedAimWorldTarget = FVector::ZeroVector;
	mutable bool bIsAimClampedToCone = false;
	mutable bool bHasMagneticRimLock = false;

	bool bIsSolarBoostInputHeld = false;
	bool bIsSolarBoosting = false;
	bool bIsInSunlight = true;
	bool bIsPullUpWarningActive = false;
	bool bHasCrashed = false;
	EGliderCrashCause LastCrashCause = EGliderCrashCause::Ground;

	bool bKeyPitchUpHeld = false;
	bool bKeyPitchDownHeld = false;
	bool bKeyRollLeftHeld = false;
	bool bKeyRollRightHeld = false;

	FTimerHandle SunlightTraceTimerHandle;

	void InputPitchUpPressed() { bKeyPitchUpHeld = true; RefreshKeyAxisInputs(); }
	void InputPitchUpReleased() { bKeyPitchUpHeld = false; RefreshKeyAxisInputs(); }
	void InputPitchDownPressed() { bKeyPitchDownHeld = true; RefreshKeyAxisInputs(); }
	void InputPitchDownReleased() { bKeyPitchDownHeld = false; RefreshKeyAxisInputs(); }
	void InputRollLeftPressed() { bKeyRollLeftHeld = true; RefreshKeyAxisInputs(); }
	void InputRollLeftReleased() { bKeyRollLeftHeld = false; RefreshKeyAxisInputs(); }
	void InputRollRightPressed() { bKeyRollRightHeld = true; RefreshKeyAxisInputs(); }
	void InputRollRightReleased() { bKeyRollRightHeld = false; RefreshKeyAxisInputs(); }
	void RefreshKeyAxisInputs();

	void UpdateFlightKinematics(float DeltaTime);
	void UpdateSolarEnergyAndAltitudeChecks(float DeltaTime);
	void UpdateDecoupledCamera(float DeltaTime);
	void PerformSunlightTrace();
	void TriggerCrash(EGliderCrashCause Cause);

	FVector ComputeClampedAimDirection(AActor*& OutMagneticTarget, FVector& OutMagneticRimLocation) const;

	UFUNCTION()
	void HandleGliderOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void HandleGliderHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& Hit);
};

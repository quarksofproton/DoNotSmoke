// Copyright Do Not Smoke Team. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayInterfaces.h"
#include "Smokestack.generated.h"

class UStaticMeshComponent;
class USphereComponent;
class UCapsuleComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class UAudioComponent;
class USoundBase;
class ASmokestack;

UENUM(BlueprintType)
enum class ESmokestackType : uint8
{
	Normal      UMETA(DisplayName = "Type A - Normal Single"),
	Clustered   UMETA(DisplayName = "Type B - Clustered Power Plant"),
	HeavyLethal UMETA(DisplayName = "Type C - Heavy Lethal Plume")
};

/** Fired once per chimney mouth sealed (a Clustered stack broadcasts 2-3 times; check IsSealed() for full seal). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSmokestackSealedSignature, ASmokestack*, SealedSmokestack);

/**
 * Unified industrial smokestack actor supporting Type A (Normal), Type B (Clustered),
 * and Type C (Heavy Lethal Plume) archetypes.
 */
UCLASS(Blueprintable)
class DONOTSMOKE_API ASmokestack : public AActor, public IBPI_SealableTarget, public IBPI_FlightHazard
{
	GENERATED_BODY()

public:
	ASmokestack();

	// ==========================================
	// DISPATCHERS (DIP - Observer Pattern)
	// ==========================================
	UPROPERTY(BlueprintAssignable, Category = "Smokestack|Events")
	FOnSmokestackSealedSignature OnSmokestackSealed;

	// ==========================================
	// PUBLIC API / STATE QUERIES
	// ==========================================
	UFUNCTION(BlueprintPure, Category = "Smokestack|State")
	ESmokestackType GetSmokestackType() const { return SmokestackType; }

	UFUNCTION(BlueprintPure, Category = "Smokestack|State")
	bool IsSealed() const { return bIsSealed; }

	/** Returns the number of active chimney mouths on this actor (1 for Single/Heavy, 2 or 3 for Clustered). */
	UFUNCTION(BlueprintPure, Category = "Smokestack|State")
	int32 GetActiveMouthCount() const;

	/** Returns the number of currently unsealed chimney mouths on this actor. */
	UFUNCTION(BlueprintPure, Category = "Smokestack|State")
	int32 GetUnsealedMouthCount() const;

	/** Scales the overhead smog canopy blocker radius based on global carbon accumulation (0.0 = initial, 1.0 = max choke). */
	UFUNCTION(BlueprintCallable, Category = "Smokestack|Environment")
	void SetCanopyExpansionAlpha(float ExpansionAlpha);

	// ==========================================
	// INTERFACE IMPLEMENTATIONS (ISP)
	// ==========================================
	virtual bool TrySealTarget_Implementation(AActor* SealInstigator) override;
	virtual bool IsTargetSealed_Implementation() const override;
	virtual FVector GetTargetRimLocation_Implementation() const override;
	virtual TArray<FVector> GetUnsealedRimLocations_Implementation() const override;

	virtual bool IsLethalCrashHazard_Implementation(const UPrimitiveComponent* HazardComponent) const override;
	virtual float GetWakeDownwashSpeed_Implementation(const UPrimitiveComponent* HazardComponent) const override;

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

	/** Blueprint cosmetic hook fired each time a chimney mouth is sealed and starts to sparkle (once per mouth). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Smokestack|Cosmetics", meta = (DisplayName = "On Smokestack Sealed (Cosmetics)"))
	void BP_OnSmokestackSealed();

private:
	// ==========================================
	// COMPONENTS
	// ==========================================
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Smokestack|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;

	/** Physical chimney visual mesh. Automatically switches between Single, Cluster, and Heavy meshes based on SmokestackType. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Smokestack|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> ChimneyBodyMesh;

	/** Physical crash collision capsule covering the primary chimney shaft below the open rim. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Smokestack|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCapsuleComponent> ChimneyBodyCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Smokestack|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCapsuleComponent> ChimneyBodyCollision_2;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Smokestack|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCapsuleComponent> ChimneyBodyCollision_3;

	/** Top opening target sphere (Default 240u radius) for receiving Carbon Seal projectiles. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Smokestack|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> RimTargetSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Smokestack|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> RimTargetSphere_2;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Smokestack|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> RimTargetSphere_3;

	/** Carbon seal cap mesh (SM_Seal_*) hidden while open and made visible on the rim upon sealing. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Smokestack|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> SealCapMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Smokestack|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> SealCapMesh_2;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Smokestack|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> SealCapMesh_3;

	/** Vertical hazard volume active only on Type C (HeavyLethal) while unsealed. Flying inside causes Crash. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Smokestack|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCapsuleComponent> LethalPlumeCapsule;

	/** Overhead smog canopy blocker at world Z CanopyWorldAltitude that blocks upward sunlight LineTraces until this stack is sealed. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Smokestack|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> OverheadSmogCanopySphere;

	/** Volumetric smoke cloud (NS_SkySmog) spreading from the top of the plume; its radius follows the canopy blocker and it dissipates when the stack is sealed (the sun pocket). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Smokestack|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNiagaraComponent> CanopySmokeVFX;

	/** Active dark smoke column VFX (NS_ToxicSmog or NS_HeavyToxicSmog). Deactivated upon sealing. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Smokestack|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNiagaraComponent> ToxicSmogVFX;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Smokestack|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNiagaraComponent> ToxicSmogVFX_2;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Smokestack|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNiagaraComponent> ToxicSmogVFX_3;

	/** Warm sparkle on a sealed rim (NS_SealSparkle), standing in for the solar panel glint until the panel mesh lands. Activated upon sealing. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Smokestack|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNiagaraComponent> SealSparkleVFX;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Smokestack|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNiagaraComponent> SealSparkleVFX_2;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Smokestack|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNiagaraComponent> SealSparkleVFX_3;

	/** Low pressurized rumble (SFX_Heavy_Smog_Rumble) heard near an unsealed Type C stack. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Smokestack|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAudioComponent> HeavyRumbleAudio;

	// ==========================================
	// DESIGNER TUNING / CONFIG
	// ==========================================
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Smokestack|Config", meta = (AllowPrivateAccess = "true"))
	ESmokestackType SmokestackType = ESmokestackType::Normal;

	/** Number of chimney mouths when SmokestackType is Clustered (2 = Twin Power Plant, 3 = Triple Power Plant). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Smokestack|Config", meta = (AllowPrivateAccess = "true", ClampMin = "2", ClampMax = "3"))
	int32 ClusterStackCount = 2;

	/** Per-placement height variation (multiplies the type's height scale), for less uniform skylines. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Smokestack|Config", meta = (AllowPrivateAccess = "true", ClampMin = "0.5", ClampMax = "2.0"))
	float HeightVariation = 1.0f;

	/** Vertical stretch per archetype: Normal (Type A) is drawn taller than its 1200u art so it clearly towers over Clustered (600u). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Smokestack|Config", meta = (AllowPrivateAccess = "true"))
	float NormalHeightScale = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Smokestack|Config", meta = (AllowPrivateAccess = "true"))
	float ClusteredHeightScale = 1.0f;

	/** 1400u art x 1.714 = ~2400u, the same mouth height as Type A. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Smokestack|Config", meta = (AllowPrivateAccess = "true"))
	float HeavyHeightScale = 1.714f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Smokestack|Config", meta = (AllowPrivateAccess = "true", ClampMin = "100.0", ClampMax = "500.0"))
	float RimSphereRadius = 240.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Smokestack|Config", meta = (AllowPrivateAccess = "true", ClampMin = "100.0"))
	float LethalPlumeRadius = 220.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Smokestack|Config", meta = (AllowPrivateAccess = "true", ClampMin = "200.0"))
	float LethalPlumeHalfHeight = 3400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Smokestack|Config", meta = (AllowPrivateAccess = "true", ClampMin = "200.0"))
	float CanopyMinRadius = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Smokestack|Config", meta = (AllowPrivateAccess = "true", ClampMin = "500.0"))
	float CanopyMaxRadius = 9000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Smokestack|Config", meta = (AllowPrivateAccess = "true"))
	float CanopyWorldAltitude = 8500.0f;

	// ==========================================
	// ARCHETYPE MESH ASSETS
	// ==========================================
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Smokestack|Assets", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMesh> MeshSingle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Smokestack|Assets", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMesh> MeshCluster;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Smokestack|Assets", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMesh> MeshCluster2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Smokestack|Assets", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMesh> MeshCluster3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Smokestack|Assets", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMesh> MeshHeavy;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Smokestack|Assets", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMesh> SealMeshSingle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Smokestack|Assets", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMesh> SealMeshCluster;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Smokestack|Assets", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMesh> SealMeshHeavy;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Smokestack|Assets", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNiagaraSystem> ToxicSmogSystem;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Smokestack|Assets", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNiagaraSystem> SealSparkleSystem;

	/** Pitch-black dense plume used instead of ToxicSmogSystem on Type C (HeavyLethal) stacks. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Smokestack|Assets", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNiagaraSystem> HeavyToxicSmogSystem;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Smokestack|Assets", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNiagaraSystem> SkySmogSystem;

	/** Played once at the rim when a mouth is sealed (SFX_Seal_Latch). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Smokestack|Assets", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USoundBase> SealLatchSound;

	/** Sealed-rim sparkle: particles per second, sprite size multiplier and spread as a fraction of RimSphereRadius. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Smokestack|Config", meta = (AllowPrivateAccess = "true"))
	float SealSparkleSpawnRate = 8.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Smokestack|Config", meta = (AllowPrivateAccess = "true"))
	float SealSparkleSizeScale = 0.6f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Smokestack|Config", meta = (AllowPrivateAccess = "true"))
	float SealSparkleSpreadFraction = 0.7f;

	/**
	 * The canopy blocker is the bottom cap of a huge sphere lifted this far above CanopyWorldAltitude, so it acts as a flat
	 * disc of the canopy radius (sagging only ~R^2 / 2*Lift below the ceiling) and upward traces never start inside it.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Smokestack|Config", meta = (AllowPrivateAccess = "true"))
	float CanopyDomeLift = 200000.0f;

	/** Height of the sky smoke cloud relative to CanopyWorldAltitude; kept above it so big sprites do not hang into the flight band. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Smokestack|Config", meta = (AllowPrivateAccess = "true"))
	float CanopySmokeOffsetZ = 1200.0f;

	/** Sky smoke spawn rate at the smallest and largest canopy radius (particles per second). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Smokestack|Config", meta = (AllowPrivateAccess = "true"))
	FVector2D CanopySmokeSpawnRateRange = FVector2D(3.0f, 16.0f);

	/** Sky smoke particle size multiplier at the smallest and largest canopy radius, so the cloud stays dense as it spreads. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Smokestack|Config", meta = (AllowPrivateAccess = "true"))
	FVector2D CanopySmokeSizeScaleRange = FVector2D(1.0f, 2.0f);

	// ==========================================
	// INTERNAL STATE
	// ==========================================
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Smokestack|State", meta = (AllowPrivateAccess = "true"))
	bool bIsSealed = false;

	bool bMouthSealed[3] = { false, false, false };

	void RefreshArchetypeCollisionState();
	void UpdateCanopyCloud(float Radius);
	void ApplyCanopyRadius(float Radius);
	UNiagaraSystem* GetActiveSmogSystem() const;
};

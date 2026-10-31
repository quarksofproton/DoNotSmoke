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
class ASmokestack;

UENUM(BlueprintType)
enum class ESmokestackType : uint8
{
	Normal      UMETA(DisplayName = "Type A - Normal Single"),
	Clustered   UMETA(DisplayName = "Type B - Clustered Power Plant"),
	HeavyLethal UMETA(DisplayName = "Type C - Heavy Lethal Plume")
};

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

	/** Blueprint cosmetic hook fired when the smokestack transitions from toxic smoke to clean steam. */
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

	/** Overhead smog canopy blocker at sky ceiling height that blocks upward sunlight LineTraces until this stack is sealed. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Smokestack|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> OverheadSmogCanopySphere;

	/** Active dark smoke column VFX (NS_ToxicSmog or NS_HeavyToxicSmog). Deactivated upon sealing. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Smokestack|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNiagaraComponent> ToxicSmogVFX;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Smokestack|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNiagaraComponent> ToxicSmogVFX_2;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Smokestack|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNiagaraComponent> ToxicSmogVFX_3;

	/** Clean white steam plume VFX (NS_CleanSteam). Activated upon sealing. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Smokestack|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNiagaraComponent> CleanSteamVFX;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Smokestack|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNiagaraComponent> CleanSteamVFX_2;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Smokestack|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNiagaraComponent> CleanSteamVFX_3;

	// ==========================================
	// DESIGNER TUNING / CONFIG
	// ==========================================
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Smokestack|Config", meta = (AllowPrivateAccess = "true"))
	ESmokestackType SmokestackType = ESmokestackType::Normal;

	/** Number of chimney mouths when SmokestackType is Clustered (2 = Twin Power Plant, 3 = Triple Power Plant). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Smokestack|Config", meta = (AllowPrivateAccess = "true", ClampMin = "2", ClampMax = "3"))
	int32 ClusterStackCount = 2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Smokestack|Config", meta = (AllowPrivateAccess = "true", ClampMin = "100.0", ClampMax = "500.0"))
	float RimSphereRadius = 240.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Smokestack|Config", meta = (AllowPrivateAccess = "true", ClampMin = "100.0"))
	float LethalPlumeRadius = 220.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Smokestack|Config", meta = (AllowPrivateAccess = "true", ClampMin = "200.0"))
	float LethalPlumeHalfHeight = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Smokestack|Config", meta = (AllowPrivateAccess = "true", ClampMin = "200.0"))
	float CanopyMinRadius = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Smokestack|Config", meta = (AllowPrivateAccess = "true", ClampMin = "500.0"))
	float CanopyMaxRadius = 3200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Smokestack|Config", meta = (AllowPrivateAccess = "true"))
	float CanopyWorldAltitude = 4250.0f;

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

	// ==========================================
	// INTERNAL STATE
	// ==========================================
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Smokestack|State", meta = (AllowPrivateAccess = "true"))
	bool bIsSealed = false;

	bool bMouthSealed[3] = { false, false, false };

	void RefreshArchetypeCollisionState();
};

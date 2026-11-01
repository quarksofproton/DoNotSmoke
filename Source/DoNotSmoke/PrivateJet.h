// Copyright Do Not Smoke Team. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayInterfaces.h"
#include "PrivateJet.generated.h"

class USplineComponent;
class UStaticMeshComponent;
class UBoxComponent;
class UTextRenderComponent;
class UNiagaraComponent;
class UAudioComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnJetApproachStateChangedSignature, bool, bIsApproachingOrPassing, FText, ActiveBannerSlogan);

/**
 * VIP Private Jet crossing the middle of the flight band (4500u) along a spline path.
 * Broadcasts HUD approach warnings (!), tows a two-sided satirical COP31 greenwashing banner,
 * pushes the glider away before it can get close, and trails smoke puffs over the play area that
 * shade everything below them from the sun for 5-10 s.
 */
UCLASS(Blueprintable)
class DONOTSMOKE_API APrivateJet : public AActor, public IBPI_FlightHazard
{
	GENERATED_BODY()

public:
	APrivateJet();

	virtual void Tick(float DeltaTime) override;

	// ==========================================
	// DISPATCHERS (DIP - Observer Pattern)
	// ==========================================
	UPROPERTY(BlueprintAssignable, Category = "PrivateJet|Events")
	FOnJetApproachStateChangedSignature OnJetApproachStateChanged;

	// ==========================================
	// PUBLIC API / TELEMETRY
	// ==========================================
	UFUNCTION(BlueprintPure, Category = "PrivateJet|State")
	bool IsJetActiveOrApproaching() const { return bIsApproaching || bIsFlyingPass; }

	UFUNCTION(BlueprintPure, Category = "PrivateJet|State")
	FText GetCurrentBannerSlogan() const;

	/** While a pass is flying: the jet's position and its repel radii (hard keep-out and soft push start). */
	bool GetRepelSphere(FVector& OutCenter, float& OutHardRadius, float& OutSoftRadius) const;

	/** True if a live smoke puff from this jet hangs above Location (within its radius and higher up). */
	bool IsShadedBySmoke(const FVector& Location) const;

	/** Puffs are only dropped inside this XY box (the play area), so the long route does not spawn smoke far away. */
	void SetSmokeArea(const FBox2D& Area) { SmokeArea = Area; }

	/** Starts the approach warning and pass now if the jet is idle; returns false if a pass is already running. */
	UFUNCTION(BlueprintCallable, Category = "PrivateJet|State")
	bool RequestFlybyPass();

	/** When false the jet never starts passes on its own and waits for RequestFlybyPass (GameMode dispatcher). */
	UFUNCTION(BlueprintCallable, Category = "PrivateJet|State")
	void SetAutoSchedulePasses(bool bEnabled) { bAutoSchedulePasses = bEnabled; }

	// ==========================================
	// INTERFACE IMPLEMENTATIONS (ISP)
	// ==========================================
	virtual bool IsLethalCrashHazard_Implementation(const UPrimitiveComponent* HazardComponent) const override;
	virtual float GetWakeDownwashSpeed_Implementation(const UPrimitiveComponent* HazardComponent) const override;

protected:
	virtual void BeginPlay() override;

	/** Blueprint cosmetic hook when jet approach warning (!) begins (play SFX_Jet_Roar). */
	UFUNCTION(BlueprintImplementableEvent, Category = "PrivateJet|Cosmetics", meta = (DisplayName = "On Jet Approach Warning"))
	void BP_OnJetApproachWarning(const FText& BannerSlogan);

private:
	// ==========================================
	// COMPONENTS
	// ==========================================
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PrivateJet|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PrivateJet|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USplineComponent> FlightSpline;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PrivateJet|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> MovingJetCarriage;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PrivateJet|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> JetBodyMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PrivateJet|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> JetBannerMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PrivateJet|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTextRenderComponent> BannerTextRender;

	/** Mirror of BannerTextRender on the other face of the banner, so the slogan reads from both sides. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PrivateJet|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTextRenderComponent> BannerTextRenderBack;

	/** Former downwash volume; kept on the actor but never enabled since the smoke trail replaced the downwash. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PrivateJet|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> WakeTurbulenceBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PrivateJet|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNiagaraComponent> JetWakeVFX;

	// ==========================================
	// DESIGNER TUNING / CONFIG (GDD Sec 3.3 & 5)
	// ==========================================
	/** World Z altitude (units) the jet is snapped to on BeginPlay if it was placed below it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PrivateJet|Config", meta = (AllowPrivateAccess = "true"))
	float JetAltitude = 4500.0f;

	/** Wake box location relative to the jet carriage (centre sits 1400u below jet altitude). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PrivateJet|Config", meta = (AllowPrivateAccess = "true"))
	FVector WakeBoxOffset = FVector(-1500.0f, 0.0f, -3500.0f);

	/** Wake box half-extent; with the offset it spans the jet altitude (8400u) down to 1400u. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PrivateJet|Config", meta = (AllowPrivateAccess = "true"))
	FVector WakeBoxExtent = FVector(3500.0f, 3200.0f, 3500.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PrivateJet|Config", meta = (AllowPrivateAccess = "true"))
	float JetFlightSpeed = 2800.0f;

	/** Cooldown seconds between consecutive VIP jet passes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PrivateJet|Config", meta = (AllowPrivateAccess = "true"))
	float PassIntervalSeconds = 18.0f;

	/** Lead time (seconds) before the jet starts moving during which the HUD red '!' warning flashes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PrivateJet|Config", meta = (AllowPrivateAccess = "true"))
	float ApproachWarningLeadSeconds = 2.5f;

	/** Jet engine fly-by roar (SFX_Jet_Roar), played from the moving carriage at the start of each pass. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PrivateJet|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAudioComponent> JetRoarAudio;

	/** Self-timed passes every PassIntervalSeconds; disabled by the GameMode when it dispatches jets itself. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PrivateJet|Config", meta = (AllowPrivateAccess = "true"))
	bool bAutoSchedulePasses = true;

	/** Glider keep-out: it can never get closer than RepelHardRadius and is pushed away from RepelSoftRadius inwards. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PrivateJet|Repel", meta = (AllowPrivateAccess = "true"))
	float RepelHardRadius = 3500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PrivateJet|Repel", meta = (AllowPrivateAccess = "true"))
	float RepelSoftRadius = 6500.0f;

	/** Smoke trail: one NS_SkySmog puff every SmokePuffSpacing units of flight, each lasting a random time in SmokePuffLifetimeRange. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PrivateJet|Smoke", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UNiagaraSystem> SmokePuffSystem;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PrivateJet|Smoke", meta = (AllowPrivateAccess = "true"))
	float SmokePuffSpacing = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PrivateJet|Smoke", meta = (AllowPrivateAccess = "true"))
	float SmokePuffRadius = 2200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PrivateJet|Smoke", meta = (AllowPrivateAccess = "true"))
	FVector2D SmokePuffLifetimeRange = FVector2D(5.0f, 10.0f);

	/** Puffs sit a little above the jet so the banner and fuselage stay visible below the trail. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PrivateJet|Smoke", meta = (AllowPrivateAccess = "true"))
	float SmokePuffOffsetZ = 400.0f;

	// ==========================================
	// INTERNAL STATE
	// ==========================================
	float CurrentSplineDistance = 0.0f;
	float WaitTimer = 0.0f;
	int32 CurrentSloganIndex = 0;

	struct FSmokePuff
	{
		TWeakObjectPtr<class UNiagaraComponent> VFX;
		FVector Center = FVector::ZeroVector;
		float ExpireTime = 0.0f;
	};
	TArray<FSmokePuff> SmokePuffs;
	float DistanceSinceLastPuff = 0.0f;
	FBox2D SmokeArea = FBox2D(ForceInit);

	void UpdateSmokeTrail(float DeltaTime);
	bool bIsApproaching = false;
	bool bIsFlyingPass = false;

	void BeginApproachWarning();
	void StartFlybyPass();
	void FinishFlybyPass();
};

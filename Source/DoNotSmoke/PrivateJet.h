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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnJetApproachStateChangedSignature, bool, bIsApproachingOrPassing, FText, ActiveBannerSlogan);

/**
 * Stratospheric VIP Private Jet (8400u altitude) flying along a spline path.
 * Broadcasts HUD approach warnings (!), displays rotating satirical COP31 greenwashing banners,
 * and trails a non-lethal WakeTurbulenceBox causing downwash altitude loss via IBPI_FlightHazard.
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

	/** Trailing wake turbulence volume spanning 8400u (jet altitude) down to 1400u (chimney top height) to catch gliders in the flight band. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PrivateJet|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> WakeTurbulenceBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PrivateJet|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNiagaraComponent> JetWakeVFX;

	// ==========================================
	// DESIGNER TUNING / CONFIG (GDD Sec 3.3 & 5)
	// ==========================================
	/** World Z altitude (units) the jet is snapped to on BeginPlay if it was placed below it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PrivateJet|Config", meta = (AllowPrivateAccess = "true"))
	float JetAltitude = 8400.0f;

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

	/** Downward velocity (units/sec) applied to the Glider when caught inside the trailing wake turbulence, down to the glider's WakeTargetFloorAltitude (1350u). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PrivateJet|Config", meta = (AllowPrivateAccess = "true"))
	float WakeDownwashSpeed = 2400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PrivateJet|Config", meta = (AllowPrivateAccess = "true"))
	TArray<FText> BannerSlogans;

	/** Self-timed passes every PassIntervalSeconds; disabled by the GameMode when it dispatches jets itself. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PrivateJet|Config", meta = (AllowPrivateAccess = "true"))
	bool bAutoSchedulePasses = true;

	// ==========================================
	// INTERNAL STATE
	// ==========================================
	float CurrentSplineDistance = 0.0f;
	float WaitTimer = 0.0f;
	int32 CurrentSloganIndex = 0;
	bool bIsApproaching = false;
	bool bIsFlyingPass = false;

	void BeginApproachWarning();
	void StartFlybyPass();
	void FinishFlybyPass();
};

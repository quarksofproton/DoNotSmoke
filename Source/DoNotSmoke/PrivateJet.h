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
 * Stratospheric VIP Private Jet (3000u altitude) flying along a spline path.
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

	/** Trailing wake turbulence volume extending downward into the glider flight band (1800u-2600u). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PrivateJet|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> WakeTurbulenceBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PrivateJet|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNiagaraComponent> JetWakeVFX;

	// ==========================================
	// DESIGNER TUNING / CONFIG (GDD Sec 3.3 & 5)
	// ==========================================
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PrivateJet|Config", meta = (AllowPrivateAccess = "true"))
	float JetFlightSpeed = 2800.0f;

	/** Cooldown seconds between consecutive VIP jet passes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PrivateJet|Config", meta = (AllowPrivateAccess = "true"))
	float PassIntervalSeconds = 18.0f;

	/** Lead time (seconds) before the jet starts moving during which the HUD red '!' warning flashes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PrivateJet|Config", meta = (AllowPrivateAccess = "true"))
	float ApproachWarningLeadSeconds = 2.5f;

	/** Downward velocity (units/sec) applied to the Glider when caught inside the trailing wake turbulence. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PrivateJet|Config", meta = (AllowPrivateAccess = "true"))
	float WakeDownwashSpeed = 2400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PrivateJet|Config", meta = (AllowPrivateAccess = "true"))
	TArray<FText> BannerSlogans;

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

// Copyright Do Not Smoke Team. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Blueprint/UserWidget.h"
#include "DoNotSmokeGameMode.generated.h"

class UProgressBar;
class UTextBlock;
class AGliderPawn;
class ASmokestack;
class APrivateJet;
class ADoNotSmokeGameMode;

/**
 * Live flight HUD displaying altitude, solar battery charge, crosshair & carbon seal cooldown,
 * sector carbon telemetry, and pull up / VIP jet warnings.
 */
UCLASS(Blueprintable)
class DONOTSMOKE_API UDoNotSmokeFlightHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "FlightHUD")
	TObjectPtr<UProgressBar> AltitudeBar;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "FlightHUD")
	TObjectPtr<UProgressBar> SolarBatteryBar;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "FlightHUD")
	TObjectPtr<UProgressBar> SealCooldownBar;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "FlightHUD")
	TObjectPtr<UTextBlock> StacksAndCO2Text;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "FlightHUD")
	TObjectPtr<UTextBlock> JetWarningText;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "FlightHUD")
	TObjectPtr<UTextBlock> PullUpWarningText;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "FlightHUD")
	TObjectPtr<UTextBlock> CrosshairText;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	TWeakObjectPtr<AGliderPawn> CachedGlider;
	TWeakObjectPtr<ADoNotSmokeGameMode> CachedGameMode;
	bool bBuiltFallbackTree = false;

	void BuildFallbackWidgetTree();
};

/**
 * Satirical COP31 Newspaper and Performance Summary Widget displayed upon level completion.
 */
UCLASS(Blueprintable)
class DONOTSMOKE_API UDoNotSmokeSummaryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;

	UFUNCTION(BlueprintCallable, Category = "Summary")
	void SetupSummaryData(int32 InStars, float InCarbonKg, float InTimeSeconds, float InAccuracyPercent, const FText& InHeadline);

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Summary")
	TObjectPtr<UTextBlock> HeadlineText;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Summary")
	TObjectPtr<UTextBlock> StarsText;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Summary")
	TObjectPtr<UTextBlock> CarbonText;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Summary")
	TObjectPtr<UTextBlock> TimeText;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Summary")
	TObjectPtr<UTextBlock> AccuracyText;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Summary")
	TObjectPtr<UTextBlock> SummaryDetailsText;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	bool bBuiltFallbackTree = false;

	void BuildFallbackWidgetTree();
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
	FOnLevelCompletedSignature,
	int32, StarRating,
	float, CumulativeCarbonKg,
	float, ElapsedSeconds,
	float, AccuracyPercent);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnPrivateJetAlertChangedSignature,
	bool, bIsJetApproaching,
	FText, ActiveBannerSlogan);

/**
 * Core Game Mode managing smokestack tracking, cumulative CO2 emissions (OpenStacks * 10 kg/s),
 * shrinking Sun Pockets ratio, COP31 1-3 Star rating, newspaper headlines, and Instant Restart on Crash.
 */
UCLASS(Blueprintable)
class DONOTSMOKE_API ADoNotSmokeGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ADoNotSmokeGameMode();

	virtual void Tick(float DeltaTime) override;

	// ==========================================
	// DISPATCHERS (DIP - Observer Pattern)
	// ==========================================
	UPROPERTY(BlueprintAssignable, Category = "DoNotSmoke|Events")
	FOnLevelCompletedSignature OnLevelCompleted;

	UPROPERTY(BlueprintAssignable, Category = "DoNotSmoke|Events")
	FOnPrivateJetAlertChangedSignature OnPrivateJetAlertChanged;

	// ==========================================
	// PUBLIC API / LEVEL TELEMETRY
	// ==========================================
	UFUNCTION(BlueprintPure, Category = "DoNotSmoke|Score")
	int32 GetOpenSmokestackCount() const { return OpenSmokestackCount; }

	UFUNCTION(BlueprintPure, Category = "DoNotSmoke|UI")
	UDoNotSmokeFlightHUDWidget* GetFlightHUDWidget() const { return FlightHUDWidget; }

	UFUNCTION(BlueprintPure, Category = "DoNotSmoke|UI")
	UDoNotSmokeSummaryWidget* GetSummaryWidget() const { return SummaryWidget; }

	UFUNCTION(BlueprintPure, Category = "DoNotSmoke|Score")
	int32 GetTotalSmokestackCount() const { return TotalSmokestackCount; }

	UFUNCTION(BlueprintPure, Category = "DoNotSmoke|Score")
	float GetCumulativeCarbonKg() const { return CumulativeCarbonKg; }

	UFUNCTION(BlueprintPure, Category = "DoNotSmoke|Score")
	float GetTargetMinCarbonKg() const { return TargetMinCarbonKg; }

	UFUNCTION(BlueprintPure, Category = "DoNotSmoke|Score")
	float GetElapsedLevelSeconds() const { return ElapsedLevelSeconds; }

	/** Returns current open sunlight corridor ratio (0.0 = sky choked, 1.0 = clear sky). */
	UFUNCTION(BlueprintPure, Category = "DoNotSmoke|Environment")
	float GetSunPocketOpenRatio() const { return SunPocketOpenRatio; }

	UFUNCTION(BlueprintPure, Category = "DoNotSmoke|Environment")
	bool IsPrivateJetAlertActive() const { return bIsPrivateJetAlertActive; }

	UFUNCTION(BlueprintPure, Category = "DoNotSmoke|Environment")
	FText GetActiveJetBannerSlogan() const { return ActiveJetBannerSlogan; }

	UFUNCTION(BlueprintPure, Category = "DoNotSmoke|Score")
	float GetShotAccuracyPercent() const;

	UFUNCTION(BlueprintPure, Category = "DoNotSmoke|Score")
	int32 CalculateStarRating() const;

	/** Returns the satirical COP31 front-page newspaper headline matching the 1-3 star rating. */
	UFUNCTION(BlueprintPure, Category = "DoNotSmoke|Score")
	FText GetNewspaperHeadlineForStars(int32 StarCount) const;

	/** Records a projectile launch for post-level accuracy statistics. */
	UFUNCTION(BlueprintCallable, Category = "DoNotSmoke|Score")
	void NotifyCarbonSealFired();

	/** Immediately restarts the active level upon a Glider Crash. */
	UFUNCTION(BlueprintCallable, Category = "DoNotSmoke|Flow")
	void TriggerInstantRestart();

protected:
	virtual void BeginPlay() override;

	/** Blueprint hook fired when all smokestacks in the sector are sealed. */
	UFUNCTION(BlueprintImplementableEvent, Category = "DoNotSmoke|Flow", meta = (DisplayName = "On Level Completed"))
	void BP_OnLevelCompleted(int32 StarRating, float FinalCarbonKg, float FinalTimeSeconds, float AccuracyPercent);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DoNotSmoke|UI")
	TSubclassOf<UDoNotSmokeFlightHUDWidget> FlightHUDWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DoNotSmoke|UI")
	TSubclassOf<UDoNotSmokeSummaryWidget> SummaryWidgetClass;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "DoNotSmoke|UI")
	TObjectPtr<UDoNotSmokeFlightHUDWidget> FlightHUDWidget;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "DoNotSmoke|UI")
	TObjectPtr<UDoNotSmokeSummaryWidget> SummaryWidget;

private:
	// ==========================================
	// DESIGNER TUNING / CONFIG (GDD Sec 3.4)
	// ==========================================
	/** Carbon emission rate per active open smokestack (Default: 10 kg CO2/s). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DoNotSmoke|Config", meta = (AllowPrivateAccess = "true"))
	float CarbonEmissionRatePerStack = 10.0f;

	/** Theoretical minimum carbon baseline (C_min) for 3-star evaluation (e.g., 500 / 1200 / 2200 kg). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DoNotSmoke|Config", meta = (AllowPrivateAccess = "true"))
	float TargetMinCarbonKg = 500.0f;

	/** Carbon ceiling at which unsealed smokestacks completely choke out Sun Pockets (Default: 2.5x C_min). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DoNotSmoke|Config", meta = (AllowPrivateAccess = "true"))
	float SkyChokeCarbonMultiplier = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DoNotSmoke|Debug", meta = (AllowPrivateAccess = "true"))
	bool bShowDebugHUD = true;

	// ==========================================
	// INTERNAL STATE
	// ==========================================
	int32 TotalSmokestackCount = 0;
	int32 OpenSmokestackCount = 0;
	int32 TotalShotsFired = 0;
	int32 TotalSuccessfulSeals = 0;

	float CumulativeCarbonKg = 0.0f;
	float ElapsedLevelSeconds = 0.0f;
	float SunPocketOpenRatio = 1.0f;
	bool bIsLevelCompleted = false;
	bool bIsPrivateJetAlertActive = false;
	FText ActiveJetBannerSlogan;

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<ASmokestack>> TrackedSmokestacks;

	UFUNCTION()
	void HandleSmokestackSealed(ASmokestack* SealedStack);

	UFUNCTION()
	void HandleGliderCrashed(AActor* CrashedGlider);

	UFUNCTION()
	void HandlePrivateJetApproachChanged(bool bIsApproaching, FText BannerSlogan);
};

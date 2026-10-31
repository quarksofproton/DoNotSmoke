// Copyright Do Not Smoke Team. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Blueprint/UserWidget.h"
#include "GameplayInterfaces.h"
#include "DoNotSmokeGameMode.generated.h"

class UTextBlock;
class UImage;
class UButton;
class UCanvasPanel;
class UTexture2D;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class AGliderPawn;
class ASmokestack;
class APrivateJet;
class ADoNotSmokeGameMode;

/**
 * Live flight HUD built from Berf's UI textures: crosshair with radial seal cooldown ring,
 * left solar battery arc, right altimeter ruler with moving arrow, top carbon bar,
 * VIP jet warning panel, and blinking "PULL UP!" panel with hazard vignette.
 */
UCLASS(Blueprintable)
class DONOTSMOKE_API UDoNotSmokeFlightHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UDoNotSmokeFlightHUDWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "FlightHUD")
	TObjectPtr<UImage> CrosshairImage;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "FlightHUD")
	TObjectPtr<UImage> CooldownRingBase;

	/** Radial fill driven by M_UI_ProgressMask (Mode 0). */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "FlightHUD")
	TObjectPtr<UImage> CooldownRingFill;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "FlightHUD")
	TObjectPtr<UImage> CooldownRingReady;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "FlightHUD")
	TObjectPtr<UImage> BatteryGaugeFrame;

	/** Bottom-to-top fill driven by M_UI_ProgressMask (Mode 1). */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "FlightHUD")
	TObjectPtr<UImage> BatteryFill;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "FlightHUD")
	TObjectPtr<UImage> AltimeterRuler;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "FlightHUD")
	TObjectPtr<UImage> AltimeterArrow;

	/** Left slot of the top bar: open stacks, sun pockets and live star rating. */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "FlightHUD")
	TObjectPtr<UTextBlock> StacksAndCO2Text;

	/** Right slot of the top bar: cumulative CO2 against C_min, next to the smoke cloud icon. */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "FlightHUD")
	TObjectPtr<UTextBlock> CarbonStatusText;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "FlightHUD")
	TObjectPtr<UImage> JetWarningPanel;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "FlightHUD")
	TObjectPtr<UTextBlock> JetWarningText;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "FlightHUD")
	TObjectPtr<UImage> PullUpPanel;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "FlightHUD")
	TObjectPtr<UImage> PullUpChevron;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "FlightHUD")
	TObjectPtr<UTextBlock> PullUpWarningText;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "FlightHUD")
	TObjectPtr<UImage> HazardVignette;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	// ==========================================
	// UI ASSETS (Content/Game/UI)
	// ==========================================
	UPROPERTY(EditDefaultsOnly, Category = "FlightHUD|Assets")
	TObjectPtr<UMaterialInterface> ProgressMaskMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "FlightHUD|Assets")
	TObjectPtr<UTexture2D> CrosshairTexture;

	UPROPERTY(EditDefaultsOnly, Category = "FlightHUD|Assets")
	TObjectPtr<UTexture2D> RingDarkTexture;

	UPROPERTY(EditDefaultsOnly, Category = "FlightHUD|Assets")
	TObjectPtr<UTexture2D> RingFillTexture;

	UPROPERTY(EditDefaultsOnly, Category = "FlightHUD|Assets")
	TObjectPtr<UTexture2D> RingReadyTexture;

	UPROPERTY(EditDefaultsOnly, Category = "FlightHUD|Assets")
	TObjectPtr<UTexture2D> ArcGaugeTexture;

	UPROPERTY(EditDefaultsOnly, Category = "FlightHUD|Assets")
	TObjectPtr<UTexture2D> ArcChargeTexture;

	UPROPERTY(EditDefaultsOnly, Category = "FlightHUD|Assets")
	TObjectPtr<UTexture2D> ArcBoostTexture;

	UPROPERTY(EditDefaultsOnly, Category = "FlightHUD|Assets")
	TObjectPtr<UTexture2D> ArcLowTexture;

	UPROPERTY(EditDefaultsOnly, Category = "FlightHUD|Assets")
	TObjectPtr<UTexture2D> AltimeterRulerTexture;

	UPROPERTY(EditDefaultsOnly, Category = "FlightHUD|Assets")
	TObjectPtr<UTexture2D> AltimeterArrowTexture;

	UPROPERTY(EditDefaultsOnly, Category = "FlightHUD|Assets")
	TObjectPtr<UTexture2D> TopBarTexture;

	UPROPERTY(EditDefaultsOnly, Category = "FlightHUD|Assets")
	TObjectPtr<UTexture2D> CarbonIconTexture;

	UPROPERTY(EditDefaultsOnly, Category = "FlightHUD|Assets")
	TObjectPtr<UTexture2D> WarningPanelTexture;

	UPROPERTY(EditDefaultsOnly, Category = "FlightHUD|Assets")
	TObjectPtr<UTexture2D> ChevronUpTexture;

	UPROPERTY(EditDefaultsOnly, Category = "FlightHUD|Assets")
	TObjectPtr<UTexture2D> HazardFrameTexture;

	/** Battery percentage below which the arc turns red. */
	UPROPERTY(EditDefaultsOnly, Category = "FlightHUD|Tuning")
	float LowBatteryPercent = 25.0f;

private:
	TWeakObjectPtr<AGliderPawn> CachedGlider;
	TWeakObjectPtr<ADoNotSmokeGameMode> CachedGameMode;
	bool bBuiltFallbackTree = false;
	float BlinkTime = 0.0f;

	/** Last text pushed to StacksAndCO2Text, so SetText only runs when the string changes. */
	FString LastStatusText;
	FString LastCarbonText;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> CooldownRingMID;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BatteryFillMID;

	void BuildFallbackWidgetTree();
	void CreateProgressMaterials();
	void UpdateCrosshairAndCooldown(AGliderPawn* Glider);
	void UpdateBattery(AGliderPawn* Glider);
	void UpdateAltimeter(AGliderPawn* Glider);

	/** Maps altitude onto the ruler texture so its colour bands line up with the altitude stack (0 / 400 / 600 / 1400 / 3900u). */
	static float AltitudeToRulerV(float Altitude);
};

/**
 * Crash screen: cracked-glass overlay, cause icon (ground / smokestack / toxic plume) and keycap restart button.
 */
UCLASS(Blueprintable)
class DONOTSMOKE_API UDoNotSmokeCrashWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UDoNotSmokeCrashWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativeOnInitialized() override;

	UFUNCTION(BlueprintCallable, Category = "Crash")
	void SetCrashCause(EGliderCrashCause Cause);

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Crash")
	TObjectPtr<UImage> CrackOverlay;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Crash")
	TObjectPtr<UImage> CauseIcon;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Crash")
	TObjectPtr<UTextBlock> CauseText;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Crash")
	TObjectPtr<UButton> RestartButton;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Crash")
	TObjectPtr<UTextBlock> RestartText;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UPROPERTY(EditDefaultsOnly, Category = "Crash|Assets")
	TObjectPtr<UTexture2D> CrackOverlayTexture;

	UPROPERTY(EditDefaultsOnly, Category = "Crash|Assets")
	TObjectPtr<UTexture2D> GroundCrashTexture;

	UPROPERTY(EditDefaultsOnly, Category = "Crash|Assets")
	TObjectPtr<UTexture2D> SmokestackCrashTexture;

	UPROPERTY(EditDefaultsOnly, Category = "Crash|Assets")
	TObjectPtr<UTexture2D> ToxicPlumeCrashTexture;

	UPROPERTY(EditDefaultsOnly, Category = "Crash|Assets")
	TObjectPtr<UTexture2D> KeycapTexture;

private:
	bool bBuiltFallbackTree = false;

	void BuildFallbackWidgetTree();

	UFUNCTION()
	void HandleRestartClicked();
};

/**
 * Satirical COP31 newspaper front page shown on level completion. Spins in, swaps the
 * valley photo by star rating (grim / clearing / sunny) and restarts on R / Enter / Space.
 */
UCLASS(Blueprintable)
class DONOTSMOKE_API UDoNotSmokeSummaryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UDoNotSmokeSummaryWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Summary")
	void SetupSummaryData(int32 InStars, float InCarbonKg, float InTargetCarbonKg, float InTimeSeconds, float InAccuracyPercent, const FText& InHeadline);

	/** Newspaper page container; spun and scaled in on reveal. */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Summary")
	TObjectPtr<UCanvasPanel> PagePanel;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Summary")
	TObjectPtr<UImage> PhotoImage;

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

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UPROPERTY(EditDefaultsOnly, Category = "Summary|Assets")
	TObjectPtr<UTexture2D> NewspaperTexture;

	UPROPERTY(EditDefaultsOnly, Category = "Summary|Assets")
	TObjectPtr<UTexture2D> ProhibitionTexture;

	UPROPERTY(EditDefaultsOnly, Category = "Summary|Assets")
	TObjectPtr<UTexture2D> StackPuffTexture;

	UPROPERTY(EditDefaultsOnly, Category = "Summary|Assets")
	TObjectPtr<UTexture2D> CarbonIconTexture;

	/** Valley photo per star rating: index 0 = 1 star (grim), 1 = 2 stars (clearing), 2 = 3 stars (sunny). */
	UPROPERTY(EditDefaultsOnly, Category = "Summary|Assets")
	TArray<TObjectPtr<UTexture2D>> ValleyPhotoTextures;

	UPROPERTY(EditDefaultsOnly, Category = "Summary|Tuning")
	float RevealDuration = 0.9f;

private:
	bool bBuiltFallbackTree = false;
	float RevealElapsed = 0.0f;

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
 * shrinking Sun Pockets ratio, COP31 1-3 Star rating, newspaper headlines, and the crash screen / restart flow.
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

	/** Console: finishes the level with the given star rating so the summary screen can be tested. */
	UFUNCTION(Exec, Category = "DoNotSmoke|Debug")
	void DebugForceLevelComplete(int32 Stars = 3);

	/** Reloads the active level (called from the crash screen restart button). */
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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DoNotSmoke|UI")
	TSubclassOf<UDoNotSmokeCrashWidget> CrashWidgetClass;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "DoNotSmoke|UI")
	TObjectPtr<UDoNotSmokeCrashWidget> CrashWidget;

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

	/** 3 stars when cumulative carbon stays within this multiple of C_min (Default: 1.25x). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DoNotSmoke|Config", meta = (AllowPrivateAccess = "true"))
	float ThreeStarCarbonMultiplier = 1.25f;

	/** 2 stars when cumulative carbon stays within this multiple of C_min (Default: 1.75x). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DoNotSmoke|Config", meta = (AllowPrivateAccess = "true"))
	float TwoStarCarbonMultiplier = 1.75f;

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
	bool bHasGliderCrashed = false;
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

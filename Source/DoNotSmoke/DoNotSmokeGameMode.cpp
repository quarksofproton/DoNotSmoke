// Copyright Do Not Smoke Team. All Rights Reserved.

#include "DoNotSmokeGameMode.h"
#include "GliderPawn.h"
#include "PrivateJet.h"
#include "Smokestack.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "UObject/ConstructorHelpers.h"

// ==========================================
// UDoNotSmokeFlightHUDWidget Implementation
// ==========================================

void UDoNotSmokeFlightHUDWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	BuildFallbackWidgetTree();
}

TSharedRef<SWidget> UDoNotSmokeFlightHUDWidget::RebuildWidget()
{
	BuildFallbackWidgetTree();
	return Super::RebuildWidget();
}

void UDoNotSmokeFlightHUDWidget::BuildFallbackWidgetTree()
{
	if (bBuiltFallbackTree)
	{
		return;
	}

	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
	}

	if (WidgetTree->RootWidget != nullptr)
	{
		bBuiltFallbackTree = true;
		return;
	}

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = RootCanvas;

	// 1. Right side: Altitude Bar (moving vertical indicator, 0..3900u)
	if (!AltitudeBar)
	{
		AltitudeBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("AltitudeBar"));
		AltitudeBar->SetBarFillType(EProgressBarFillType::BottomToTop);
		AltitudeBar->SetFillColorAndOpacity(FLinearColor(0.2f, 0.7f, 1.0f, 0.85f));
		if (UCanvasPanelSlot* Slot = RootCanvas->AddChildToCanvas(AltitudeBar))
		{
			Slot->SetAnchors(FAnchors(1.0f, 0.25f, 1.0f, 0.75f));
			Slot->SetAlignment(FVector2D(1.0f, 0.5f));
			Slot->SetOffsets(FMargin(-40.0f, 0.0f, 24.0f, 400.0f));
		}
	}

	// 2. Left side: Solar Battery ProgressBar (0..100%)
	if (!SolarBatteryBar)
	{
		SolarBatteryBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("SolarBatteryBar"));
		SolarBatteryBar->SetBarFillType(EProgressBarFillType::BottomToTop);
		SolarBatteryBar->SetFillColorAndOpacity(FLinearColor(1.0f, 0.85f, 0.1f, 0.85f));
		if (UCanvasPanelSlot* Slot = RootCanvas->AddChildToCanvas(SolarBatteryBar))
		{
			Slot->SetAnchors(FAnchors(0.0f, 0.25f, 0.0f, 0.75f));
			Slot->SetAlignment(FVector2D(0.0f, 0.5f));
			Slot->SetOffsets(FMargin(40.0f, 0.0f, 24.0f, 400.0f));
		}
	}

	// 3. Center: Dynamic 3D Projected Crosshair Text
	if (!CrosshairText)
	{
		CrosshairText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CrosshairText"));
		CrosshairText->SetText(FText::FromString(TEXT("+")));
		CrosshairText->SetJustification(ETextJustify::Center);
		CrosshairText->SetColorAndOpacity(FSlateColor(FLinearColor(0.2f, 0.95f, 1.0f, 0.9f)));
		if (UCanvasPanelSlot* Slot = RootCanvas->AddChildToCanvas(CrosshairText))
		{
			Slot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
			Slot->SetAlignment(FVector2D(0.5f, 0.5f));
			Slot->SetSize(FVector2D(48.0f, 48.0f));
			Slot->SetPosition(FVector2D(960.0f, 540.0f));
		}
	}

	// 4. Center: Cooldown ProgressBar (1.0 - GetSealCooldownNormalized())
	if (!SealCooldownBar)
	{
		SealCooldownBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("SealCooldownBar"));
		SealCooldownBar->SetBarFillType(EProgressBarFillType::LeftToRight);
		SealCooldownBar->SetFillColorAndOpacity(FLinearColor(0.1f, 0.9f, 0.3f, 0.85f));
		if (UCanvasPanelSlot* Slot = RootCanvas->AddChildToCanvas(SealCooldownBar))
		{
			Slot->SetAnchors(FAnchors(0.5f, 0.54f, 0.5f, 0.54f));
			Slot->SetAlignment(FVector2D(0.5f, 0.0f));
			Slot->SetOffsets(FMargin(0.0f, 0.0f, 100.0f, 10.0f));
		}
	}

	// 5. Top: Open Stacks & CO2 TextBlock
	if (!StacksAndCO2Text)
	{
		StacksAndCO2Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StacksAndCO2Text"));
		StacksAndCO2Text->SetJustification(ETextJustify::Center);
		StacksAndCO2Text->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.95f)));
		if (UCanvasPanelSlot* Slot = RootCanvas->AddChildToCanvas(StacksAndCO2Text))
		{
			Slot->SetAnchors(FAnchors(0.5f, 0.03f, 0.5f, 0.03f));
			Slot->SetAlignment(FVector2D(0.5f, 0.0f));
			Slot->SetOffsets(FMargin(0.0f, 0.0f, 950.0f, 32.0f));
		}
	}

	// 6. Top: VIP Jet Warning TextBlock
	if (!JetWarningText)
	{
		JetWarningText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("JetWarningText"));
		JetWarningText->SetJustification(ETextJustify::Center);
		JetWarningText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.2f, 0.2f, 1.0f)));
		JetWarningText->SetVisibility(ESlateVisibility::Hidden);
		if (UCanvasPanelSlot* Slot = RootCanvas->AddChildToCanvas(JetWarningText))
		{
			Slot->SetAnchors(FAnchors(0.5f, 0.08f, 0.5f, 0.08f));
			Slot->SetAlignment(FVector2D(0.5f, 0.0f));
			Slot->SetOffsets(FMargin(0.0f, 0.0f, 850.0f, 32.0f));
		}
	}

	// 7. Center-bottom: Red "PULL UP!" TextBlock
	if (!PullUpWarningText)
	{
		PullUpWarningText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PullUpWarningText"));
		PullUpWarningText->SetText(FText::FromString(TEXT("PULL UP!")));
		PullUpWarningText->SetJustification(ETextJustify::Center);
		PullUpWarningText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.05f, 0.05f, 1.0f)));
		PullUpWarningText->SetVisibility(ESlateVisibility::Hidden);
		if (UCanvasPanelSlot* Slot = RootCanvas->AddChildToCanvas(PullUpWarningText))
		{
			Slot->SetAnchors(FAnchors(0.5f, 0.72f, 0.5f, 0.72f));
			Slot->SetAlignment(FVector2D(0.5f, 0.5f));
			Slot->SetOffsets(FMargin(0.0f, 0.0f, 360.0f, 50.0f));
		}
	}

	bBuiltFallbackTree = true;
}

void UDoNotSmokeFlightHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!CachedGlider.IsValid())
	{
		CachedGlider = Cast<AGliderPawn>(GetOwningPlayerPawn());
	}
	if (!CachedGameMode.IsValid())
	{
		CachedGameMode = Cast<ADoNotSmokeGameMode>(UGameplayStatics::GetGameMode(this));
	}

	AGliderPawn* Glider = CachedGlider.Get();
	ADoNotSmokeGameMode* GM = CachedGameMode.Get();

	if (Glider)
	{
		// Dynamic 3D Projected Crosshair
		if (CrosshairText)
		{
			APlayerController* PC = GetOwningPlayer();
			if (!PC)
			{
				PC = UGameplayStatics::GetPlayerController(this, 0);
			}

			if (PC)
			{
				FVector2D ScreenPos = FVector2D::ZeroVector;
				const bool bProjected = UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(
					PC,
					Glider->GetAimWorldTargetLocation(),
					ScreenPos,
					true);

				if (bProjected)
				{
					if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(CrosshairText->Slot))
					{
						CanvasSlot->SetPosition(ScreenPos);
					}
				}
			}

			// Crosshair color & feedback:
			// Red on cooldown, Neon Green [+] when magnetic lock active, Yellow when clamped to cone, Cyan/White otherwise
			if (Glider->GetSealCooldownRemaining() > 0.0f)
			{
				CrosshairText->SetText(FText::FromString(TEXT("+")));
				CrosshairText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.15f, 0.15f, 0.85f)));
			}
			else if (Glider->HasMagneticLock())
			{
				CrosshairText->SetText(FText::FromString(TEXT("[+]")));
				CrosshairText->SetColorAndOpacity(FSlateColor(FLinearColor(0.1f, 1.0f, 0.2f, 1.0f)));
			}
			else if (Glider->IsAimConeClamped())
			{
				CrosshairText->SetText(FText::FromString(TEXT("+")));
				CrosshairText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.85f, 0.1f, 0.85f)));
			}
			else
			{
				CrosshairText->SetText(FText::FromString(TEXT("+")));
				CrosshairText->SetColorAndOpacity(FSlateColor(FLinearColor(0.2f, 0.95f, 1.0f, 0.9f)));
			}
		}

		// Right side: Altitude bar (GetCurrentAltitude() / 3900.0f)
		if (AltitudeBar)
		{
			const float AltFraction = FMath::Clamp(Glider->GetCurrentAltitude() / 3900.0f, 0.0f, 1.0f);
			AltitudeBar->SetPercent(AltFraction);
		}

		// Left side: Solar Battery ProgressBar (GetSolarChargePercent() / 100.0f)
		if (SolarBatteryBar)
		{
			const float BattFraction = FMath::Clamp(Glider->GetSolarChargePercent() / 100.0f, 0.0f, 1.0f);
			SolarBatteryBar->SetPercent(BattFraction);
		}

		// Center: Crosshair + Cooldown ProgressBar (1.0f - GetSealCooldownNormalized())
		if (SealCooldownBar)
		{
			const float ReadyFraction = FMath::Clamp(1.0f - Glider->GetSealCooldownNormalized(), 0.0f, 1.0f);
			SealCooldownBar->SetPercent(ReadyFraction);
		}

		// Center-bottom: Red "PULL UP!" TextBlock (IsPullUpWarningActive())
		if (PullUpWarningText)
		{
			const bool bPullUp = Glider->IsPullUpWarningActive();
			PullUpWarningText->SetVisibility(bPullUp ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
		}
	}

	if (GM)
	{
		// Top: Open Stacks & CO2 TextBlock
		if (StacksAndCO2Text)
		{
			const FString Msg = FString::Printf(
				TEXT("OPEN STACKS: %d / %d   |   CO2: %.1f kg (Target: %.0f)   |   SUN POCKETS: %.0f%%   |   RATING: %d*"),
				GM->GetOpenSmokestackCount(),
				GM->GetTotalSmokestackCount(),
				GM->GetCumulativeCarbonKg(),
				GM->GetTargetMinCarbonKg(),
				GM->GetSunPocketOpenRatio() * 100.0f,
				GM->CalculateStarRating()
			);
			StacksAndCO2Text->SetText(FText::FromString(Msg));
		}

		// Top: VIP Jet Warning TextBlock (IsPrivateJetAlertActive())
		if (JetWarningText)
		{
			const bool bJet = GM->IsPrivateJetAlertActive();
			if (bJet)
			{
				const FString Warning = FString::Printf(TEXT("[!] VIP JET WARNING: \"%s\""), *GM->GetActiveJetBannerSlogan().ToString());
				JetWarningText->SetText(FText::FromString(Warning));
				JetWarningText->SetVisibility(ESlateVisibility::Visible);
			}
			else
			{
				JetWarningText->SetVisibility(ESlateVisibility::Hidden);
			}
		}
	}
}

// ==========================================
// UDoNotSmokeSummaryWidget Implementation
// ==========================================

void UDoNotSmokeSummaryWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	BuildFallbackWidgetTree();
}

TSharedRef<SWidget> UDoNotSmokeSummaryWidget::RebuildWidget()
{
	BuildFallbackWidgetTree();
	return Super::RebuildWidget();
}

void UDoNotSmokeSummaryWidget::BuildFallbackWidgetTree()
{
	if (bBuiltFallbackTree)
	{
		return;
	}

	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
	}

	if (WidgetTree->RootWidget != nullptr)
	{
		bBuiltFallbackTree = true;
		return;
	}

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("SummaryRootCanvas"));
	WidgetTree->RootWidget = RootCanvas;

	if (!SummaryDetailsText)
	{
		SummaryDetailsText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SummaryDetailsText"));
		SummaryDetailsText->SetJustification(ETextJustify::Center);
		SummaryDetailsText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));
		if (UCanvasPanelSlot* Slot = RootCanvas->AddChildToCanvas(SummaryDetailsText))
		{
			Slot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
			Slot->SetAlignment(FVector2D(0.5f, 0.5f));
			Slot->SetOffsets(FMargin(0.0f, 0.0f, 1000.0f, 450.0f));
		}
	}

	bBuiltFallbackTree = true;
}

void UDoNotSmokeSummaryWidget::SetupSummaryData(int32 InStars, float InCarbonKg, float InTimeSeconds, float InAccuracyPercent, const FText& InHeadline)
{
	if (HeadlineText)
	{
		HeadlineText->SetText(InHeadline);
	}
	if (StarsText)
	{
		FString StarStr = TEXT("RATING: ");
		for (int32 s = 0; s < InStars; ++s) { StarStr += TEXT("★ "); }
		StarsText->SetText(FText::FromString(StarStr));
	}
	if (CarbonText)
	{
		CarbonText->SetText(FText::FromString(FString::Printf(TEXT("Cumulative CO2 Emitted: %.1f kg"), InCarbonKg)));
	}
	if (TimeText)
	{
		TimeText->SetText(FText::FromString(FString::Printf(TEXT("Mission Time: %.1f s"), InTimeSeconds)));
	}
	if (AccuracyText)
	{
		AccuracyText->SetText(FText::FromString(FString::Printf(TEXT("Carbon Seal Accuracy: %.0f%%"), InAccuracyPercent)));
	}
	if (SummaryDetailsText)
	{
		FString StarStr;
		for (int32 s = 0; s < InStars; ++s) { StarStr += TEXT("★ "); }

		const FString Details = FString::Printf(
			TEXT("====================================================\n"
			     "COP31 CLIMATE SUMMIT SPECIAL EDITION\n"
			     "====================================================\n\n"
			     "\"%s\"\n\n"
			     "RATING: %s (%d / 3 Stars)\n"
			     "CUMULATIVE CO2 EMISSIONS: %.1f kg\n"
			     "OPERATION TIME: %.1f seconds\n"
			     "CARBON SEAL ACCURACY: %.0f%%\n\n"
			     "====================================================\n"
			     "[ Sector Cleared - Smokestacks Permanently Sealed ]\n"
			     "===================================================="),
			*InHeadline.ToString(),
			*StarStr,
			InStars,
			InCarbonKg,
			InTimeSeconds,
			InAccuracyPercent
		);
		SummaryDetailsText->SetText(FText::FromString(Details));
	}
}

// ==========================================
// ADoNotSmokeGameMode Implementation
// ==========================================

ADoNotSmokeGameMode::ADoNotSmokeGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	DefaultPawnClass = AGliderPawn::StaticClass();

	static ConstructorHelpers::FClassFinder<UDoNotSmokeFlightHUDWidget> HUDWidgetFinder(TEXT("/Game/Game/UI/WBP_FlightHUD.WBP_FlightHUD_C"));
	if (HUDWidgetFinder.Succeeded())
	{
		FlightHUDWidgetClass = HUDWidgetFinder.Class;
	}
	else
	{
		FlightHUDWidgetClass = UDoNotSmokeFlightHUDWidget::StaticClass();
	}

	static ConstructorHelpers::FClassFinder<UDoNotSmokeSummaryWidget> SummaryWidgetFinder(TEXT("/Game/Game/UI/WBP_COP31_Summary.WBP_COP31_Summary_C"));
	if (SummaryWidgetFinder.Succeeded())
	{
		SummaryWidgetClass = SummaryWidgetFinder.Class;
	}
	else
	{
		SummaryWidgetClass = UDoNotSmokeSummaryWidget::StaticClass();
	}
}

void ADoNotSmokeGameMode::BeginPlay()
{
	Super::BeginPlay();

	TotalSmokestackCount = 0;
	OpenSmokestackCount = 0;
	TotalShotsFired = 0;
	TotalSuccessfulSeals = 0;
	CumulativeCarbonKg = 0.0f;
	ElapsedLevelSeconds = 0.0f;
	SunPocketOpenRatio = 1.0f;
	bIsLevelCompleted = false;
	bIsPrivateJetAlertActive = false;
	ActiveJetBannerSlogan = FText::GetEmpty();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Register all Smokestacks in the active level
	TrackedSmokestacks.Empty();
	TArray<AActor*> FoundStacks;
	UGameplayStatics::GetAllActorsOfClass(World, ASmokestack::StaticClass(), FoundStacks);

	for (AActor* Actor : FoundStacks)
	{
		if (ASmokestack* Stack = Cast<ASmokestack>(Actor))
		{
			TrackedSmokestacks.Add(Stack);
			TotalSmokestackCount += Stack->GetActiveMouthCount();
			if (!Stack->IsSealed())
			{
				OpenSmokestackCount += Stack->GetUnsealedMouthCount();
				Stack->OnSmokestackSealed.AddDynamic(this, &ADoNotSmokeGameMode::HandleSmokestackSealed);
			}
		}
	}

	// Register all Private Jets in the active level
	TArray<AActor*> FoundJets;
	UGameplayStatics::GetAllActorsOfClass(World, APrivateJet::StaticClass(), FoundJets);
	for (AActor* Actor : FoundJets)
	{
		if (APrivateJet* Jet = Cast<APrivateJet>(Actor))
		{
			Jet->OnJetApproachStateChanged.AddDynamic(this, &ADoNotSmokeGameMode::HandlePrivateJetApproachChanged);
		}
	}

	// Bind to player GliderPawn events for shot accounting and Crash restart
	if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0))
	{
		if (AGliderPawn* Glider = Cast<AGliderPawn>(PlayerPawn))
		{
			Glider->OnCarbonSealFired.AddDynamic(this, &ADoNotSmokeGameMode::NotifyCarbonSealFired);
			Glider->OnGliderCrashed.AddDynamic(this, &ADoNotSmokeGameMode::HandleGliderCrashed);
		}
	}

	// Automatically create and AddToViewport the FlightHUDWidget
	TSubclassOf<UDoNotSmokeFlightHUDWidget> TargetHUDClass = FlightHUDWidgetClass;
	if (!TargetHUDClass)
	{
		TargetHUDClass = UDoNotSmokeFlightHUDWidget::StaticClass();
	}
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0))
	{
		FlightHUDWidget = CreateWidget<UDoNotSmokeFlightHUDWidget>(PC, TargetHUDClass);
		if (FlightHUDWidget)
		{
			FlightHUDWidget->AddToViewport(10);
			bShowDebugHUD = false;
			if (AGliderPawn* Glider = Cast<AGliderPawn>(PC->GetPawn()))
			{
				Glider->SetShowDebugHUD(false);
			}
		}
	}
}

void ADoNotSmokeGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsLevelCompleted)
	{
		return;
	}

	ElapsedLevelSeconds += DeltaTime;

	if (OpenSmokestackCount > 0)
	{
		CumulativeCarbonKg += static_cast<float>(OpenSmokestackCount) * CarbonEmissionRatePerStack * DeltaTime;

		const float MaxChokeCarbon = FMath::Max(100.0f, TargetMinCarbonKg * SkyChokeCarbonMultiplier);
		const float ChokeAlpha = FMath::Clamp(CumulativeCarbonKg / MaxChokeCarbon, 0.0f, 1.0f);
		SunPocketOpenRatio = 1.0f - ChokeAlpha;

		// Expand overhead smog canopies on remaining unsealed smokestacks
		for (const TWeakObjectPtr<ASmokestack>& WeakStack : TrackedSmokestacks)
		{
			if (ASmokestack* Stack = WeakStack.Get())
			{
				Stack->SetCanopyExpansionAlpha(ChokeAlpha);
			}
		}
	}

	if (bShowDebugHUD && GEngine)
	{
		const FString SectorStatus = FString::Printf(
			TEXT("[COP31 AUDIT] Open Stacks: %d / %d | CO2 Emitted: %.1f kg (C_min: %.0f kg) | Sun Pockets: %.0f%% | Stars: %d* %s"),
			OpenSmokestackCount,
			TotalSmokestackCount,
			CumulativeCarbonKg,
			TargetMinCarbonKg,
			SunPocketOpenRatio * 100.0f,
			CalculateStarRating(),
			bIsPrivateJetAlertActive ? *FString::Printf(TEXT(" | [!] VIP JET APPROACHING: \"%s\""), *ActiveJetBannerSlogan.ToString()) : TEXT(""));

		GEngine->AddOnScreenDebugMessage(102, 0.0f, bIsPrivateJetAlertActive ? FColor::Red : FColor::Yellow, SectorStatus);
	}
}

float ADoNotSmokeGameMode::GetShotAccuracyPercent() const
{
	if (TotalShotsFired <= 0)
	{
		return 100.0f;
	}
	return FMath::Clamp((static_cast<float>(TotalSuccessfulSeals) / static_cast<float>(TotalShotsFired)) * 100.0f, 0.0f, 100.0f);
}

int32 ADoNotSmokeGameMode::CalculateStarRating() const
{
	if (CumulativeCarbonKg <= TargetMinCarbonKg * 1.25f)
	{
		return 3;
	}
	if (CumulativeCarbonKg <= TargetMinCarbonKg * 1.75f)
	{
		return 2;
	}
	return 1;
}

FText ADoNotSmokeGameMode::GetNewspaperHeadlineForStars(int32 StarCount) const
{
	switch (StarCount)
	{
	case 3:
		return FText::FromString(TEXT("GÖKYÜZÜ NEFES ALDI: GİZEMLİ PLANÖR SANAYİ BACALARINI KAYNAĞINDA MÜHÜRLEDİ!"));
	case 2:
		return FText::FromString(TEXT("EMİSYONLAR FRENLENDİ AMA KARBON BULUTU TAM DAĞILMADI"));
	default:
		return FText::FromString(TEXT("KIL PAYI KURTULUŞ: BACALAR KAPANDI AMA ŞEHİR İS YUTTU"));
	}
}

void ADoNotSmokeGameMode::NotifyCarbonSealFired()
{
	++TotalShotsFired;
}

void ADoNotSmokeGameMode::TriggerInstantRestart()
{
	if (UWorld* World = GetWorld())
	{
		const FString CurrentMapName = UGameplayStatics::GetCurrentLevelName(World, true);
		UGameplayStatics::OpenLevel(World, FName(*CurrentMapName));
	}
}

void ADoNotSmokeGameMode::HandleSmokestackSealed(ASmokestack* SealedStack)
{
	if (bIsLevelCompleted)
	{
		return;
	}

	++TotalSuccessfulSeals;
	OpenSmokestackCount = FMath::Max(0, OpenSmokestackCount - 1);

	if (OpenSmokestackCount == 0 && TotalSmokestackCount > 0)
	{
		bIsLevelCompleted = true;
		SunPocketOpenRatio = 1.0f;

		const int32 FinalStars = CalculateStarRating();
		const float FinalAccuracy = GetShotAccuracyPercent();
		const FText FinalHeadline = GetNewspaperHeadlineForStars(FinalStars);

		TSubclassOf<UDoNotSmokeSummaryWidget> TargetSummaryClass = SummaryWidgetClass;
		if (!TargetSummaryClass)
		{
			TargetSummaryClass = UDoNotSmokeSummaryWidget::StaticClass();
		}
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
		{
			SummaryWidget = CreateWidget<UDoNotSmokeSummaryWidget>(PC, TargetSummaryClass);
			if (SummaryWidget)
			{
				SummaryWidget->SetupSummaryData(FinalStars, CumulativeCarbonKg, ElapsedLevelSeconds, FinalAccuracy, FinalHeadline);
				SummaryWidget->AddToViewport(20);
			}
		}

		if (bShowDebugHUD && GEngine)
		{
			const FString SummaryMsg = FString::Printf(
				TEXT("=== SECTOR CLEARED (%d STARS) ===\n%s\nFinal CO2: %.1f kg | Time: %.1fs | Accuracy: %.0f%%"),
				FinalStars,
				*FinalHeadline.ToString(),
				CumulativeCarbonKg,
				ElapsedLevelSeconds,
				FinalAccuracy);
			GEngine->AddOnScreenDebugMessage(103, 15.0f, FColor::Green, SummaryMsg);
		}

		OnLevelCompleted.Broadcast(FinalStars, CumulativeCarbonKg, ElapsedLevelSeconds, FinalAccuracy);
		BP_OnLevelCompleted(FinalStars, CumulativeCarbonKg, ElapsedLevelSeconds, FinalAccuracy);
	}
}

void ADoNotSmokeGameMode::HandleGliderCrashed(AActor* CrashedGlider)
{
	if (!bIsLevelCompleted)
	{
		TriggerInstantRestart();
	}
}

void ADoNotSmokeGameMode::HandlePrivateJetApproachChanged(bool bIsApproaching, FText BannerSlogan)
{
	bIsPrivateJetAlertActive = bIsApproaching;
	ActiveJetBannerSlogan = BannerSlogan;
	OnPrivateJetAlertChanged.Broadcast(bIsPrivateJetAlertActive, ActiveJetBannerSlogan);
}

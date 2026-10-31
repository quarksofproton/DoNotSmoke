// Copyright Do Not Smoke Team. All Rights Reserved.

#include "DoNotSmokeGameMode.h"
#include "GliderPawn.h"
#include "PrivateJet.h"
#include "Smokestack.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

ADoNotSmokeGameMode::ADoNotSmokeGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	DefaultPawnClass = AGliderPawn::StaticClass();
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

		if (bShowDebugHUD && GEngine)
		{
			const FString SummaryMsg = FString::Printf(
				TEXT("=== SECTOR CLEARED (%d STARS) ===\n%s\nFinal CO2: %.1f kg | Time: %.1fs | Accuracy: %.0f%%"),
				FinalStars,
				*GetNewspaperHeadlineForStars(FinalStars).ToString(),
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

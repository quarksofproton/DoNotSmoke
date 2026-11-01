// Copyright Do Not Smoke Team. All Rights Reserved.

#include "DoNotSmokeGameMode.h"
#include "GliderPawn.h"
#include "PrivateJet.h"
#include "Smokestack.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Components/LightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

// ==========================================
// Shared UI helpers
// ==========================================

namespace DoNotSmokeUI
{
	template <typename T>
	static T* FindUIAsset(const TCHAR* Path)
	{
		ConstructorHelpers::FObjectFinder<T> Finder(Path);
		return Finder.Succeeded() ? Finder.Object : nullptr;
	}

	static UImage* AddImage(UWidgetTree* Tree, UCanvasPanel* Canvas, const FName Name, UTexture2D* Texture,
		const FAnchors& Anchors, const FVector2D& Alignment, const FVector2D& Position, const FVector2D& Size)
	{
		UImage* Image = Tree->ConstructWidget<UImage>(UImage::StaticClass(), Name);
		if (Texture)
		{
			Image->SetBrushFromTexture(Texture, false);
		}
		Image->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(Image))
		{
			Slot->SetAnchors(Anchors);
			Slot->SetAlignment(Alignment);
			Slot->SetPosition(Position);
			Slot->SetSize(Size);
		}
		return Image;
	}

	static UTextBlock* MakeText(UWidgetTree* Tree, const FName Name, int32 FontSize, const FLinearColor& Color)
	{
		UTextBlock* Text = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = FontSize;
		Text->SetFont(Font);
		Text->SetJustification(ETextJustify::Center);
		Text->SetColorAndOpacity(FSlateColor(Color));
		Text->SetVisibility(ESlateVisibility::HitTestInvisible);
		return Text;
	}

	/** Builds a run of star glyphs, optionally separated (e.g. TEXT(" ") for "★ ★ ★"). */
	static FString MakeStarString(int32 Count, const TCHAR* Separator = TEXT(""))
	{
		FString Stars;
		for (int32 s = 0; s < Count; ++s)
		{
			if (s > 0)
			{
				Stars += Separator;
			}
			Stars += TEXT("★");
		}
		return Stars;
	}

	/** Auto-sized and centred on Position so text stays vertically centred on its panel art. */
	static void AddText(UCanvasPanel* Canvas, UTextBlock* Text, const FAnchors& Anchors, const FVector2D& Position)
	{
		if (UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(Text))
		{
			Slot->SetAnchors(Anchors);
			Slot->SetAlignment(FVector2D(0.5f, 0.5f));
			Slot->SetPosition(Position);
			Slot->SetAutoSize(true);
		}
	}
}

// ==========================================
// UDoNotSmokeFlightHUDWidget Implementation
// ==========================================

// Layout reference: 1920x1080, scaled by the project DPI curve
namespace HUDLayout
{
	static const FVector2D CrosshairSize(200.0f, 200.0f);
	static const FVector2D CooldownRingSize(120.0f, 120.0f);

	static const FVector2D BatterySize(280.0f, 280.0f);
	static const FVector2D BatteryPosition(-80.0f, 0.0f);
	// Vertical extent of the coloured arc inside its 800x800 texture
	static constexpr float BatteryArcTopV = 132.0f / 800.0f;
	static constexpr float BatteryArcBottomV = 668.0f / 800.0f;

	static const FVector2D RulerSize(90.0f, 495.0f);
	static const FVector2D RulerPosition(-60.0f, 0.0f);
	static const FVector2D ArrowSize(44.0f, 44.0f);
	// Horizontal position of the tick column inside the ruler texture
	static constexpr float RulerTickU = 0.75f;
}

UDoNotSmokeFlightHUDWidget::UDoNotSmokeFlightHUDWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	using namespace DoNotSmokeUI;
	ProgressMaskMaterial = FindUIAsset<UMaterialInterface>(TEXT("/Game/Game/UI/Materials/M_UI_ProgressMask.M_UI_ProgressMask"));
	CrosshairTexture = FindUIAsset<UTexture2D>(TEXT("/Game/Game/UI/Textures/T_UI_Crosshair.T_UI_Crosshair"));
	RingDarkTexture = FindUIAsset<UTexture2D>(TEXT("/Game/Game/UI/Textures/T_UI_RingDark.T_UI_RingDark"));
	RingFillTexture = FindUIAsset<UTexture2D>(TEXT("/Game/Game/UI/Textures/T_UI_Ring.T_UI_Ring"));
	RingReadyTexture = FindUIAsset<UTexture2D>(TEXT("/Game/Game/UI/Textures/T_UI_RingGreen.T_UI_RingGreen"));
	ArcGaugeTexture = FindUIAsset<UTexture2D>(TEXT("/Game/Game/UI/Textures/T_UI_ArcGauge.T_UI_ArcGauge"));
	ArcChargeTexture = FindUIAsset<UTexture2D>(TEXT("/Game/Game/UI/Textures/T_UI_ArcGreen.T_UI_ArcGreen"));
	ArcBoostTexture = FindUIAsset<UTexture2D>(TEXT("/Game/Game/UI/Textures/T_UI_ArcBlue.T_UI_ArcBlue"));
	ArcLowTexture = FindUIAsset<UTexture2D>(TEXT("/Game/Game/UI/Textures/T_UI_ArcRed.T_UI_ArcRed"));
	AltimeterRulerTexture = FindUIAsset<UTexture2D>(TEXT("/Game/Game/UI/Textures/T_UI_GaugeAltitudeRuler.T_UI_GaugeAltitudeRuler"));
	AltimeterArrowTexture = FindUIAsset<UTexture2D>(TEXT("/Game/Game/UI/Textures/T_UI_ArrowLeft.T_UI_ArrowLeft"));
	TopBarTexture = FindUIAsset<UTexture2D>(TEXT("/Game/Game/UI/Textures/T_UI_HudBar.T_UI_HudBar"));
	CarbonIconTexture = FindUIAsset<UTexture2D>(TEXT("/Game/Game/UI/Textures/T_UI_SmokeCloud.T_UI_SmokeCloud"));
	WarningPanelTexture = FindUIAsset<UTexture2D>(TEXT("/Game/Game/UI/Textures/T_UI_WarningPanel.T_UI_WarningPanel"));
	ChevronUpTexture = FindUIAsset<UTexture2D>(TEXT("/Game/Game/UI/Textures/T_UI_ChevronUp.T_UI_ChevronUp"));
	HazardFrameTexture = FindUIAsset<UTexture2D>(TEXT("/Game/Game/UI/Textures/T_UI_HazardFrame.T_UI_HazardFrame"));
}

void UDoNotSmokeFlightHUDWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	BuildFallbackWidgetTree();
	CreateProgressMaterials();
}

TSharedRef<SWidget> UDoNotSmokeFlightHUDWidget::RebuildWidget()
{
	BuildFallbackWidgetTree();
	return Super::RebuildWidget();
}

void UDoNotSmokeFlightHUDWidget::BuildFallbackWidgetTree()
{
	using namespace DoNotSmokeUI;

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

	const FAnchors TopLeft(0.0f, 0.0f);
	const FAnchors Center(0.5f, 0.5f);
	const FAnchors LeftCenter(0.0f, 0.5f);
	const FAnchors RightCenter(1.0f, 0.5f);
	const FAnchors TopCenter(0.5f, 0.0f);
	const FVector2D Centered(0.5f, 0.5f);

	// 1. Full-screen red hazard vignette (behind everything, shown with PULL UP!)
	HazardVignette = AddImage(WidgetTree, RootCanvas, TEXT("HazardVignette"), HazardFrameTexture, FAnchors(0.0f, 0.0f, 1.0f, 1.0f), FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D::ZeroVector);
	if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(HazardVignette->Slot))
	{
		Slot->SetOffsets(FMargin(0.0f));
	}
	HazardVignette->SetVisibility(ESlateVisibility::Collapsed);

	// 2. Left: solar battery arc (frame + masked fill share the same box so the art lines up)
	BatteryGaugeFrame = AddImage(WidgetTree, RootCanvas, TEXT("BatteryGaugeFrame"), ArcGaugeTexture, LeftCenter, FVector2D(0.0f, 0.5f), HUDLayout::BatteryPosition, HUDLayout::BatterySize);
	BatteryFill = AddImage(WidgetTree, RootCanvas, TEXT("BatteryFill"), ArcChargeTexture, LeftCenter, FVector2D(0.0f, 0.5f), HUDLayout::BatteryPosition, HUDLayout::BatterySize);

	// 3. Right: altimeter ruler + moving arrow
	AltimeterRuler = AddImage(WidgetTree, RootCanvas, TEXT("AltimeterRuler"), AltimeterRulerTexture, RightCenter, FVector2D(1.0f, 0.5f), HUDLayout::RulerPosition, HUDLayout::RulerSize);
	AltimeterArrow = AddImage(WidgetTree, RootCanvas, TEXT("AltimeterArrow"), AltimeterArrowTexture, RightCenter, FVector2D(0.0f, 0.5f), FVector2D::ZeroVector, HUDLayout::ArrowSize);

	// 4. Center: cooldown ring stack + crosshair (repositioned every tick onto the projected aim point)
	CooldownRingBase = AddImage(WidgetTree, RootCanvas, TEXT("CooldownRingBase"), RingDarkTexture, TopLeft, Centered, FVector2D(960.0f, 540.0f), HUDLayout::CooldownRingSize);
	CooldownRingFill = AddImage(WidgetTree, RootCanvas, TEXT("CooldownRingFill"), RingFillTexture, TopLeft, Centered, FVector2D(960.0f, 540.0f), HUDLayout::CooldownRingSize);
	CooldownRingReady = AddImage(WidgetTree, RootCanvas, TEXT("CooldownRingReady"), RingReadyTexture, TopLeft, Centered, FVector2D(960.0f, 540.0f), HUDLayout::CooldownRingSize);
	CrosshairImage = AddImage(WidgetTree, RootCanvas, TEXT("CrosshairImage"), CrosshairTexture, TopLeft, Centered, FVector2D(960.0f, 540.0f), HUDLayout::CrosshairSize);

	// 5. Top: carbon bar
	// hud_bar art is 1500x200 with two dark slots centred at x 425 and 1075; drawn at 0.56 scale
	AddImage(WidgetTree, RootCanvas, TEXT("TopBarPanel"), TopBarTexture, TopCenter, FVector2D(0.5f, 0.0f), FVector2D(0.0f, 16.0f), FVector2D(840.0f, 112.0f));
	StacksAndCO2Text = MakeText(WidgetTree, TEXT("StacksAndCO2Text"), 15, FLinearColor(0.95f, 0.95f, 0.95f, 1.0f));
	AddText(RootCanvas, StacksAndCO2Text, TopCenter, FVector2D(-182.0f, 72.0f));
	AddImage(WidgetTree, RootCanvas, TEXT("CarbonIcon"), CarbonIconTexture, TopCenter, Centered, FVector2D(72.0f, 72.0f), FVector2D(52.0f, 52.0f));
	CarbonStatusText = MakeText(WidgetTree, TEXT("CarbonStatusText"), 15, FLinearColor(0.95f, 0.95f, 0.95f, 1.0f));
	AddText(RootCanvas, CarbonStatusText, TopCenter, FVector2D(205.0f, 72.0f));

	// 6. Top: VIP jet warning panel
	JetWarningPanel = AddImage(WidgetTree, RootCanvas, TEXT("JetWarningPanel"), WarningPanelTexture, TopCenter, FVector2D(0.5f, 0.0f), FVector2D(0.0f, 200.0f), FVector2D(600.0f, 150.0f));
	JetWarningText = MakeText(WidgetTree, TEXT("JetWarningText"), 20, FLinearColor(1.0f, 0.25f, 0.2f, 1.0f));
	AddText(RootCanvas, JetWarningText, TopCenter, FVector2D(0.0f, 275.0f));
	JetWarningPanel->SetVisibility(ESlateVisibility::Collapsed);
	JetWarningText->SetVisibility(ESlateVisibility::Collapsed);

	// 7. Lower center: PULL UP! chevron + panel
	const FAnchors PullUpAnchor(0.5f, 0.74f);
	PullUpChevron = AddImage(WidgetTree, RootCanvas, TEXT("PullUpChevron"), ChevronUpTexture, PullUpAnchor, Centered, FVector2D(0.0f, -105.0f), FVector2D(110.0f, 110.0f));
	PullUpPanel = AddImage(WidgetTree, RootCanvas, TEXT("PullUpPanel"), WarningPanelTexture, PullUpAnchor, Centered, FVector2D::ZeroVector, FVector2D(520.0f, 130.0f));
	PullUpWarningText = MakeText(WidgetTree, TEXT("PullUpWarningText"), 40, FLinearColor(1.0f, 0.15f, 0.1f, 1.0f));
	PullUpWarningText->SetText(FText::FromString(TEXT("PULL UP!")));
	AddText(RootCanvas, PullUpWarningText, PullUpAnchor, FVector2D::ZeroVector);
	PullUpChevron->SetVisibility(ESlateVisibility::Collapsed);
	PullUpPanel->SetVisibility(ESlateVisibility::Collapsed);
	PullUpWarningText->SetVisibility(ESlateVisibility::Collapsed);

	bBuiltFallbackTree = true;
}

void UDoNotSmokeFlightHUDWidget::CreateProgressMaterials()
{
	if (!ProgressMaskMaterial)
	{
		return;
	}

	if (CooldownRingFill && !CooldownRingMID)
	{
		CooldownRingMID = UMaterialInstanceDynamic::Create(ProgressMaskMaterial, this);
		CooldownRingMID->SetTextureParameterValue(TEXT("Tex"), RingFillTexture);
		CooldownRingMID->SetScalarParameterValue(TEXT("Mode"), 0.0f);
		CooldownRingFill->SetBrushFromMaterial(CooldownRingMID);
	}

	if (BatteryFill && !BatteryFillMID)
	{
		BatteryFillMID = UMaterialInstanceDynamic::Create(ProgressMaskMaterial, this);
		BatteryFillMID->SetTextureParameterValue(TEXT("Tex"), ArcChargeTexture);
		BatteryFillMID->SetScalarParameterValue(TEXT("Mode"), 1.0f);
		BatteryFillMID->SetScalarParameterValue(TEXT("LinearMin"), HUDLayout::BatteryArcTopV);
		BatteryFillMID->SetScalarParameterValue(TEXT("LinearMax"), HUDLayout::BatteryArcBottomV);
		BatteryFill->SetBrushFromMaterial(BatteryFillMID);
	}
}

float UDoNotSmokeFlightHUDWidget::AltitudeToRulerV(float Altitude)
{
	// Ruler texture rows (normalized V) for each altitude band boundary:
	// dark smog bed 0-400u, red pull-up band 400-600u, orange chimney band 600-1400u, green open sky 1400-7800u
	// These mirror the glider's KillFloor / SmogBed / PullUp / chimney band / GliderMaxCeilingAltitude values and are tied
	// to the ruler art, so they must be updated together with those properties.
	static const float Altitudes[] = { 0.0f, 400.0f, 600.0f, 1400.0f, 7800.0f };
	static const float RulerV[] = { 0.923f, 0.812f, 0.754f, 0.515f, 0.166f };

	if (Altitude <= Altitudes[0])
	{
		return RulerV[0];
	}
	for (int32 i = 1; i < UE_ARRAY_COUNT(Altitudes); ++i)
	{
		if (Altitude <= Altitudes[i])
		{
			const float Alpha = (Altitude - Altitudes[i - 1]) / (Altitudes[i] - Altitudes[i - 1]);
			return FMath::Lerp(RulerV[i - 1], RulerV[i], Alpha);
		}
	}
	return RulerV[UE_ARRAY_COUNT(RulerV) - 1];
}

void UDoNotSmokeFlightHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	using namespace DoNotSmokeUI;

	Super::NativeTick(MyGeometry, InDeltaTime);

	BlinkTime += InDeltaTime;

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

	// Hard on/off blink at ~3 Hz for warnings
	const bool bBlinkOn = FMath::Fmod(BlinkTime, 0.33f) < 0.2f;

	if (Glider)
	{
		UpdateCrosshairAndCooldown(Glider);
		UpdateBattery(Glider);
		UpdateAltimeter(Glider);

		const bool bPullUp = Glider->IsPullUpWarningActive();
		const ESlateVisibility PullUpVis = (bPullUp && bBlinkOn) ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
		if (PullUpPanel) { PullUpPanel->SetVisibility(PullUpVis); }
		if (PullUpChevron) { PullUpChevron->SetVisibility(PullUpVis); }
		if (PullUpWarningText) { PullUpWarningText->SetVisibility(PullUpVis); }
		if (HazardVignette)
		{
			HazardVignette->SetVisibility(bPullUp ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
			HazardVignette->SetRenderOpacity(0.55f + 0.45f * FMath::Abs(FMath::Sin(BlinkTime * 6.0f)));
		}
	}

	if (GM)
	{
		if (StacksAndCO2Text)
		{
			const FString Stars = MakeStarString(GM->CalculateStarRating());

			const FString Msg = FString::Printf(
				TEXT("STACKS %d/%d  SUN %.0f%%  %s"),
				GM->GetOpenSmokestackCount(),
				GM->GetTotalSmokestackCount(),
				GM->GetSunPocketOpenRatio() * 100.0f,
				*Stars);
			if (Msg != LastStatusText)
			{
				LastStatusText = Msg;
				StacksAndCO2Text->SetText(FText::FromString(Msg));
			}
		}

		if (CarbonStatusText)
		{
			const FString CarbonMsg = FString::Printf(TEXT("CO2 %.0f / %.0f kg"), GM->GetCumulativeCarbonKg(), GM->GetTargetMinCarbonKg());
			if (CarbonMsg != LastCarbonText)
			{
				LastCarbonText = CarbonMsg;
				CarbonStatusText->SetText(FText::FromString(CarbonMsg));
			}
		}

		const bool bJet = GM->IsPrivateJetAlertActive();
		const ESlateVisibility JetVis = (bJet && bBlinkOn) ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
		if (JetWarningPanel) { JetWarningPanel->SetVisibility(JetVis); }
		if (JetWarningText)
		{
			if (bJet)
			{
				JetWarningText->SetText(FText::FromString(FString::Printf(TEXT("!  VIP JET INCOMING  !\n\"%s\""), *GM->GetActiveJetBannerSlogan().ToString())));
			}
			JetWarningText->SetVisibility(JetVis);
		}
	}
}

void UDoNotSmokeFlightHUDWidget::UpdateCrosshairAndCooldown(AGliderPawn* Glider)
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		PC = UGameplayStatics::GetPlayerController(this, 0);
	}

	FVector2D ScreenPos = FVector2D::ZeroVector;
	if (PC && UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PC, Glider->GetAimWorldTargetLocation(), ScreenPos, true))
	{
		for (UImage* Image : { CooldownRingBase.Get(), CooldownRingFill.Get(), CooldownRingReady.Get(), CrosshairImage.Get() })
		{
			if (UCanvasPanelSlot* CanvasSlot = Image ? Cast<UCanvasPanelSlot>(Image->Slot) : nullptr)
			{
				CanvasSlot->SetPosition(ScreenPos);
			}
		}
	}

	const bool bCoolingDown = Glider->GetSealCooldownRemaining() > 0.0f;

	// Red on cooldown, green on magnetic lock, yellow when clamped to the aim cone, white otherwise
	if (CrosshairImage)
	{
		FLinearColor Tint = FLinearColor::White;
		if (bCoolingDown)
		{
			Tint = FLinearColor(1.0f, 0.3f, 0.3f, 0.9f);
		}
		else if (Glider->HasMagneticLock())
		{
			Tint = FLinearColor(0.3f, 1.0f, 0.45f, 1.0f);
		}
		else if (Glider->IsAimConeClamped())
		{
			Tint = FLinearColor(1.0f, 0.85f, 0.2f, 0.9f);
		}
		CrosshairImage->SetColorAndOpacity(Tint);
	}

	if (CooldownRingMID)
	{
		CooldownRingMID->SetScalarParameterValue(TEXT("Progress"), FMath::Clamp(1.0f - Glider->GetSealCooldownNormalized(), 0.0f, 1.0f));
	}
	if (CooldownRingFill)
	{
		CooldownRingFill->SetVisibility(bCoolingDown ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (CooldownRingReady)
	{
		CooldownRingReady->SetVisibility(bCoolingDown ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
}

void UDoNotSmokeFlightHUDWidget::UpdateBattery(AGliderPawn* Glider)
{
	if (!BatteryFillMID)
	{
		return;
	}

	const float Charge = Glider->GetSolarChargePercent();
	UTexture2D* ArcTexture = ArcChargeTexture;
	if (Glider->IsBoosting())
	{
		ArcTexture = ArcBoostTexture;
	}
	else if (Charge < LowBatteryPercent)
	{
		ArcTexture = ArcLowTexture;
	}

	BatteryFillMID->SetTextureParameterValue(TEXT("Tex"), ArcTexture);
	BatteryFillMID->SetScalarParameterValue(TEXT("Progress"), FMath::Clamp(Charge / 100.0f, 0.0f, 1.0f));
}

void UDoNotSmokeFlightHUDWidget::UpdateAltimeter(AGliderPawn* Glider)
{
	UCanvasPanelSlot* ArrowSlot = AltimeterArrow ? Cast<UCanvasPanelSlot>(AltimeterArrow->Slot) : nullptr;
	if (!ArrowSlot)
	{
		return;
	}

	// Arrow tip sits on the ruler's tick column; Y follows the ruler's colour bands
	const float RulerLeftX = HUDLayout::RulerPosition.X - HUDLayout::RulerSize.X;
	const float ArrowX = RulerLeftX + HUDLayout::RulerSize.X * HUDLayout::RulerTickU;
	const float ArrowY = (AltitudeToRulerV(Glider->GetCurrentAltitude()) - 0.5f) * HUDLayout::RulerSize.Y;
	ArrowSlot->SetPosition(FVector2D(ArrowX, ArrowY));
}

// ==========================================
// UDoNotSmokeCrashWidget Implementation
// ==========================================

UDoNotSmokeCrashWidget::UDoNotSmokeCrashWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	using namespace DoNotSmokeUI;
	CrackOverlayTexture = FindUIAsset<UTexture2D>(TEXT("/Game/Game/UI/Textures/T_UI_CrackOverlay.T_UI_CrackOverlay"));
	GroundCrashTexture = FindUIAsset<UTexture2D>(TEXT("/Game/Game/UI/Textures/T_UI_GliderDive.T_UI_GliderDive"));
	SmokestackCrashTexture = FindUIAsset<UTexture2D>(TEXT("/Game/Game/UI/Textures/T_UI_SmokestackBurst.T_UI_SmokestackBurst"));
	ToxicPlumeCrashTexture = FindUIAsset<UTexture2D>(TEXT("/Game/Game/UI/Textures/T_UI_SmokeWarning.T_UI_SmokeWarning"));
	KeycapTexture = FindUIAsset<UTexture2D>(TEXT("/Game/Game/UI/Textures/T_UI_Keycap.T_UI_Keycap"));

	SetIsFocusable(true);
}

void UDoNotSmokeCrashWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	BuildFallbackWidgetTree();

	if (RestartButton)
	{
		RestartButton->OnClicked.AddUniqueDynamic(this, &UDoNotSmokeCrashWidget::HandleRestartClicked);
	}
}

TSharedRef<SWidget> UDoNotSmokeCrashWidget::RebuildWidget()
{
	BuildFallbackWidgetTree();
	return Super::RebuildWidget();
}

void UDoNotSmokeCrashWidget::BuildFallbackWidgetTree()
{
	using namespace DoNotSmokeUI;

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

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CrashRootCanvas"));
	WidgetTree->RootWidget = RootCanvas;

	const FVector2D Centered(0.5f, 0.5f);

	// 1. Cracked-glass vignette stretched over the whole screen
	CrackOverlay = AddImage(WidgetTree, RootCanvas, TEXT("CrackOverlay"), CrackOverlayTexture, FAnchors(0.0f, 0.0f, 1.0f, 1.0f), FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D::ZeroVector);
	if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(CrackOverlay->Slot))
	{
		Slot->SetOffsets(FMargin(0.0f));
	}

	// 2. Cause icon + label
	CauseIcon = AddImage(WidgetTree, RootCanvas, TEXT("CauseIcon"), GroundCrashTexture, FAnchors(0.5f, 0.4f), Centered, FVector2D::ZeroVector, FVector2D(340.0f, 340.0f));
	CauseText = MakeText(WidgetTree, TEXT("CauseText"), 40, FLinearColor(1.0f, 0.95f, 0.9f, 1.0f));
	AddText(RootCanvas, CauseText, FAnchors(0.5f, 0.62f), FVector2D::ZeroVector);

	// 3. Restart button on the keycap texture
	RestartButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("RestartButton"));
	if (KeycapTexture)
	{
		FSlateBrush KeycapBrush;
		KeycapBrush.SetResourceObject(KeycapTexture);
		KeycapBrush.ImageSize = FVector2D(KeycapTexture->GetSizeX(), KeycapTexture->GetSizeY());
		KeycapBrush.DrawAs = ESlateBrushDrawType::Image;

		FButtonStyle Style = RestartButton->GetStyle();
		Style.Normal = KeycapBrush;
		Style.Hovered = KeycapBrush;
		Style.Hovered.TintColor = FSlateColor(FLinearColor(1.15f, 1.15f, 1.15f, 1.0f));
		Style.Pressed = KeycapBrush;
		Style.Pressed.TintColor = FSlateColor(FLinearColor(0.75f, 0.75f, 0.75f, 1.0f));
		Style.NormalPadding = FMargin(0.0f);
		Style.PressedPadding = FMargin(0.0f, 4.0f, 0.0f, 0.0f);
		RestartButton->SetStyle(Style);
	}

	RestartText = MakeText(WidgetTree, TEXT("RestartText"), 30, FLinearColor(0.12f, 0.13f, 0.16f, 1.0f));
	RestartText->SetText(FText::FromString(TEXT("RESTART  [R]")));
	RestartButton->AddChild(RestartText);

	if (UCanvasPanelSlot* Slot = RootCanvas->AddChildToCanvas(RestartButton))
	{
		Slot->SetAnchors(FAnchors(0.5f, 0.78f));
		Slot->SetAlignment(Centered);
		Slot->SetSize(FVector2D(440.0f, 110.0f));
	}

	bBuiltFallbackTree = true;
}

void UDoNotSmokeCrashWidget::SetCrashCause(EGliderCrashCause Cause)
{
	UTexture2D* Icon = GroundCrashTexture;
	FString Label = TEXT("GROUND IMPACT");

	switch (Cause)
	{
	case EGliderCrashCause::Smokestack:
		Icon = SmokestackCrashTexture;
		Label = TEXT("SMOKESTACK COLLISION");
		break;
	case EGliderCrashCause::ToxicPlume:
		Icon = ToxicPlumeCrashTexture;
		Label = TEXT("CHOKED IN TOXIC PLUME");
		break;
	default:
		break;
	}

	if (CauseIcon && Icon)
	{
		CauseIcon->SetBrushFromTexture(Icon, false);
	}
	if (CauseText)
	{
		CauseText->SetText(FText::FromString(Label));
	}
}

FReply UDoNotSmokeCrashWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::R || Key == EKeys::Enter || Key == EKeys::SpaceBar)
	{
		HandleRestartClicked();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UDoNotSmokeCrashWidget::HandleRestartClicked()
{
	if (ADoNotSmokeGameMode* GM = Cast<ADoNotSmokeGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GM->TriggerInstantRestart();
	}
}

// ==========================================
// UDoNotSmokeSummaryWidget Implementation
// ==========================================

// Newspaper art is 800x1000; every page element below is placed in those pixel coordinates
namespace NewspaperLayout
{
	static constexpr float PageScale = 0.88f;
	static const FVector2D PageSize(800.0f * PageScale, 1000.0f * PageScale);
	static const FLinearColor InkColor(0.15f, 0.16f, 0.19f, 1.0f);

	static FVector2D Px(float X, float Y) { return FVector2D(X, Y) * PageScale; }

	static void Place(UCanvasPanel* Page, UWidget* Widget, float X0, float Y0, float X1, float Y1)
	{
		if (UCanvasPanelSlot* Slot = Page->AddChildToCanvas(Widget))
		{
			Slot->SetAnchors(FAnchors(0.0f, 0.0f));
			Slot->SetPosition(Px(X0, Y0));
			Slot->SetSize(Px(X1 - X0, Y1 - Y0));
		}
	}

	/** Transparent border that centres its content inside a page box. */
	static UBorder* MakeCentredBox(UWidgetTree* Tree, const FName Name, UWidget* Content)
	{
		UBorder* Box = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), Name);
		Box->SetBrushColor(FLinearColor::Transparent);
		Box->SetPadding(FMargin(0.0f));
		Box->SetHorizontalAlignment(HAlign_Center);
		Box->SetVerticalAlignment(VAlign_Center);
		Box->SetContent(Content);
		return Box;
	}

	static void SetBold(UTextBlock* Text)
	{
		FSlateFontInfo Font = Text->GetFont();
		Font.TypefaceFontName = TEXT("Bold");
		Text->SetFont(Font);
	}
}

UDoNotSmokeSummaryWidget::UDoNotSmokeSummaryWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	using namespace DoNotSmokeUI;
	NewspaperTexture = FindUIAsset<UTexture2D>(TEXT("/Game/Game/UI/Textures/T_UI_Newspaper.T_UI_Newspaper"));
	ProhibitionTexture = FindUIAsset<UTexture2D>(TEXT("/Game/Game/UI/Textures/T_UI_Prohibition.T_UI_Prohibition"));
	StackPuffTexture = FindUIAsset<UTexture2D>(TEXT("/Game/Game/UI/Textures/T_UI_StackPuff.T_UI_StackPuff"));
	CarbonIconTexture = FindUIAsset<UTexture2D>(TEXT("/Game/Game/UI/Textures/T_UI_SmokeCloud.T_UI_SmokeCloud"));
	ValleyPhotoTextures = {
		FindUIAsset<UTexture2D>(TEXT("/Game/Game/UI/Textures/T_UI_Valley1Star.T_UI_Valley1Star")),
		FindUIAsset<UTexture2D>(TEXT("/Game/Game/UI/Textures/T_UI_Valley2Star.T_UI_Valley2Star")),
		FindUIAsset<UTexture2D>(TEXT("/Game/Game/UI/Textures/T_UI_ValleyScene.T_UI_ValleyScene")) };

	SetIsFocusable(true);
}

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
	using namespace DoNotSmokeUI;
	using namespace NewspaperLayout;

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

	// 1. Dim the frozen game behind the page
	UImage* Backdrop = AddImage(WidgetTree, RootCanvas, TEXT("Backdrop"), nullptr, FAnchors(0.0f, 0.0f, 1.0f, 1.0f), FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D::ZeroVector);
	Backdrop->SetColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.6f));
	if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Backdrop->Slot))
	{
		Slot->SetOffsets(FMargin(0.0f));
	}

	// 2. Page container (the reveal spins this)
	PagePanel = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("PagePanel"));
	if (UCanvasPanelSlot* Slot = RootCanvas->AddChildToCanvas(PagePanel))
	{
		Slot->SetAnchors(FAnchors(0.5f, 0.5f));
		Slot->SetAlignment(FVector2D(0.5f, 0.5f));
		Slot->SetSize(PageSize);
	}

	UImage* Page = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("NewspaperPage"));
	Page->SetBrushFromTexture(NewspaperTexture, false);
	Place(PagePanel, Page, 0.0f, 0.0f, 800.0f, 1000.0f);

	// 3. Masthead: Do Not Smoke logo (chimney under the prohibition sign) + paper name
	UImage* LogoStack = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("LogoStack"));
	LogoStack->SetBrushFromTexture(StackPuffTexture, false);
	Place(PagePanel, LogoStack, 116.0f, 56.0f, 190.0f, 136.0f);
	UImage* LogoSign = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("LogoSign"));
	LogoSign->SetBrushFromTexture(ProhibitionTexture, false);
	Place(PagePanel, LogoSign, 104.0f, 47.0f, 200.0f, 143.0f);

	UTextBlock* Masthead = MakeText(WidgetTree, TEXT("MastheadText"), 34, FLinearColor(0.95f, 0.93f, 0.88f, 1.0f));
	SetBold(Masthead);
	Masthead->SetText(FText::FromString(TEXT("THE DAILY SMOG")));
	Place(PagePanel, MakeCentredBox(WidgetTree, TEXT("MastheadBox"), Masthead), 205.0f, 42.0f, 705.0f, 148.0f);

	UTextBlock* Edition = MakeText(WidgetTree, TEXT("EditionText"), 10, InkColor);
	Edition->SetText(FText::FromString(TEXT("COP31 CLIMATE SUMMIT  ·  SPECIAL EDITION")));
	Place(PagePanel, MakeCentredBox(WidgetTree, TEXT("EditionBox"), Edition), 56.0f, 180.0f, 745.0f, 200.0f);

	// 4. Headline
	HeadlineText = MakeText(WidgetTree, TEXT("HeadlineText"), 19, InkColor);
	SetBold(HeadlineText);
	HeadlineText->SetAutoWrapText(true);
	Place(PagePanel, MakeCentredBox(WidgetTree, TEXT("HeadlineBox"), HeadlineText), 66.0f, 218.0f, 735.0f, 307.0f);

	// 5. Photo window, clipped so the 16:10 valley art fills the wide frame
	UCanvasPanel* PhotoClip = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("PhotoClip"));
	PhotoClip->SetClipping(EWidgetClipping::ClipToBounds);
	Place(PagePanel, PhotoClip, 76.0f, 347.0f, 724.0f, 580.0f);
	PhotoImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("PhotoImage"));
	PhotoImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UCanvasPanelSlot* Slot = PhotoClip->AddChildToCanvas(PhotoImage))
	{
		const float PhotoHeight = 648.0f * 1000.0f / 1600.0f;
		Slot->SetAnchors(FAnchors(0.0f, 0.0f));
		Slot->SetPosition(Px(0.0f, (233.0f - PhotoHeight) * 0.5f));
		Slot->SetSize(Px(648.0f, PhotoHeight));
	}

	// 6. Audit box (dashed frame): stars, CO2 vs C_min, time, accuracy
	UTextBlock* AuditTab = MakeText(WidgetTree, TEXT("AuditTabText"), 10, FLinearColor(0.95f, 0.93f, 0.88f, 1.0f));
	SetBold(AuditTab);
	AuditTab->SetText(FText::FromString(TEXT("AUDIT")));
	Place(PagePanel, MakeCentredBox(WidgetTree, TEXT("AuditTabBox"), AuditTab), 344.0f, 604.0f, 410.0f, 632.0f);

	UVerticalBox* AuditList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("AuditList"));
	StarsText = MakeText(WidgetTree, TEXT("StarsText"), 34, FLinearColor(0.85f, 0.2f, 0.15f, 1.0f));
	if (UVerticalBoxSlot* Slot = AuditList->AddChildToVerticalBox(StarsText))
	{
		Slot->SetHorizontalAlignment(HAlign_Center);
		Slot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
	}

	UHorizontalBox* CarbonRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("CarbonRow"));
	UImage* CarbonIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("SummaryCarbonIcon"));
	CarbonIcon->SetBrushFromTexture(CarbonIconTexture, false);
	CarbonIcon->SetDesiredSizeOverride(FVector2D(34.0f, 34.0f));
	if (UHorizontalBoxSlot* Slot = CarbonRow->AddChildToHorizontalBox(CarbonIcon))
	{
		Slot->SetVerticalAlignment(VAlign_Center);
		Slot->SetPadding(FMargin(0.0f, 0.0f, 6.0f, 0.0f));
	}
	CarbonText = MakeText(WidgetTree, TEXT("CarbonText"), 15, InkColor);
	if (UHorizontalBoxSlot* Slot = CarbonRow->AddChildToHorizontalBox(CarbonText))
	{
		Slot->SetVerticalAlignment(VAlign_Center);
	}
	if (UVerticalBoxSlot* Slot = AuditList->AddChildToVerticalBox(CarbonRow))
	{
		Slot->SetHorizontalAlignment(HAlign_Center);
	}

	TimeText = MakeText(WidgetTree, TEXT("TimeText"), 15, InkColor);
	AuditList->AddChildToVerticalBox(TimeText)->SetHorizontalAlignment(HAlign_Center);
	AccuracyText = MakeText(WidgetTree, TEXT("AccuracyText"), 15, InkColor);
	AuditList->AddChildToVerticalBox(AccuracyText)->SetHorizontalAlignment(HAlign_Center);

	Place(PagePanel, MakeCentredBox(WidgetTree, TEXT("AuditBox"), AuditList), 352.0f, 638.0f, 738.0f, 852.0f);

	// 7. Footer strip: restart prompt
	UTextBlock* Footer = MakeText(WidgetTree, TEXT("FooterText"), 14, InkColor);
	SetBold(Footer);
	Footer->SetText(FText::FromString(TEXT("[R]  RESTART SECTOR")));
	Place(PagePanel, MakeCentredBox(WidgetTree, TEXT("FooterBox"), Footer), 95.0f, 885.0f, 657.0f, 930.0f);

	bBuiltFallbackTree = true;
}

void UDoNotSmokeSummaryWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!PagePanel || RevealElapsed >= RevealDuration)
	{
		return;
	}

	// Classic spinning-newspaper reveal: two turns, ease-out, settling on a slight tilt
	RevealElapsed = FMath::Min(RevealElapsed + InDeltaTime, RevealDuration);
	const float Alpha = RevealElapsed / FMath::Max(RevealDuration, KINDA_SMALL_NUMBER);
	const float Eased = 1.0f - FMath::Pow(1.0f - Alpha, 3.0f);
	const float Angle = FMath::Lerp(-720.0f, -3.0f, Eased);
	const float Scale = FMath::Lerp(0.1f, 1.0f, Eased);
	PagePanel->SetRenderTransform(FWidgetTransform(FVector2D::ZeroVector, FVector2D(Scale, Scale), FVector2D::ZeroVector, Angle));
}

FReply UDoNotSmokeSummaryWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::R || Key == EKeys::Enter || Key == EKeys::SpaceBar)
	{
		if (ADoNotSmokeGameMode* GM = Cast<ADoNotSmokeGameMode>(UGameplayStatics::GetGameMode(this)))
		{
			GM->TriggerInstantRestart();
		}
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UDoNotSmokeSummaryWidget::SetupSummaryData(int32 InStars, float InCarbonKg, float InTargetCarbonKg, float InTimeSeconds, float InAccuracyPercent, const FText& InHeadline)
{
	using namespace DoNotSmokeUI;

	const int32 Stars = FMath::Clamp(InStars, 1, 3);

	if (HeadlineText)
	{
		HeadlineText->SetText(InHeadline);
	}
	if (StarsText)
	{
		FString StarStr = MakeStarString(Stars, TEXT(" "));
		for (int32 s = Stars; s < 3; ++s) { StarStr += TEXT(" ☆"); }
		StarsText->SetText(FText::FromString(StarStr));
	}
	if (CarbonText)
	{
		CarbonText->SetText(FText::FromString(FString::Printf(TEXT("CO2  %.0f kg  /  target %.0f kg"), InCarbonKg, InTargetCarbonKg)));
	}
	if (TimeText)
	{
		TimeText->SetText(FText::FromString(FString::Printf(TEXT("TIME  %.1f s"), InTimeSeconds)));
	}
	if (AccuracyText)
	{
		AccuracyText->SetText(FText::FromString(FString::Printf(TEXT("SEAL ACCURACY  %.0f%%"), InAccuracyPercent)));
	}
	if (PhotoImage && ValleyPhotoTextures.IsValidIndex(Stars - 1) && ValleyPhotoTextures[Stars - 1])
	{
		PhotoImage->SetBrushFromTexture(ValleyPhotoTextures[Stars - 1], false);
	}

	RevealElapsed = 0.0f;
}

// ==========================================
// ADoNotSmokeGameMode Implementation
// ==========================================

ADoNotSmokeGameMode::ADoNotSmokeGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	DefaultPawnClass = AGliderPawn::StaticClass();

	FlightHUDWidgetClass = UDoNotSmokeFlightHUDWidget::StaticClass();
	SummaryWidgetClass = UDoNotSmokeSummaryWidget::StaticClass();
	CrashWidgetClass = UDoNotSmokeCrashWidget::StaticClass();
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
	bHasGliderCrashed = false;
	bIsPrivateJetAlertActive = false;
	ActiveJetBannerSlogan = FText::GetEmpty();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Register all Smokestacks in the active level
	TrackedSmokestacks.Empty();
	for (TActorIterator<ASmokestack> It(World); It; ++It)
	{
		ASmokestack* Stack = *It;
		TrackedSmokestacks.Add(Stack);
		TotalSmokestackCount += Stack->GetActiveMouthCount();
		if (!Stack->IsSealed())
		{
			OpenSmokestackCount += Stack->GetUnsealedMouthCount();
			Stack->OnSmokestackSealed.AddDynamic(this, &ADoNotSmokeGameMode::HandleSmokestackSealed);
		}
	}

	// Register all Private Jets in the active level
	// Sky actors driven by carbon: first directional light is the sun, first height fog carries the high smoke haze
	TActorIterator<ADirectionalLight> SunIt(World);
	if (SunIt)
	{
		SunLight = *SunIt;
		BaseSunIntensity = SunIt->GetLightComponent() ? SunIt->GetLightComponent()->Intensity : 0.0f;
	}
	TActorIterator<AExponentialHeightFog> FogIt(World);
	if (FogIt)
	{
		SkyFog = *FogIt;
	}
	ApplySkyChoke(0.0f);

	for (TActorIterator<APrivateJet> It(World); It; ++It)
	{
		It->OnJetApproachStateChanged.AddDynamic(this, &ADoNotSmokeGameMode::HandlePrivateJetApproachChanged);
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
		}
	}
}

void ADoNotSmokeGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsLevelCompleted || bHasGliderCrashed)
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
		ApplySkyChoke(ChokeAlpha);

		// Expand overhead smog canopies on remaining unsealed smokestacks
		for (const TWeakObjectPtr<ASmokestack>& WeakStack : TrackedSmokestacks)
		{
			if (ASmokestack* Stack = WeakStack.Get())
			{
				Stack->SetCanopyExpansionAlpha(ChokeAlpha);
			}
		}
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
	if (CumulativeCarbonKg <= TargetMinCarbonKg * ThreeStarCarbonMultiplier)
	{
		return 3;
	}
	if (CumulativeCarbonKg <= TargetMinCarbonKg * TwoStarCarbonMultiplier)
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

void ADoNotSmokeGameMode::ApplySkyChoke(float ChokeAlpha)
{
	const float Alpha = FMath::Clamp(ChokeAlpha, 0.0f, 1.0f);

	if (ADirectionalLight* Sun = SunLight.Get())
	{
		if (ULightComponent* SunComponent = Sun->GetLightComponent())
		{
			SunComponent->SetIntensity(BaseSunIntensity * FMath::Lerp(1.0f, ChokedSunIntensityScale, Alpha));
		}
	}

	if (AExponentialHeightFog* Fog = SkyFog.Get())
	{
		if (UExponentialHeightFogComponent* FogComponent = Fog->GetComponent())
		{
			// Second fog layer = smoke haze high in the sky, independent of the smog bed's first layer
			FogComponent->SecondFogData.FogDensity = ChokedHighSmogDensity * Alpha;
			FogComponent->SecondFogData.FogHeightOffset = HighSmogHeightOffset;
			FogComponent->MarkRenderStateDirty();
		}
	}
}

void ADoNotSmokeGameMode::DebugForceLevelComplete(int32 Stars)
{
#if !UE_BUILD_SHIPPING
	if (bIsLevelCompleted || bHasGliderCrashed)
	{
		return;
	}

	// Pick a carbon total inside the requested star band, then seal the "last" mouth
	const int32 ClampedStars = FMath::Clamp(Stars, 1, 3);
	const float Multiplier = ClampedStars == 3 ? 1.0f : (ClampedStars == 2 ? (ThreeStarCarbonMultiplier + TwoStarCarbonMultiplier) * 0.5f : TwoStarCarbonMultiplier + 0.5f);
	CumulativeCarbonKg = TargetMinCarbonKg * Multiplier;
	OpenSmokestackCount = 1;
	TotalSmokestackCount = FMath::Max(TotalSmokestackCount, 1);
	HandleSmokestackSealed(nullptr);
#endif
}

void ADoNotSmokeGameMode::TriggerInstantRestart()
{
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0))
		{
			PC->SetInputMode(FInputModeGameOnly());
			PC->SetShowMouseCursor(false);
		}

		const FString CurrentMapName = UGameplayStatics::GetCurrentLevelName(World, true);
		UGameplayStatics::OpenLevel(World, FName(*CurrentMapName));
	}
}

void ADoNotSmokeGameMode::HandleSmokestackSealed(ASmokestack* SealedStack)
{
	if (bIsLevelCompleted || bHasGliderCrashed)
	{
		return;
	}

	++TotalSuccessfulSeals;
	OpenSmokestackCount = FMath::Max(0, OpenSmokestackCount - 1);

	if (OpenSmokestackCount == 0 && TotalSmokestackCount > 0)
	{
		bIsLevelCompleted = true;
		SunPocketOpenRatio = 1.0f;
		ApplySkyChoke(0.0f);

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
				SummaryWidget->SetupSummaryData(FinalStars, CumulativeCarbonKg, TargetMinCarbonKg, ElapsedLevelSeconds, FinalAccuracy, FinalHeadline);
				SummaryWidget->AddToViewport(20);

				if (FlightHUDWidget)
				{
					FlightHUDWidget->SetVisibility(ESlateVisibility::Collapsed);
				}

				FInputModeUIOnly InputMode;
				InputMode.SetWidgetToFocus(SummaryWidget->TakeWidget());
				InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
				PC->SetInputMode(InputMode);
				PC->SetShowMouseCursor(true);
			}
		}

		OnLevelCompleted.Broadcast(FinalStars, CumulativeCarbonKg, ElapsedLevelSeconds, FinalAccuracy);
		BP_OnLevelCompleted(FinalStars, CumulativeCarbonKg, ElapsedLevelSeconds, FinalAccuracy);
	}
}

void ADoNotSmokeGameMode::HandleGliderCrashed(AActor* CrashedGlider)
{
	if (bIsLevelCompleted || bHasGliderCrashed)
	{
		return;
	}

	bHasGliderCrashed = true;

	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	TSubclassOf<UDoNotSmokeCrashWidget> TargetCrashClass = CrashWidgetClass ? CrashWidgetClass : TSubclassOf<UDoNotSmokeCrashWidget>(UDoNotSmokeCrashWidget::StaticClass());
	CrashWidget = PC ? CreateWidget<UDoNotSmokeCrashWidget>(PC, TargetCrashClass) : nullptr;
	if (!CrashWidget)
	{
		TriggerInstantRestart();
		return;
	}

	const AGliderPawn* Glider = Cast<AGliderPawn>(CrashedGlider);
	CrashWidget->SetCrashCause(Glider ? Glider->GetLastCrashCause() : EGliderCrashCause::Ground);
	CrashWidget->AddToViewport(30);

	if (FlightHUDWidget)
	{
		FlightHUDWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(CrashWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PC->SetInputMode(InputMode);
	PC->SetShowMouseCursor(true);
}

void ADoNotSmokeGameMode::HandlePrivateJetApproachChanged(bool bIsApproaching, FText BannerSlogan)
{
	bIsPrivateJetAlertActive = bIsApproaching;
	ActiveJetBannerSlogan = BannerSlogan;
	OnPrivateJetAlertChanged.Broadcast(bIsPrivateJetAlertActive, ActiveJetBannerSlogan);
}

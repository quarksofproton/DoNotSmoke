// Copyright Do Not Smoke Team. All Rights Reserved.

#include "DoNotSmokeGameMode.h"
#include "DoNotSmokeLocale.h"
#include "GliderPawn.h"
#include "PrivateJet.h"
#include "Smokestack.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "TimerManager.h"
#include "Engine/DirectionalLight.h"
#include "Components/LightComponent.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
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
#include "Components/SizeBox.h"
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

	static void SetBold(UTextBlock* Text)
	{
		FSlateFontInfo Font = Text->GetFont();
		Font.TypefaceFontName = TEXT("Bold");
		Text->SetFont(Font);
	}

	/** Placeholder button: a flat rounded rectangle that brightens on hover, until Berf's button art arrives. */
	static UButton* MakeFlatButton(UWidgetTree* Tree, const FName Name, const FLinearColor& Color, UTextBlock* Label)
	{
		UButton* Button = Tree->ConstructWidget<UButton>(UButton::StaticClass(), Name);

		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.OutlineSettings = FSlateBrushOutlineSettings(10.0f);
		Brush.TintColor = FSlateColor(Color);

		FButtonStyle Style = Button->GetStyle();
		Style.Normal = Brush;
		Style.Hovered = Brush;
		Style.Hovered.TintColor = FSlateColor(Color * 1.35f);
		Style.Pressed = Brush;
		Style.Pressed.TintColor = FSlateColor(Color * 0.7f);
		Style.NormalPadding = FMargin(0.0f);
		Style.PressedPadding = FMargin(0.0f, 3.0f, 0.0f, 0.0f);
		Button->SetStyle(Style);

		Label->SetVisibility(ESlateVisibility::HitTestInvisible);
		Button->AddChild(Label);
		return Button;
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
	static const FVector2D CrosshairSize(100.0f, 100.0f);
	static const FVector2D CooldownRingSize(60.0f, 60.0f);

	static const FVector2D BatterySize(340.0f, 340.0f);
	static const FVector2D BatteryPosition(-97.0f, 0.0f);
	// Sun-bolt icon centred over the arc's top cap (cap centre at U 0.57, top edge at V 0.09 of the 800x800 art)
	static const FVector2D BatteryIconSize(84.0f, 84.0f);
	static const FVector2D BatteryIconPosition(BatteryPosition.X + BatterySize.X * 0.57f, BatterySize.Y * (0.09f - 0.5f) - 44.0f);
	// Vertical extent of the coloured arc inside its 800x800 texture
	static constexpr float BatteryArcTopV = 132.0f / 800.0f;
	static constexpr float BatteryArcBottomV = 668.0f / 800.0f;

	static const FVector2D RulerSize(90.0f, 495.0f);
	static const FVector2D RulerPosition(-60.0f, 0.0f);
	static const FVector2D ArrowSize(44.0f, 44.0f);
	// Horizontal position of the tick column inside the ruler texture
	static constexpr float RulerTickU = 0.75f;
	// Centre of the small dark window at the top of the 400x2200 ruler art, used for the altitude readout
	static constexpr float RulerWindowV = 0.051f;
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
	SunBoltTexture = FindUIAsset<UTexture2D>(TEXT("/Game/Game/UI/Textures/T_UI_SunBolt.T_UI_SunBolt"));
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
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(HazardVignette->Slot))
	{
		CanvasSlot->SetOffsets(FMargin(0.0f));
	}
	HazardVignette->SetVisibility(ESlateVisibility::Collapsed);

	// 2. Left: solar battery arc (frame + masked fill share the same box so the art lines up)
	BatteryGaugeFrame = AddImage(WidgetTree, RootCanvas, TEXT("BatteryGaugeFrame"), ArcGaugeTexture, LeftCenter, FVector2D(0.0f, 0.5f), HUDLayout::BatteryPosition, HUDLayout::BatterySize);
	BatteryFill = AddImage(WidgetTree, RootCanvas, TEXT("BatteryFill"), ArcChargeTexture, LeftCenter, FVector2D(0.0f, 0.5f), HUDLayout::BatteryPosition, HUDLayout::BatterySize);
	BatteryIcon = AddImage(WidgetTree, RootCanvas, TEXT("BatteryIcon"), SunBoltTexture, LeftCenter, Centered, HUDLayout::BatteryIconPosition, HUDLayout::BatteryIconSize);

	// 3. Right: altimeter ruler + moving arrow
	AltimeterRuler = AddImage(WidgetTree, RootCanvas, TEXT("AltimeterRuler"), AltimeterRulerTexture, RightCenter, FVector2D(1.0f, 0.5f), HUDLayout::RulerPosition, HUDLayout::RulerSize);
	AltimeterArrow = AddImage(WidgetTree, RootCanvas, TEXT("AltimeterArrow"), AltimeterArrowTexture, RightCenter, FVector2D(0.0f, 0.5f), FVector2D::ZeroVector, HUDLayout::ArrowSize);
	AltitudeText = MakeText(WidgetTree, TEXT("AltitudeText"), 12, FLinearColor(0.95f, 0.95f, 0.95f, 1.0f));
	SetBold(AltitudeText);
	AddText(RootCanvas, AltitudeText, RightCenter, FVector2D(HUDLayout::RulerPosition.X - HUDLayout::RulerSize.X * 0.5f, (HUDLayout::RulerWindowV - 0.5f) * HUDLayout::RulerSize.Y));

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
	// Sits right of the top bar at the same height (bar spans x -420..420, y 16..128)
	JetWarningPanel = AddImage(WidgetTree, RootCanvas, TEXT("JetWarningPanel"), WarningPanelTexture, TopCenter, FVector2D(0.0f, 0.0f), FVector2D(430.0f, 16.0f), FVector2D(448.0f, 112.0f));
	JetWarningText = MakeText(WidgetTree, TEXT("JetWarningText"), 18, FLinearColor(1.0f, 0.25f, 0.2f, 1.0f));
	JetWarningText->SetText(DoNotSmokeLocale::Text(TEXT("HudJetIncoming")));
	AddText(RootCanvas, JetWarningText, TopCenter, FVector2D(654.0f, 72.0f));
	JetWarningPanel->SetVisibility(ESlateVisibility::Collapsed);
	JetWarningText->SetVisibility(ESlateVisibility::Collapsed);

	// 7. Lower center: PULL UP! chevron + panel
	const FAnchors PullUpAnchor(0.5f, 0.74f);
	PullUpChevron = AddImage(WidgetTree, RootCanvas, TEXT("PullUpChevron"), ChevronUpTexture, PullUpAnchor, Centered, FVector2D(0.0f, -105.0f), FVector2D(110.0f, 110.0f));
	PullUpPanel = AddImage(WidgetTree, RootCanvas, TEXT("PullUpPanel"), WarningPanelTexture, PullUpAnchor, Centered, FVector2D::ZeroVector, FVector2D(520.0f, 130.0f));
	PullUpWarningText = MakeText(WidgetTree, TEXT("PullUpWarningText"), 40, FLinearColor(1.0f, 0.15f, 0.1f, 1.0f));
	PullUpWarningText->SetText(DoNotSmokeLocale::Text(TEXT("HudPullUp")));
	AddText(RootCanvas, PullUpWarningText, PullUpAnchor, FVector2D::ZeroVector);
	PullUpChevron->SetVisibility(ESlateVisibility::Collapsed);
	PullUpPanel->SetVisibility(ESlateVisibility::Collapsed);
	PullUpWarningText->SetVisibility(ESlateVisibility::Collapsed);

	// 8. Top left: controls tutorial panel (shown at takeoff, fades out later)
	BuildControlsPanel(RootCanvas);

	// 9. Center: big "3, 2, 1, RENEW IT!" countdown
	CountdownText = MakeText(WidgetTree, TEXT("CountdownText"), 120, FLinearColor(1.0f, 0.86f, 0.25f, 1.0f));
	SetBold(CountdownText);
	FSlateFontInfo CountdownFont = CountdownText->GetFont();
	CountdownFont.OutlineSettings.OutlineSize = 4;
	CountdownFont.OutlineSettings.OutlineColor = FLinearColor(0.05f, 0.05f, 0.08f, 0.85f);
	CountdownText->SetFont(CountdownFont);
	AddText(RootCanvas, CountdownText, Center, FVector2D(0.0f, -60.0f));
	CountdownText->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	CountdownText->SetVisibility(ESlateVisibility::Collapsed);

	bBuiltFallbackTree = true;
}

void UDoNotSmokeFlightHUDWidget::BuildControlsPanel(UCanvasPanel* RootCanvas)
{
	using namespace DoNotSmokeUI;

	const FLinearColor KeyColor(1.0f, 0.86f, 0.25f, 1.0f);
	const FLinearColor ActionColor(0.95f, 0.95f, 0.95f, 1.0f);

	UVerticalBox* Rows = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ControlsRows"));

	UTextBlock* Title = MakeText(WidgetTree, TEXT("ControlsTitle"), 18, KeyColor);
	SetBold(Title);
	Title->SetJustification(ETextJustify::Left);
	Title->SetText(DoNotSmokeLocale::Text(TEXT("CtrlTitle")));
	Rows->AddChildToVerticalBox(Title)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

	// Key column has a fixed width so the action texts line up
	static const TCHAR* const RowKeys[][2] = {
		{ TEXT("CtrlKeyPitch"), TEXT("CtrlPitch") },
		{ TEXT("CtrlKeyRoll"), TEXT("CtrlRoll") },
		{ TEXT("CtrlKeyAim"), TEXT("CtrlAim") },
		{ TEXT("CtrlKeyFire"), TEXT("CtrlFire") },
		{ TEXT("CtrlKeyBoost"), TEXT("CtrlBoost") },
	};
	for (const auto& RowKey : RowKeys)
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

		USizeBox* KeyBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		KeyBox->SetWidthOverride(160.0f);
		UTextBlock* KeyText = MakeText(WidgetTree, NAME_None, 15, KeyColor);
		SetBold(KeyText);
		KeyText->SetJustification(ETextJustify::Left);
		KeyText->SetText(DoNotSmokeLocale::Text(RowKey[0]));
		KeyBox->AddChild(KeyText);
		Row->AddChildToHorizontalBox(KeyBox);

		UTextBlock* ActionText = MakeText(WidgetTree, NAME_None, 15, ActionColor);
		ActionText->SetJustification(ETextJustify::Left);
		ActionText->SetText(DoNotSmokeLocale::Text(RowKey[1]));
		Row->AddChildToHorizontalBox(ActionText);

		Rows->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.0f, 2.0f));
	}

	UTextBlock* Hint = MakeText(WidgetTree, TEXT("ControlsHint"), 13, FLinearColor(0.7f, 0.85f, 1.0f, 1.0f));
	Hint->SetJustification(ETextJustify::Left);
	Hint->SetText(DoNotSmokeLocale::Text(TEXT("CtrlHint")));
	Rows->AddChildToVerticalBox(Hint)->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 0.0f));

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ControlsPanel"));
	Panel->SetBrushColor(FLinearColor(0.02f, 0.03f, 0.05f, 0.6f));
	Panel->SetPadding(FMargin(18.0f, 14.0f));
	Panel->SetContent(Rows);
	Panel->SetVisibility(ESlateVisibility::Collapsed);
	if (UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(Panel))
	{
		CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f));
		CanvasSlot->SetPosition(FVector2D(24.0f, 24.0f));
		CanvasSlot->SetAutoSize(true);
	}
	ControlsPanel = Panel;
}

void UDoNotSmokeFlightHUDWidget::StartCountdown()
{
	CountdownElapsed = 0.0f;
	bCountdownGoSent = false;
	ControlsPanelAge = -1.0f;
	if (ControlsPanel)
	{
		ControlsPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UDoNotSmokeFlightHUDWidget::TickCountdown(float DeltaTime)
{
	if (!CountdownText || CountdownElapsed < 0.0f)
	{
		return;
	}

	CountdownElapsed += DeltaTime;
	const float Step = FMath::Max(CountdownStepSeconds, 0.1f);
	const float GoStart = Step * 3.0f;

	if (CountdownElapsed >= GoStart + CountdownGoHoldSeconds)
	{
		CountdownText->SetVisibility(ESlateVisibility::Collapsed);
		CountdownElapsed = -1.0f;
		return;
	}

	FText Label;
	float LocalAlpha = 0.0f;
	float Opacity = 1.0f;
	if (CountdownElapsed < GoStart)
	{
		const int32 Index = FMath::FloorToInt(CountdownElapsed / Step);
		Label = FText::AsNumber(3 - Index);
		LocalAlpha = (CountdownElapsed - Index * Step) / Step;
		Opacity = 1.0f - FMath::Clamp((LocalAlpha - 0.75f) / 0.25f, 0.0f, 1.0f);
	}
	else
	{
		Label = DoNotSmokeLocale::Text(TEXT("CountdownGo"));
		LocalAlpha = (CountdownElapsed - GoStart) / FMath::Max(CountdownGoHoldSeconds, 0.1f);
		Opacity = 1.0f - FMath::Clamp((LocalAlpha - 0.6f) / 0.4f, 0.0f, 1.0f);

		if (!bCountdownGoSent)
		{
			bCountdownGoSent = true;
			ControlsPanelAge = 0.0f;
			if (ADoNotSmokeGameMode* GM = Cast<ADoNotSmokeGameMode>(UGameplayStatics::GetGameMode(this)))
			{
				GM->HandleCountdownFinished();
			}
		}
	}

	// Each label pops in large and settles within the first quarter of its step
	const float Pop = 1.0f - FMath::Pow(1.0f - FMath::Clamp(LocalAlpha / 0.25f, 0.0f, 1.0f), 3.0f);
	const float Scale = FMath::Lerp(1.7f, 1.0f, Pop);

	CountdownText->SetText(Label);
	CountdownText->SetRenderScale(FVector2D(Scale, Scale));
	CountdownText->SetRenderOpacity(Opacity);
	CountdownText->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UDoNotSmokeFlightHUDWidget::TickControlsPanel(float DeltaTime)
{
	if (!ControlsPanel || ControlsPanelAge < 0.0f)
	{
		return;
	}

	ControlsPanelAge += DeltaTime;
	const float FadeAlpha = (ControlsPanelAge - ControlsPanelSeconds) / FMath::Max(ControlsPanelFadeSeconds, 0.1f);
	if (FadeAlpha >= 1.0f)
	{
		ControlsPanel->SetVisibility(ESlateVisibility::Collapsed);
		ControlsPanelAge = -1.0f;
		return;
	}

	ControlsPanel->SetRenderOpacity(1.0f - FMath::Clamp(FadeAlpha, 0.0f, 1.0f));
	ControlsPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
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

	// Widgets tick on real time, so the countdown runs while the level is still paused
	TickCountdown(InDeltaTime);
	TickControlsPanel(InDeltaTime);

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

			const FString Msg = DoNotSmokeLocale::Format(TEXT("HudStatus"), {
				GM->GetOpenSmokestackCount(),
				GM->GetTotalSmokestackCount(),
				FString::Printf(TEXT("%.0f"), GM->GetSunPocketOpenRatio() * 100.0f),
				Stars }).ToString();
			if (Msg != LastStatusText)
			{
				LastStatusText = Msg;
				StacksAndCO2Text->SetText(FText::FromString(Msg));
			}
		}

		if (CarbonStatusText)
		{
			const FString CarbonMsg = DoNotSmokeLocale::Format(TEXT("HudCarbon"), {
				FString::Printf(TEXT("%.0f"), GM->GetCumulativeCarbonKg()),
				FString::Printf(TEXT("%.0f"), GM->GetTargetMinCarbonKg()) }).ToString();
			if (CarbonMsg != LastCarbonText)
			{
				LastCarbonText = CarbonMsg;
				CarbonStatusText->SetText(FText::FromString(CarbonMsg));
			}
		}

		// Blink the warning JetWarningBlinkCount times when a jet is dispatched (slogan stays on the jet's banner)
		const bool bJetAlert = GM->IsPrivateJetAlertActive();
		if (bJetAlert && !bWasJetAlertActive)
		{
			JetWarningRemaining = JetWarningDisplaySeconds;
		}
		bWasJetAlertActive = bJetAlert;
		JetWarningRemaining = FMath::Max(0.0f, JetWarningRemaining - InDeltaTime);

		// Each blink is on for 60% of its period, then off
		const float BlinkPeriod = JetWarningDisplaySeconds / FMath::Max(JetWarningBlinkCount, 1);
		const float JetWarningAge = JetWarningDisplaySeconds - JetWarningRemaining;
		const bool bJetBlinkOn = JetWarningRemaining > 0.0f && FMath::Fmod(JetWarningAge, BlinkPeriod) < BlinkPeriod * 0.6f;
		const ESlateVisibility JetVis = bJetBlinkOn ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
		if (JetWarningPanel) { JetWarningPanel->SetVisibility(JetVis); }
		if (JetWarningText) { JetWarningText->SetVisibility(JetVis); }
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
	const int32 Altitude = FMath::Max(0, FMath::RoundToInt(Glider->GetCurrentAltitude()));
	if (AltitudeText && Altitude != LastAltitudeShown)
	{
		LastAltitudeShown = Altitude;
		AltitudeText->SetText(FText::AsNumber(Altitude, &FNumberFormattingOptions::DefaultNoGrouping()));
	}

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
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(CrackOverlay->Slot))
	{
		CanvasSlot->SetOffsets(FMargin(0.0f));
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
	RestartText->SetText(DoNotSmokeLocale::Text(TEXT("CrashRestart")));
	RestartButton->AddChild(RestartText);

	if (UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(RestartButton))
	{
		CanvasSlot->SetAnchors(FAnchors(0.5f, 0.78f));
		CanvasSlot->SetAlignment(Centered);
		CanvasSlot->SetSize(FVector2D(440.0f, 110.0f));
	}

	bBuiltFallbackTree = true;
}

void UDoNotSmokeCrashWidget::SetCrashCause(EGliderCrashCause Cause)
{
	UTexture2D* Icon = GroundCrashTexture;
	const TCHAR* LabelKey = TEXT("CrashGround");

	switch (Cause)
	{
	case EGliderCrashCause::Smokestack:
		Icon = SmokestackCrashTexture;
		LabelKey = TEXT("CrashSmokestack");
		break;
	case EGliderCrashCause::ToxicPlume:
		Icon = ToxicPlumeCrashTexture;
		LabelKey = TEXT("CrashToxicPlume");
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
		CauseText->SetText(DoNotSmokeLocale::Text(LabelKey));
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
		if (UCanvasPanelSlot* CanvasSlot = Page->AddChildToCanvas(Widget))
		{
			CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f));
			CanvasSlot->SetPosition(Px(X0, Y0));
			CanvasSlot->SetSize(Px(X1 - X0, Y1 - Y0));
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

	if (RestartButton)
	{
		RestartButton->OnClicked.AddUniqueDynamic(this, &UDoNotSmokeSummaryWidget::HandleRestartClicked);
	}
}

void UDoNotSmokeSummaryWidget::HandleRestartClicked()
{
	if (ADoNotSmokeGameMode* GM = Cast<ADoNotSmokeGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GM->TriggerInstantRestart();
	}
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
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Backdrop->Slot))
	{
		CanvasSlot->SetOffsets(FMargin(0.0f));
	}

	// 2. Page container (the reveal spins this)
	PagePanel = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("PagePanel"));
	if (UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(PagePanel))
	{
		CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CanvasSlot->SetSize(PageSize);
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
	Masthead->SetText(DoNotSmokeLocale::Text(TEXT("PaperMasthead")));
	Place(PagePanel, MakeCentredBox(WidgetTree, TEXT("MastheadBox"), Masthead), 205.0f, 42.0f, 705.0f, 148.0f);

	UTextBlock* Edition = MakeText(WidgetTree, TEXT("EditionText"), 10, InkColor);
	Edition->SetText(DoNotSmokeLocale::Text(TEXT("PaperEdition")));
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
	if (UCanvasPanelSlot* CanvasSlot = PhotoClip->AddChildToCanvas(PhotoImage))
	{
		const float PhotoHeight = 648.0f * 1000.0f / 1600.0f;
		CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f));
		CanvasSlot->SetPosition(Px(0.0f, (233.0f - PhotoHeight) * 0.5f));
		CanvasSlot->SetSize(Px(648.0f, PhotoHeight));
	}

	// 6. Audit box (dashed frame): stars, CO2 vs C_min, time, accuracy
	UTextBlock* AuditTab = MakeText(WidgetTree, TEXT("AuditTabText"), 10, FLinearColor(0.95f, 0.93f, 0.88f, 1.0f));
	SetBold(AuditTab);
	AuditTab->SetText(DoNotSmokeLocale::Text(TEXT("PaperAudit")));
	Place(PagePanel, MakeCentredBox(WidgetTree, TEXT("AuditTabBox"), AuditTab), 344.0f, 604.0f, 410.0f, 632.0f);

	UVerticalBox* AuditList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("AuditList"));
	StarsText = MakeText(WidgetTree, TEXT("StarsText"), 34, FLinearColor(0.85f, 0.2f, 0.15f, 1.0f));
	if (UVerticalBoxSlot* CanvasSlot = AuditList->AddChildToVerticalBox(StarsText))
	{
		CanvasSlot->SetHorizontalAlignment(HAlign_Center);
		CanvasSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
	}

	UHorizontalBox* CarbonRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("CarbonRow"));
	UImage* CarbonIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("SummaryCarbonIcon"));
	CarbonIcon->SetBrushFromTexture(CarbonIconTexture, false);
	CarbonIcon->SetDesiredSizeOverride(FVector2D(34.0f, 34.0f));
	if (UHorizontalBoxSlot* CanvasSlot = CarbonRow->AddChildToHorizontalBox(CarbonIcon))
	{
		CanvasSlot->SetVerticalAlignment(VAlign_Center);
		CanvasSlot->SetPadding(FMargin(0.0f, 0.0f, 6.0f, 0.0f));
	}
	CarbonText = MakeText(WidgetTree, TEXT("CarbonText"), 15, InkColor);
	if (UHorizontalBoxSlot* CanvasSlot = CarbonRow->AddChildToHorizontalBox(CarbonText))
	{
		CanvasSlot->SetVerticalAlignment(VAlign_Center);
	}
	if (UVerticalBoxSlot* CanvasSlot = AuditList->AddChildToVerticalBox(CarbonRow))
	{
		CanvasSlot->SetHorizontalAlignment(HAlign_Center);
	}

	TimeText = MakeText(WidgetTree, TEXT("TimeText"), 15, InkColor);
	AuditList->AddChildToVerticalBox(TimeText)->SetHorizontalAlignment(HAlign_Center);
	AccuracyText = MakeText(WidgetTree, TEXT("AccuracyText"), 15, InkColor);
	AuditList->AddChildToVerticalBox(AccuracyText)->SetHorizontalAlignment(HAlign_Center);

	Place(PagePanel, MakeCentredBox(WidgetTree, TEXT("AuditBox"), AuditList), 352.0f, 638.0f, 738.0f, 852.0f);

	// 7. Footer strip: restart prompt
	UTextBlock* Footer = MakeText(WidgetTree, TEXT("FooterText"), 14, InkColor);
	SetBold(Footer);
	Footer->SetText(DoNotSmokeLocale::Text(TEXT("PaperFooter")));
	Place(PagePanel, MakeCentredBox(WidgetTree, TEXT("FooterBox"), Footer), 95.0f, 885.0f, 657.0f, 930.0f);

	// 8. Buttons beside the page: Restart, and Next Level (not wired until L2 exists)
	auto AddSideButton = [&](const FName Name, const FLinearColor& Color, const TCHAR* LabelKey, float OffsetY) -> UButton*
	{
		UTextBlock* Label = MakeText(WidgetTree, NAME_None, 24, FLinearColor(0.97f, 0.97f, 0.97f, 1.0f));
		SetBold(Label);
		Label->SetText(DoNotSmokeLocale::Text(LabelKey));
		UButton* Button = MakeFlatButton(WidgetTree, Name, Color, Label);
		if (UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(Button))
		{
			CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			CanvasSlot->SetAlignment(FVector2D(0.0f, 0.5f));
			CanvasSlot->SetPosition(FVector2D(PageSize.X * 0.5f + 48.0f, OffsetY));
			CanvasSlot->SetSize(FVector2D(320.0f, 72.0f));
		}
		return Button;
	};
	RestartButton = AddSideButton(TEXT("SummaryRestartButton"), FLinearColor(0.16f, 0.55f, 0.28f, 0.95f), TEXT("CrashRestart"), -48.0f);
	NextLevelButton = AddSideButton(TEXT("SummaryNextLevelButton"), FLinearColor(0.2f, 0.32f, 0.5f, 0.95f), TEXT("NextLevel"), 48.0f);

	// 9. Confetti layer above the page, centred so piece positions are offsets from screen centre
	ConfettiLayer = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("ConfettiLayer"));
	ConfettiLayer->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(ConfettiLayer))
	{
		CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CanvasSlot->SetSize(FVector2D::ZeroVector);
	}

	bBuiltFallbackTree = true;
}

void UDoNotSmokeSummaryWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Widgets tick on real time, so the reveal and confetti keep animating while the game is paused
	TickConfetti(InDeltaTime);

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
		CarbonText->SetText(DoNotSmokeLocale::Format(TEXT("PaperCarbon"), { FString::Printf(TEXT("%.0f"), InCarbonKg), FString::Printf(TEXT("%.0f"), InTargetCarbonKg) }));
	}
	if (TimeText)
	{
		TimeText->SetText(DoNotSmokeLocale::Format(TEXT("PaperTime"), { FString::Printf(TEXT("%.1f"), InTimeSeconds) }));
	}
	if (AccuracyText)
	{
		AccuracyText->SetText(DoNotSmokeLocale::Format(TEXT("PaperAccuracy"), { FString::Printf(TEXT("%.0f"), InAccuracyPercent) }));
	}
	if (PhotoImage && ValleyPhotoTextures.IsValidIndex(Stars - 1) && ValleyPhotoTextures[Stars - 1])
	{
		PhotoImage->SetBrushFromTexture(ValleyPhotoTextures[Stars - 1], false);
	}

	RevealElapsed = 0.0f;
	BurstConfetti();
}

void UDoNotSmokeSummaryWidget::BurstConfetti()
{
	if (!ConfettiLayer || !WidgetTree)
	{
		return;
	}

	ConfettiLayer->ClearChildren();
	ConfettiPieces.Reset();
	ConfettiAge = 0.0f;

	// Palette taken from the HUD art: alarm red, solar green, solar blue, sun yellow, paper white
	static const FLinearColor Palette[] = {
		FLinearColor(0.90f, 0.19f, 0.15f), FLinearColor(0.30f, 0.80f, 0.45f), FLinearColor(0.20f, 0.55f, 0.95f),
		FLinearColor(1.00f, 0.80f, 0.20f), FLinearColor(0.95f, 0.93f, 0.88f) };

	for (int32 i = 0; i < ConfettiCount; ++i)
	{
		UImage* Piece = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		Piece->SetColorAndOpacity(Palette[FMath::RandRange(0, UE_ARRAY_COUNT(Palette) - 1)]);
		if (UCanvasPanelSlot* CanvasSlot = ConfettiLayer->AddChildToCanvas(Piece))
		{
			CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			CanvasSlot->SetSize(FVector2D(FMath::FRandRange(8.0f, 14.0f), FMath::FRandRange(14.0f, 24.0f)));
		}

		// Fountain burst from just below the page centre, biased upward
		const float LaunchAngle = FMath::DegreesToRadians(FMath::FRandRange(-160.0f, -20.0f));
		const float Speed = FMath::FRandRange(700.0f, 1500.0f);

		FConfettiPiece& State = ConfettiPieces.AddDefaulted_GetRef();
		State.Image = Piece;
		State.Position = FVector2D(FMath::FRandRange(-60.0f, 60.0f), 120.0f);
		State.Velocity = FVector2D(FMath::Cos(LaunchAngle), FMath::Sin(LaunchAngle)) * Speed;
		State.Angle = FMath::FRandRange(0.0f, 360.0f);
		State.AngularSpeed = FMath::FRandRange(-540.0f, 540.0f);
		State.Flutter = FMath::FRandRange(0.0f, 2.0f * PI);
	}
}

void UDoNotSmokeSummaryWidget::TickConfetti(float DeltaTime)
{
	if (ConfettiPieces.Num() == 0)
	{
		return;
	}

	ConfettiAge += DeltaTime;
	if (ConfettiAge >= ConfettiLifetime)
	{
		if (ConfettiLayer)
		{
			ConfettiLayer->ClearChildren();
		}
		ConfettiPieces.Reset();
		return;
	}

	// Paper-like fall: gravity, strong air drag and a sideways flutter; fade out over the last second
	const float Opacity = FMath::Clamp(ConfettiLifetime - ConfettiAge, 0.0f, 1.0f);
	const float Drag = FMath::Exp(-1.6f * DeltaTime);
	for (FConfettiPiece& State : ConfettiPieces)
	{
		UImage* Piece = State.Image.Get();
		if (!Piece)
		{
			continue;
		}

		State.Velocity = State.Velocity * Drag + FVector2D(0.0f, 900.0f * DeltaTime);
		State.Flutter += DeltaTime * 6.0f;
		State.Position += (State.Velocity + FVector2D(FMath::Sin(State.Flutter) * 60.0f, 0.0f)) * DeltaTime;
		State.Angle += State.AngularSpeed * DeltaTime;

		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Piece->Slot))
		{
			CanvasSlot->SetPosition(State.Position);
		}
		Piece->SetRenderTransformAngle(State.Angle);
		Piece->SetRenderOpacity(Opacity);
	}
}

// ==========================================
// UDoNotSmokeMainMenuWidget Implementation
// ==========================================

UDoNotSmokeMainMenuWidget::UDoNotSmokeMainMenuWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	using namespace DoNotSmokeUI;
	ProhibitionTexture = FindUIAsset<UTexture2D>(TEXT("/Game/Game/UI/Textures/T_UI_Prohibition.T_UI_Prohibition"));
	StackPuffTexture = FindUIAsset<UTexture2D>(TEXT("/Game/Game/UI/Textures/T_UI_StackPuff.T_UI_StackPuff"));

	SetIsFocusable(true);
}

void UDoNotSmokeMainMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	BuildFallbackWidgetTree();

	if (PlayButton)
	{
		PlayButton->OnClicked.AddUniqueDynamic(this, &UDoNotSmokeMainMenuWidget::HandlePlayClicked);
	}
	if (LanguageButton)
	{
		LanguageButton->OnClicked.AddUniqueDynamic(this, &UDoNotSmokeMainMenuWidget::HandleLanguageClicked);
	}
	if (QuitButton)
	{
		QuitButton->OnClicked.AddUniqueDynamic(this, &UDoNotSmokeMainMenuWidget::HandleQuitClicked);
	}
}

TSharedRef<SWidget> UDoNotSmokeMainMenuWidget::RebuildWidget()
{
	BuildFallbackWidgetTree();
	return Super::RebuildWidget();
}

void UDoNotSmokeMainMenuWidget::BuildFallbackWidgetTree()
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

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("MenuRootCanvas"));
	WidgetTree->RootWidget = RootCanvas;

	const FVector2D Centered(0.5f, 0.5f);

	// 1. Dim the paused valley behind the menu
	UImage* Backdrop = AddImage(WidgetTree, RootCanvas, TEXT("MenuBackdrop"), nullptr, FAnchors(0.0f, 0.0f, 1.0f, 1.0f), FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D::ZeroVector);
	Backdrop->SetColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.45f));
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Backdrop->Slot))
	{
		CanvasSlot->SetOffsets(FMargin(0.0f));
	}

	// 2. Logo: chimney under the prohibition sign, as on the newspaper masthead
	const FAnchors LogoAnchor(0.5f, 0.18f);
	AddImage(WidgetTree, RootCanvas, TEXT("MenuLogoStack"), StackPuffTexture, LogoAnchor, Centered, FVector2D(0.0f, 8.0f), FVector2D(170.0f, 184.0f));
	AddImage(WidgetTree, RootCanvas, TEXT("MenuLogoSign"), ProhibitionTexture, LogoAnchor, Centered, FVector2D::ZeroVector, FVector2D(220.0f, 220.0f));

	// 3. Title + tagline
	UTextBlock* Title = MakeText(WidgetTree, TEXT("MenuTitle"), 76, FLinearColor(0.97f, 0.95f, 0.9f, 1.0f));
	SetBold(Title);
	Title->SetText(FText::FromString(TEXT("DO NOT SMOKE")));
	AddText(RootCanvas, Title, FAnchors(0.5f, 0.38f), FVector2D::ZeroVector);

	TaglineText = MakeText(WidgetTree, TEXT("MenuTagline"), 22, FLinearColor(1.0f, 0.86f, 0.25f, 1.0f));
	AddText(RootCanvas, TaglineText, FAnchors(0.5f, 0.38f), FVector2D(0.0f, 72.0f));

	// 4. Buttons
	// Buttons share one anchor and step down by a fixed pixel gap, so they never overlap on short viewports
	const FAnchors ButtonAnchor(0.5f, 0.55f);
	auto AddMenuButton = [&](const FName Name, const FLinearColor& Color, int32 Row, TObjectPtr<UTextBlock>& OutLabel) -> UButton*
	{
		OutLabel = MakeText(WidgetTree, NAME_None, 28, FLinearColor(0.97f, 0.97f, 0.97f, 1.0f));
		SetBold(OutLabel);
		UButton* Button = MakeFlatButton(WidgetTree, Name, Color, OutLabel);
		if (UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(Button))
		{
			CanvasSlot->SetAnchors(ButtonAnchor);
			CanvasSlot->SetAlignment(Centered);
			CanvasSlot->SetPosition(FVector2D(0.0f, Row * 96.0f));
			CanvasSlot->SetSize(FVector2D(400.0f, 76.0f));
		}
		return Button;
	};

	PlayButton = AddMenuButton(TEXT("PlayButton"), FLinearColor(0.16f, 0.55f, 0.28f, 0.95f), 0, PlayText);
	QuitButton = AddMenuButton(TEXT("QuitButton"), FLinearColor(0.55f, 0.17f, 0.15f, 0.95f), 1, QuitText);

	// 5. Language square in the bottom-right corner: each press cycles EN -> TR -> NL
	LanguageText = MakeText(WidgetTree, TEXT("LanguageText"), 30, FLinearColor(0.97f, 0.97f, 0.97f, 1.0f));
	SetBold(LanguageText);
	LanguageButton = MakeFlatButton(WidgetTree, TEXT("LanguageButton"), FLinearColor(0.2f, 0.32f, 0.5f, 0.95f), LanguageText);
	if (UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(LanguageButton))
	{
		CanvasSlot->SetAnchors(FAnchors(1.0f, 1.0f));
		CanvasSlot->SetAlignment(FVector2D(1.0f, 1.0f));
		CanvasSlot->SetPosition(FVector2D(-32.0f, -32.0f));
		CanvasSlot->SetSize(FVector2D(88.0f, 88.0f));
	}

	RefreshTexts();
	bBuiltFallbackTree = true;
}

void UDoNotSmokeMainMenuWidget::RefreshTexts()
{
	if (TaglineText) { TaglineText->SetText(DoNotSmokeLocale::Text(TEXT("MenuTagline"))); }
	if (PlayText) { PlayText->SetText(DoNotSmokeLocale::Text(TEXT("MenuPlay"))); }
	if (LanguageText) { LanguageText->SetText(DoNotSmokeLocale::Text(TEXT("MenuLanguage"))); }
	if (QuitText) { QuitText->SetText(DoNotSmokeLocale::Text(TEXT("MenuQuit"))); }
}

FReply UDoNotSmokeMainMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Enter || Key == EKeys::SpaceBar)
	{
		HandlePlayClicked();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UDoNotSmokeMainMenuWidget::HandlePlayClicked()
{
	if (ADoNotSmokeGameMode* GM = Cast<ADoNotSmokeGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GM->StartFlightFromMenu();
	}
}

void UDoNotSmokeMainMenuWidget::HandleLanguageClicked()
{
	DoNotSmokeLocale::CycleLanguage();
	RefreshTexts();
}

void UDoNotSmokeMainMenuWidget::HandleQuitClicked()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
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
	MainMenuWidgetClass = UDoNotSmokeMainMenuWidget::StaticClass();

	static ConstructorHelpers::FObjectFinder<USoundBase> LevelMusicAsset(TEXT("/Game/Game/Audio/MUS_Level01.MUS_Level01"));
	if (LevelMusicAsset.Succeeded())
	{
		LevelMusic = LevelMusicAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> SunRainAsset(TEXT("/Game/Game/VFX/NS_SunMotes.NS_SunMotes"));
	if (SunRainAsset.Succeeded())
	{
		SunRainSystem = SunRainAsset.Object;
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
	// The first directional light is the sun, dimmed as carbon accumulates
	TActorIterator<ADirectionalLight> SunIt(World);
	if (SunIt)
	{
		SunLight = *SunIt;
		BaseSunIntensity = SunIt->GetLightComponent() ? SunIt->GetLightComponent()->Intensity : 0.0f;
	}
	ApplySkyChoke(0.0f);

	// Jets no longer self-schedule: the dispatcher sends one random route at a time
	TrackedJets.Empty();
	for (TActorIterator<APrivateJet> It(World); It; ++It)
	{
		It->OnJetApproachStateChanged.AddDynamic(this, &ADoNotSmokeGameMode::HandlePrivateJetApproachChanged);
		It->SetAutoSchedulePasses(false);
		TrackedJets.Add(*It);
	}
	JetDispatchTimer = FirstJetDelaySeconds;

	ApplyLevelBorders();
	BuildSunRainGrid();

	// Bind to player GliderPawn events for shot accounting and Crash restart
	if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0))
	{
		if (AGliderPawn* Glider = Cast<AGliderPawn>(PlayerPawn))
		{
			Glider->OnCarbonSealFired.AddDynamic(this, &ADoNotSmokeGameMode::NotifyCarbonSealFired);
			Glider->OnGliderCrashed.AddDynamic(this, &ADoNotSmokeGameMode::HandleGliderCrashed);
		}
	}

	// "COP" cheat: the GameMode listens to C, O, P itself so the glider stays free of debug input
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0))
	{
		EnableInput(PC);
		if (InputComponent)
		{
			InputComponent->BindKey(EKeys::C, IE_Pressed, this, &ADoNotSmokeGameMode::HandleCheatKeyC);
			InputComponent->BindKey(EKeys::O, IE_Pressed, this, &ADoNotSmokeGameMode::HandleCheatKeyO);
			InputComponent->BindKey(EKeys::P, IE_Pressed, this, &ADoNotSmokeGameMode::HandleCheatKeyP);
		}
	}

	if (LevelMusic)
	{
		LevelMusicAudio = UGameplayStatics::CreateSound2D(this, LevelMusic, LevelMusicVolume);
		if (LevelMusicAudio)
		{
			LevelMusicAudio->bIsUISound = true;
			LevelMusicAudio->OnAudioFinished.AddDynamic(this, &ADoNotSmokeGameMode::HandleLevelMusicFinished);
			LevelMusicAudio->Play();
		}
	}

	// The level waits frozen behind the title screen; a restart skips straight to the countdown
	bFlightStarted = false;
	World->GetTimerManager().SetTimer(PreFlightFreezeTimerHandle, this, &ADoNotSmokeGameMode::FreezeForPreFlight, 0.1f, false);
	if (UGameplayStatics::HasOption(OptionsString, TEXT("SkipMenu")))
	{
		BeginCountdown();
	}
	else
	{
		ShowMainMenu();
	}
}

void ADoNotSmokeGameMode::ShowMainMenu()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	TSubclassOf<UDoNotSmokeMainMenuWidget> TargetMenuClass = MainMenuWidgetClass ? MainMenuWidgetClass : TSubclassOf<UDoNotSmokeMainMenuWidget>(UDoNotSmokeMainMenuWidget::StaticClass());
	MainMenuWidget = PC ? CreateWidget<UDoNotSmokeMainMenuWidget>(PC, TargetMenuClass) : nullptr;
	if (!MainMenuWidget)
	{
		BeginCountdown();
		return;
	}

	MainMenuWidget->AddToViewport(40);

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(MainMenuWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PC->SetInputMode(InputMode);
	PC->SetShowMouseCursor(true);
}

void ADoNotSmokeGameMode::StartFlightFromMenu()
{
	if (MainMenuWidget)
	{
		MainMenuWidget->RemoveFromParent();
		MainMenuWidget = nullptr;
	}
	BeginCountdown();
}

void ADoNotSmokeGameMode::BeginCountdown()
{
	// The HUD is created only now so its texts use the language picked on the title screen
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC)
	{
		HandleCountdownFinished();
		return;
	}

	if (!FlightHUDWidget)
	{
		TSubclassOf<UDoNotSmokeFlightHUDWidget> TargetHUDClass = FlightHUDWidgetClass ? FlightHUDWidgetClass : TSubclassOf<UDoNotSmokeFlightHUDWidget>(UDoNotSmokeFlightHUDWidget::StaticClass());
		FlightHUDWidget = CreateWidget<UDoNotSmokeFlightHUDWidget>(PC, TargetHUDClass);
		if (FlightHUDWidget)
		{
			FlightHUDWidget->AddToViewport(10);
		}
	}

	PC->SetInputMode(FInputModeGameOnly());
	PC->SetShowMouseCursor(false);

	if (FlightHUDWidget)
	{
		FlightHUDWidget->StartCountdown();
	}
	else
	{
		HandleCountdownFinished();
	}
}

void ADoNotSmokeGameMode::FreezeForPreFlight()
{
	if (!bFlightStarted)
	{
		if (AGliderPawn* Glider = Cast<AGliderPawn>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0)))
		{
			Glider->SetMotionBlurSuppressed(true);
		}
		UGameplayStatics::SetGamePaused(GetWorld(), true);
	}
}

void ADoNotSmokeGameMode::ApplyLevelBorders()
{
	FBox2D Area(ForceInit);
	TArray<AActor*> Borders;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), LevelBorderTag, Borders);
	for (AActor* Border : Borders)
	{
		Area += FVector2D(Border->GetActorLocation());
		Border->SetActorHiddenInGame(true);
		Border->SetActorEnableCollision(false);
	}

	// Two or more corners are needed to span an area
	if (Borders.Num() < 2 || !Area.bIsValid)
	{
		return;
	}

	PlayArea = Area;
	const FBox2D WallArea = Area.ExpandBy(FlightWallMargin);
	if (AGliderPawn* Glider = Cast<AGliderPawn>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0)))
	{
		Glider->SetFlightBounds(WallArea);
	}
	for (const TWeakObjectPtr<APrivateJet>& WeakJet : TrackedJets)
	{
		if (APrivateJet* Jet = WeakJet.Get())
		{
			Jet->SetSmokeArea(WallArea);
		}
	}
}

void ADoNotSmokeGameMode::HandleCountdownFinished()
{
	bFlightStarted = true;
	if (AGliderPawn* Glider = Cast<AGliderPawn>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0)))
	{
		Glider->SetMotionBlurSuppressed(false);
	}
	GetWorldTimerManager().ClearTimer(PreFlightFreezeTimerHandle);
	UGameplayStatics::SetGamePaused(GetWorld(), false);
}

void ADoNotSmokeGameMode::HandleLevelMusicFinished()
{
	// Keeps the track going even if the imported wave is not flagged as looping
	if (LevelMusicAudio)
	{
		LevelMusicAudio->Play();
	}
}

void ADoNotSmokeGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bFlightStarted || bIsLevelCompleted || bHasGliderCrashed)
	{
		return;
	}

	ElapsedLevelSeconds += DeltaTime;
	TickJetDispatcher(DeltaTime);

	if (OpenSmokestackCount > 0)
	{
		CumulativeCarbonKg += static_cast<float>(OpenSmokestackCount) * CarbonEmissionRatePerStack * DeltaTime;

		const float MaxChokeCarbon = FMath::Max(100.0f, TargetMinCarbonKg * SkyChokeCarbonMultiplier);
		const float ChokeAlpha = FMath::Clamp(CumulativeCarbonKg / MaxChokeCarbon, 0.0f, 1.0f);
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
		return DoNotSmokeLocale::Text(TEXT("Headline3"));
	case 2:
		return DoNotSmokeLocale::Text(TEXT("Headline2"));
	default:
		return DoNotSmokeLocale::Text(TEXT("Headline1"));
	}
}

void ADoNotSmokeGameMode::NotifyCarbonSealFired()
{
	++TotalShotsFired;
}

void ADoNotSmokeGameMode::BuildSunRainGrid()
{
	UWorld* World = GetWorld();
	if (!World || !SunRainSystem || SunRainCellSize <= 0.0f)
	{
		return;
	}

	// Cover the smokestacks' footprint plus a margin
	FBox2D Bounds(ForceInit);
	for (const TWeakObjectPtr<ASmokestack>& WeakStack : TrackedSmokestacks)
	{
		if (const ASmokestack* Stack = WeakStack.Get())
		{
			Bounds += FVector2D(Stack->GetActorLocation());
		}
	}
	if (!Bounds.bIsValid)
	{
		return;
	}
	Bounds = Bounds.ExpandBy(SunRainMargin);

	const int32 CellsX = FMath::CeilToInt(Bounds.GetSize().X / SunRainCellSize);
	const int32 CellsY = FMath::CeilToInt(Bounds.GetSize().Y / SunRainCellSize);
	for (int32 IX = 0; IX < CellsX; ++IX)
	{
		for (int32 IY = 0; IY < CellsY; ++IY)
		{
			// Spawned at the smoke ceiling; motes fall to the ground under the system's gravity
			const FVector CellCentre(Bounds.Min.X + (IX + 0.5f) * SunRainCellSize, Bounds.Min.Y + (IY + 0.5f) * SunRainCellSize, 8000.0f);
			UNiagaraComponent* Cell = UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, SunRainSystem, CellCentre, FRotator::ZeroRotator,
				FVector::OneVector, false, true, ENCPoolMethod::None);
			if (Cell)
			{
				// NS_SunMotes reuses NS_SkySmog's user parameters: cylinder radius, spawn rate, sprite size multiplier
				Cell->SetVariableFloat(TEXT("SpreadRadius"), SunRainCellSize * 0.6f);
				Cell->SetVariableFloat(TEXT("SpawnRate"), SunRainSpawnRate);
				Cell->SetVariableFloat(TEXT("SizeScale"), SunRainSizeScale);
				SunRainCells.Add(Cell);
			}
		}
	}

	UpdateSunRainCells();
	World->GetTimerManager().SetTimer(SunRainTimerHandle, this, &ADoNotSmokeGameMode::UpdateSunRainCells, SunRainCheckInterval, true);
}

void ADoNotSmokeGameMode::UpdateSunRainCells()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Same test as the glider's sunlight trace: a cell rains light only while no smoke canopy or jet smoke is above it.
	// The open share of the cells inside the play area is the HUD's SUN percentage.
	int32 PlayCells = 0;
	int32 OpenPlayCells = 0;
	for (UNiagaraComponent* Cell : SunRainCells)
	{
		if (!Cell)
		{
			continue;
		}

		const FVector Centre = Cell->GetComponentLocation();
		FHitResult Hit;
		bool bCovered = World->LineTraceSingleByChannel(Hit, FVector(Centre.X, Centre.Y, 3000.0f), FVector(Centre.X, Centre.Y, 8500.0f), ECC_Visibility);
		for (const TWeakObjectPtr<APrivateJet>& WeakJet : TrackedJets)
		{
			const APrivateJet* Jet = WeakJet.Get();
			bCovered = bCovered || (Jet && Jet->IsShadedBySmoke(FVector(Centre.X, Centre.Y, 3000.0f)));
		}

		if (!PlayArea.bIsValid || PlayArea.IsInside(FVector2D(Centre)))
		{
			++PlayCells;
			OpenPlayCells += bCovered ? 0 : 1;
		}

		if (bCovered && Cell->IsActive())
		{
			Cell->Deactivate();
		}
		else if (!bCovered && !Cell->IsActive())
		{
			Cell->Activate(true);
		}
	}

	if (!bIsLevelCompleted && PlayCells > 0)
	{
		SunPocketOpenRatio = static_cast<float>(OpenPlayCells) / static_cast<float>(PlayCells);
	}
}

void ADoNotSmokeGameMode::TickJetDispatcher(float DeltaTime)
{
	if (TrackedJets.Num() == 0)
	{
		return;
	}

	JetDispatchTimer -= DeltaTime;
	if (JetDispatchTimer > 0.0f)
	{
		return;
	}

	TArray<APrivateJet*> IdleJets;
	for (const TWeakObjectPtr<APrivateJet>& WeakJet : TrackedJets)
	{
		APrivateJet* Jet = WeakJet.Get();
		if (!Jet)
		{
			continue;
		}
		if (Jet->IsJetActiveOrApproaching())
		{
			// A pass is still in the air: check again shortly instead of stacking a second jet
			JetDispatchTimer = 1.0f;
			return;
		}
		IdleJets.Add(Jet);
	}

	if (IdleJets.Num() > 0)
	{
		IdleJets[FMath::RandRange(0, IdleJets.Num() - 1)]->RequestFlybyPass();
	}
	JetDispatchTimer = JetDispatchIntervalSeconds;
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
}

void ADoNotSmokeGameMode::DebugSealNearestStack()
{
#if !UE_BUILD_SHIPPING
	APawn* Glider = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (!Glider)
	{
		return;
	}

	ASmokestack* Nearest = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();
	for (const TWeakObjectPtr<ASmokestack>& WeakStack : TrackedSmokestacks)
	{
		ASmokestack* Stack = WeakStack.Get();
		if (Stack && !Stack->IsSealed())
		{
			const float DistSq = FVector::DistSquared(Stack->GetActorLocation(), Glider->GetActorLocation());
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				Nearest = Stack;
			}
		}
	}

	// One TrySealTarget call seals one mouth; repeat for clustered stacks
	for (int32 Attempt = 0; Nearest && !Nearest->IsSealed() && Attempt < 3; ++Attempt)
	{
		IBPI_SealableTarget::Execute_TrySealTarget(Nearest, Glider);
	}
#endif
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

void ADoNotSmokeGameMode::HandleCheatKey(TCHAR Letter)
{
	// The letters must come in order, each within 1.5 s of the previous one
	const float Now = GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.0f;
	if (Now - LastCheatKeyTime > 1.5f)
	{
		CheatBuffer.Reset();
	}
	LastCheatKeyTime = Now;

	CheatBuffer.AppendChar(Letter);
	if (!FString(TEXT("COP")).StartsWith(CheatBuffer))
	{
		CheatBuffer = (Letter == TEXT('C')) ? FString(TEXT("C")) : FString();
	}
	else if (CheatBuffer == TEXT("COP"))
	{
		CheatBuffer.Reset();
		CheatFinishLevel();
	}
}

void ADoNotSmokeGameMode::CheatFinishLevel()
{
	if (!bFlightStarted || bIsLevelCompleted || bHasGliderCrashed)
	{
		return;
	}

	// Treat the next seal as the last open mouth; the summary then shows whatever the run has so far
	OpenSmokestackCount = 1;
	TotalSmokestackCount = FMath::Max(TotalSmokestackCount, 1);
	HandleSmokestackSealed(nullptr);
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

		UGameplayStatics::SetGamePaused(World, false);
		const FString CurrentMapName = UGameplayStatics::GetCurrentLevelName(World, true);
		UGameplayStatics::OpenLevel(World, FName(*CurrentMapName), true, TEXT("SkipMenu"));
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

		// Freeze the world behind the newspaper (glider, jets, timers); UI keeps ticking on real time
		UGameplayStatics::SetGamePaused(GetWorld(), true);

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

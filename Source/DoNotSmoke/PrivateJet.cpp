// Copyright Do Not Smoke Team. All Rights Reserved.

#include "PrivateJet.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/AudioComponent.h"
#include "DoNotSmokeLocale.h"
#include "Sound/SoundBase.h"
#include "Engine/StaticMesh.h"
#include "NiagaraComponent.h"
#include "UObject/ConstructorHelpers.h"

APrivateJet::APrivateJet()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	FlightSpline = CreateDefaultSubobject<USplineComponent>(TEXT("FlightSpline"));
	FlightSpline->SetupAttachment(SceneRoot);
	FlightSpline->ClearSplinePoints(false);
	FlightSpline->AddSplinePoint(FVector(-12000.0f, 0.0f, 0.0f), ESplineCoordinateSpace::Local, false);
	FlightSpline->AddSplinePoint(FVector(12000.0f, 0.0f, 0.0f), ESplineCoordinateSpace::Local, true);

	MovingJetCarriage = CreateDefaultSubobject<USceneComponent>(TEXT("MovingJetCarriage"));
	MovingJetCarriage->SetupAttachment(SceneRoot);

	JetBodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("JetBodyMesh"));
	JetBodyMesh->SetupAttachment(MovingJetCarriage);
	JetBodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	JetBodyMesh->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	JetBodyMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));

	JetBannerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("JetBannerMesh"));
	JetBannerMesh->SetupAttachment(MovingJetCarriage);
	JetBannerMesh->SetRelativeLocation(FVector(-950.0f, 0.0f, 0.0f));
	JetBannerMesh->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
	JetBannerMesh->SetRelativeScale3D(FVector(2.2f, 8.0f, 1.0f));
	JetBannerMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	BannerTextRender = CreateDefaultSubobject<UTextRenderComponent>(TEXT("BannerTextRender"));
	BannerTextRender->SetupAttachment(MovingJetCarriage);
	BannerTextRender->SetRelativeLocation(FVector(-950.0f, 30.0f, -40.0f));
	BannerTextRender->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
	BannerTextRender->SetHorizontalAlignment(EHTA_Center);
	BannerTextRender->SetVerticalAlignment(EVRTA_TextCenter);
	BannerTextRender->SetWorldSize(72.0f);
	BannerTextRender->SetTextRenderColor(FColor::Yellow);

	// Wake turbulence box spans from jet altitude (8400u) down to chimney top height (1400u) relative to the jet
	WakeTurbulenceBox = CreateDefaultSubobject<UBoxComponent>(TEXT("WakeTurbulenceBox"));
	WakeTurbulenceBox->SetupAttachment(MovingJetCarriage);
	WakeTurbulenceBox->SetRelativeLocation(WakeBoxOffset);
	WakeTurbulenceBox->InitBoxExtent(WakeBoxExtent);
	WakeTurbulenceBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WakeTurbulenceBox->SetCollisionResponseToAllChannels(ECR_Overlap);
	WakeTurbulenceBox->SetGenerateOverlapEvents(true);
	WakeTurbulenceBox->SetHiddenInGame(true);

	JetWakeVFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("JetWakeVFX"));
	JetWakeVFX->SetupAttachment(MovingJetCarriage);
	JetWakeVFX->bAutoActivate = false;

	// Default greybox meshes so the jet and banner are visible immediately before custom 3D assets are assigned
	static ConstructorHelpers::FObjectFinder<UStaticMesh> JetMeshAsset(TEXT("/Game/Game/Meshes/SM_PrivateJet/StaticMeshes/SM_PrivateJet.SM_PrivateJet"));
	if (JetMeshAsset.Succeeded())
	{
		JetBodyMesh->SetStaticMesh(JetMeshAsset.Object);
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMeshAsset(TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (PlaneMeshAsset.Succeeded())
	{
		JetBannerMesh->SetStaticMesh(PlaneMeshAsset.Object);
	}

	JetRoarAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("JetRoarAudio"));
	JetRoarAudio->SetupAttachment(MovingJetCarriage);
	JetRoarAudio->bAutoActivate = false;
	// The jet flies ~7 km above the glider, so the falloff is long enough to hear it from the flight band
	JetRoarAudio->bOverrideAttenuation = true;
	JetRoarAudio->AttenuationOverrides.bAttenuate = true;
	JetRoarAudio->AttenuationOverrides.bSpatialize = true;
	JetRoarAudio->AttenuationOverrides.AttenuationShapeExtents = FVector(6000.0f);
	JetRoarAudio->AttenuationOverrides.FalloffDistance = 20000.0f;
	static ConstructorHelpers::FObjectFinder<USoundBase> JetRoarAsset(TEXT("/Game/Game/Audio/SFX_Jet_Roar.SFX_Jet_Roar"));
	if (JetRoarAsset.Succeeded())
	{
		JetRoarAudio->SetSound(JetRoarAsset.Object);
	}
}

void APrivateJet::BeginPlay()
{
	Super::BeginPlay();

	// Stratospheric minimum altitude safety enforcement (JetAltitude, above the 7800u glider ceiling)
	if (GetActorLocation().Z < JetAltitude)
	{
		FVector CorrectedLoc = GetActorLocation();
		CorrectedLoc.Z = JetAltitude;
		SetActorLocation(CorrectedLoc);
	}

	if (WakeTurbulenceBox)
	{
		WakeTurbulenceBox->SetRelativeLocation(WakeBoxOffset);
		WakeTurbulenceBox->SetBoxExtent(WakeBoxExtent);
	}

	FinishFlybyPass();
	// ASSUMPTION: First pass starts quicker (6.0s after level start) so the player experiences the jet early.
	WaitTimer = FMath::Min(6.0f, PassIntervalSeconds);
}

bool APrivateJet::RequestFlybyPass()
{
	if (IsJetActiveOrApproaching())
	{
		return false;
	}

	BeginApproachWarning();
	return true;
}

void APrivateJet::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsApproaching && !bIsFlyingPass)
	{
		if (bAutoSchedulePasses)
		{
			WaitTimer -= DeltaTime;
			if (WaitTimer <= 0.0f)
			{
				BeginApproachWarning();
			}
		}
		return;
	}

	if (bIsApproaching)
	{
		WaitTimer -= DeltaTime;
		if (WaitTimer <= 0.0f)
		{
			StartFlybyPass();
		}
		return;
	}

	if (bIsFlyingPass && FlightSpline && MovingJetCarriage)
	{
		CurrentSplineDistance += JetFlightSpeed * DeltaTime;
		const float TotalLength = FlightSpline->GetSplineLength();

		const FVector NewWorldLoc = FlightSpline->GetLocationAtDistanceAlongSpline(CurrentSplineDistance, ESplineCoordinateSpace::World);
		const FRotator NewWorldRot = FlightSpline->GetRotationAtDistanceAlongSpline(CurrentSplineDistance, ESplineCoordinateSpace::World);

		MovingJetCarriage->SetWorldLocationAndRotation(NewWorldLoc, NewWorldRot);

		if (CurrentSplineDistance >= TotalLength)
		{
			FinishFlybyPass();
		}
	}
}

FText APrivateJet::GetCurrentBannerSlogan() const
{
	// Satirical COP31 greenwashing slogans (GDD Sec 5), looked up per pass so a language change applies at once
	const int32 Index = FMath::Clamp(CurrentSloganIndex, 0, DoNotSmokeLocale::JetSloganCount - 1);
	return DoNotSmokeLocale::Text(*FString::Printf(TEXT("JetSlogan%d"), Index));
}

bool APrivateJet::IsLethalCrashHazard_Implementation(const UPrimitiveComponent* HazardComponent) const
{
	// Stratospheric VIP Jet never causes direct pawn-to-mesh lethal crashes; only wake downwash
	return false;
}

float APrivateJet::GetWakeDownwashSpeed_Implementation(const UPrimitiveComponent* HazardComponent) const
{
	if (bIsFlyingPass && HazardComponent == WakeTurbulenceBox)
	{
		return WakeDownwashSpeed;
	}
	return 0.0f;
}

void APrivateJet::BeginApproachWarning()
{
	bIsApproaching = true;
	bIsFlyingPass = false;
	WaitTimer = ApproachWarningLeadSeconds;

	const FText ActiveSlogan = GetCurrentBannerSlogan();
	CurrentSloganIndex = (CurrentSloganIndex + 1) % DoNotSmokeLocale::JetSloganCount;

	if (BannerTextRender)
	{
		BannerTextRender->SetText(ActiveSlogan);
	}

	OnJetApproachStateChanged.Broadcast(true, ActiveSlogan);
	BP_OnJetApproachWarning(ActiveSlogan);
}

void APrivateJet::StartFlybyPass()
{
	bIsApproaching = false;
	bIsFlyingPass = true;
	CurrentSplineDistance = 0.0f;

	if (MovingJetCarriage)
	{
		// Snap to spline start first so the jet and wake do not render one frame at the previous position
		if (FlightSpline)
		{
			MovingJetCarriage->SetWorldLocationAndRotation(
				FlightSpline->GetLocationAtDistanceAlongSpline(0.0f, ESplineCoordinateSpace::World),
				FlightSpline->GetRotationAtDistanceAlongSpline(0.0f, ESplineCoordinateSpace::World));
		}
		MovingJetCarriage->SetVisibility(true, true);
	}

	if (WakeTurbulenceBox)
	{
		WakeTurbulenceBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}

	if (JetWakeVFX)
	{
		JetWakeVFX->Activate(true);
	}

	if (JetRoarAudio)
	{
		JetRoarAudio->Play();
	}
}

void APrivateJet::FinishFlybyPass()
{
	bIsApproaching = false;
	bIsFlyingPass = false;
	CurrentSplineDistance = 0.0f;
	WaitTimer = PassIntervalSeconds;

	if (MovingJetCarriage)
	{
		MovingJetCarriage->SetVisibility(false, true);
	}

	if (WakeTurbulenceBox)
	{
		WakeTurbulenceBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (JetWakeVFX)
	{
		JetWakeVFX->Deactivate();
	}

	OnJetApproachStateChanged.Broadcast(false, FText::GetEmpty());
}

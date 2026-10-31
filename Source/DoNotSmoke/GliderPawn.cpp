// Copyright Do Not Smoke Team. All Rights Reserved.

#include "GliderPawn.h"
#include "CarbonSealProjectile.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const TCHAR* const SolarGliderMeshPath = TEXT("/Game/Game/Meshes/SM_SolarGlider_V2/SM_SolarGlider/StaticMeshes/SM_SolarGlider.SM_SolarGlider");
}

AGliderPawn::AGliderPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	AutoPossessPlayer = EAutoReceiveInput::Player0;

	GliderCollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("GliderCollisionSphere"));
	GliderCollisionSphere->InitSphereRadius(90.0f);
	GliderCollisionSphere->SetCollisionProfileName(TEXT("Pawn"));
	GliderCollisionSphere->SetGenerateOverlapEvents(true);
	GliderCollisionSphere->SetNotifyRigidBodyCollision(true);
	SetRootComponent(GliderCollisionSphere);

	GliderMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("GliderMesh"));
	GliderMesh->SetupAttachment(GliderCollisionSphere);
	GliderMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	GreyboxGliderMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GreyboxGliderMesh"));
	GreyboxGliderMesh->SetupAttachment(GliderMesh);
	GreyboxGliderMesh->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	GreyboxGliderMesh->SetRelativeScale3D(FVector(0.42f, 0.42f, 0.42f));
	GreyboxGliderMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SolarGliderMeshAsset(SolarGliderMeshPath);
	if (SolarGliderMeshAsset.Succeeded())
	{
		GreyboxGliderMesh->SetStaticMesh(SolarGliderMeshAsset.Object);
	}

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GliderCollisionSphere);
	CameraBoom->SetUsingAbsoluteRotation(true);
	CameraBoom->TargetArmLength = CameraArmLength;
	CameraBoom->SocketOffset = FVector(0.0f, 0.0f, CameraSocketHeightOffset);
	CameraBoom->bUsePawnControlRotation = false;
	CameraBoom->bInheritPitch = true;
	CameraBoom->bInheritRoll = true;
	CameraBoom->bInheritYaw = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 10.0f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
	FollowCamera->FieldOfView = BaseFOV;

	SolarBoostVFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("SolarBoostVFX"));
	SolarBoostVFX->SetupAttachment(GliderMesh);
	SolarBoostVFX->bAutoActivate = false;

	CarbonSealProjectileClass = ACarbonSealProjectile::StaticClass();
}

void AGliderPawn::BeginPlay()
{
	Super::BeginPlay();

	if (GreyboxGliderMesh)
	{
		UStaticMesh* V2Glider = LoadObject<UStaticMesh>(nullptr, SolarGliderMeshPath);
		if (V2Glider)
		{
			GreyboxGliderMesh->SetStaticMesh(V2Glider);
		}
	}

	CurrentSpeed = BaseVelocity;
	SolarCharge = 100.0f;
	// Allow +150u initial maneuver buffer up to ceiling so immediate pitch-up feels responsive
	MaxAllowedGlideZ = FMath::Min(GetActorLocation().Z + 150.0f, GliderMaxCeilingAltitude);

	if (CameraBoom)
	{
		CameraBoom->SetUsingAbsoluteRotation(true);
		CameraBoom->TargetArmLength = CameraArmLength;
		CameraBoom->SocketOffset = FVector(0.0f, 0.0f, CameraSocketHeightOffset);
	}

	SmoothedCameraWorldRot = FRotator(GetActorRotation().Pitch * 0.20f, GetActorRotation().Yaw, 0.0f);
	if (CameraBoom)
	{
		CameraBoom->SetWorldRotation(FRotator(SmoothedCameraWorldRot.Pitch + CameraPitchOffset, SmoothedCameraWorldRot.Yaw + CameraYawOffset, 0.0f));
	}

	if (GreyboxGliderMesh && GliderMesh && GliderMesh->GetSkeletalMeshAsset() != nullptr)
	{
		GreyboxGliderMesh->SetVisibility(false);
	}

	if (GliderCollisionSphere)
	{
		GliderCollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &AGliderPawn::HandleGliderOverlap);
		GliderCollisionSphere->OnComponentHit.AddDynamic(this, &AGliderPawn::HandleGliderHit);
	}

	if (UWorld* World = GetWorld())
	{
		// Smokestacks are static level actors, so the sealable list is gathered once instead of every tick
		TArray<AActor*> SealableActors;
		UGameplayStatics::GetAllActorsWithInterface(World, UBPI_SealableTarget::StaticClass(), SealableActors);
		CachedSealableActors.Reset();
		for (AActor* SealableActor : SealableActors)
		{
			CachedSealableActors.Add(SealableActor);
		}

		World->GetTimerManager().SetTimer(
			SunlightTraceTimerHandle,
			this,
			&AGliderPawn::PerformSunlightTrace,
			SunlightTraceInterval,
			true);
	}

	PerformSunlightTrace();
}

void AGliderPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (!PlayerInputComponent)
	{
		return;
	}

	// Zero-setup direct key & mouse bindings so any test map works immediately out of the box
	PlayerInputComponent->BindKey(EKeys::W, IE_Pressed, this, &AGliderPawn::InputPitchUpPressed);
	PlayerInputComponent->BindKey(EKeys::W, IE_Released, this, &AGliderPawn::InputPitchUpReleased);
	PlayerInputComponent->BindKey(EKeys::S, IE_Pressed, this, &AGliderPawn::InputPitchDownPressed);
	PlayerInputComponent->BindKey(EKeys::S, IE_Released, this, &AGliderPawn::InputPitchDownReleased);

	PlayerInputComponent->BindKey(EKeys::A, IE_Pressed, this, &AGliderPawn::InputRollLeftPressed);
	PlayerInputComponent->BindKey(EKeys::A, IE_Released, this, &AGliderPawn::InputRollLeftReleased);
	PlayerInputComponent->BindKey(EKeys::D, IE_Pressed, this, &AGliderPawn::InputRollRightPressed);
	PlayerInputComponent->BindKey(EKeys::D, IE_Released, this, &AGliderPawn::InputRollRightReleased);

	PlayerInputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &AGliderPawn::StartSolarBoost);
	PlayerInputComponent->BindKey(EKeys::SpaceBar, IE_Released, this, &AGliderPawn::StopSolarBoost);
	PlayerInputComponent->BindKey(EKeys::LeftShift, IE_Pressed, this, &AGliderPawn::StartSolarBoost);
	PlayerInputComponent->BindKey(EKeys::LeftShift, IE_Released, this, &AGliderPawn::StopSolarBoost);

	PlayerInputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &AGliderPawn::FireCarbonSeal);

	PlayerInputComponent->BindAxisKey(EKeys::MouseX, this, &AGliderPawn::InputLookYaw);
	PlayerInputComponent->BindAxisKey(EKeys::MouseY, this, &AGliderPawn::InputLookPitch);
}

void AGliderPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bHasCrashed)
	{
		return;
	}

	if (SealCooldownRemaining > 0.0f)
	{
		SealCooldownRemaining = FMath::Max(0.0f, SealCooldownRemaining - DeltaTime);
	}

	UpdateSolarEnergyAndAltitudeChecks(DeltaTime);
	UpdateFlightKinematics(DeltaTime);
	UpdateDecoupledCamera(DeltaTime);

	// Compute aim direction unconditionally every frame so CachedAimWorldTarget,
	// bIsAimClampedToCone, and bHasMagneticRimLock are kept live for UMG HUD & targeting
	AActor* LockedTarget = nullptr;
	FVector LockedRimPos = FVector::ZeroVector;
	ComputeClampedAimDirection(LockedTarget, LockedRimPos);
}

float AGliderPawn::GetSealCooldownNormalized() const
{
	if (SealCooldownDuration <= KINDA_SMALL_NUMBER)
	{
		return 0.0f;
	}
	return FMath::Clamp(SealCooldownRemaining / SealCooldownDuration, 0.0f, 1.0f);
}

void AGliderPawn::ApplyWakeTurbulence(float DownwashSpeed, float DurationSeconds)
{
	if (GetActorLocation().Z <= WakeTargetFloorAltitude)
	{
		return;
	}
	ActiveDownwashSpeed = FMath::Max(ActiveDownwashSpeed, DownwashSpeed);
	DownwashTimeRemaining = FMath::Max(DownwashTimeRemaining, DurationSeconds);
	BP_OnWakeTurbulenceHit();
}

void AGliderPawn::InputPitch(float AxisValue)
{
	PitchInput = FMath::Clamp(AxisValue, -1.0f, 1.0f);
}

void AGliderPawn::InputRoll(float AxisValue)
{
	RollInput = FMath::Clamp(AxisValue, -1.0f, 1.0f);
}

void AGliderPawn::RefreshKeyAxisInputs()
{
	PitchInput = (bKeyPitchUpHeld ? 1.0f : 0.0f) + (bKeyPitchDownHeld ? -1.0f : 0.0f);
	RollInput = (bKeyRollRightHeld ? 1.0f : 0.0f) + (bKeyRollLeftHeld ? -1.0f : 0.0f);
}

void AGliderPawn::InputLookYaw(float AxisValue)
{
	CameraYawOffset = FMath::Clamp(CameraYawOffset + AxisValue * MouseSensitivity, -85.0f, 85.0f);
}

void AGliderPawn::InputLookPitch(float AxisValue)
{
	CameraPitchOffset = FMath::Clamp(CameraPitchOffset + AxisValue * MouseSensitivity, -60.0f, 45.0f);
}

void AGliderPawn::StartSolarBoost()
{
	bIsSolarBoostInputHeld = true;
}

void AGliderPawn::StopSolarBoost()
{
	bIsSolarBoostInputHeld = false;
}

void AGliderPawn::FireCarbonSeal()
{
	if (bHasCrashed || SealCooldownRemaining > 0.0f || !CarbonSealProjectileClass)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	AActor* MagneticTarget = nullptr;
	FVector MagneticRimLocation = FVector::ZeroVector;
	const FVector LaunchDir = ComputeClampedAimDirection(MagneticTarget, MagneticRimLocation);

	const FVector SpawnLocation = GetProjectileSpawnLocation();
	const FRotator SpawnRotation = LaunchDir.Rotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ACarbonSealProjectile* Projectile = World->SpawnActor<ACarbonSealProjectile>(
		CarbonSealProjectileClass,
		SpawnLocation,
		SpawnRotation,
		SpawnParams);

	if (Projectile)
	{
		Projectile->InitializeLaunch(LaunchDir, MagneticTarget, MagneticRimLocation);
		SealCooldownRemaining = SealCooldownDuration;
		OnCarbonSealFired.Broadcast();
		BP_OnCarbonSealFired(LaunchDir);
	}
}

void AGliderPawn::UpdateFlightKinematics(float DeltaTime)
{
	FRotator CurrentRot = GetActorRotation();

	// Stall nose-drop penalty if velocity approaches stall floor
	float EffectivePitchInput = PitchInput;
	if (CurrentSpeed <= StallVelocity + 40.0f && EffectivePitchInput > 0.0f && !bIsSolarBoosting)
	{
		EffectivePitchInput = -0.65f;
	}

	// Smooth the input axes rather than double-interpolating DeltaTime rotation steps
	SmoothedPitchInput = FMath::FInterpTo(SmoothedPitchInput, EffectivePitchInput, DeltaTime, 12.0f);
	SmoothedRollInput = FMath::FInterpTo(SmoothedRollInput, RollInput, DeltaTime, 12.0f);

	const float NewPitch = FMath::Clamp(CurrentRot.Pitch + SmoothedPitchInput * PitchRateDegPerSec * DeltaTime, -70.0f, 65.0f);
	const float NewYaw = CurrentRot.Yaw + SmoothedRollInput * RollYawRateDegPerSec * DeltaTime;
	const float NewRoll = SmoothedRollInput * 55.0f;

	FRotator SmoothedRot(NewPitch, NewYaw, NewRoll);
	SetActorRotation(SmoothedRot);

	// Convert pitch angle to kinetic acceleration (diving builds speed, climbing drains speed)
	const float PitchRadians = FMath::DegreesToRadians(SmoothedRot.Pitch);
	const float GravitySlopeAccel = -FMath::Sin(PitchRadians) * 1400.0f;

	float TargetVelocity = CurrentSpeed + GravitySlopeAccel * DeltaTime;

	// Natural aerodynamic drag pulling speed gently toward BaseVelocity in level flight
	if (FMath::Abs(SmoothedRot.Pitch) < 8.0f && !bIsSolarBoosting)
	{
		TargetVelocity = FMath::FInterpTo(TargetVelocity, BaseVelocity, DeltaTime, 1.2f);
	}

	if (bIsSolarBoosting)
	{
		TargetVelocity = FMath::Max(TargetVelocity, BaseVelocity + SolarBoostThrust);
	}

	CurrentSpeed = FMath::Clamp(TargetVelocity, StallVelocity, MaxDiveVelocity);

	FVector VelocityVector = GetActorForwardVector() * CurrentSpeed;

	// Baseline aerodynamic sink rate when in level flight without boost
	if (!bIsSolarBoosting && SmoothedRot.Pitch <= 2.0f)
	{
		const float PassiveSinkRate = CurrentSpeed / FMath::Max(1.0f, GlideRatio);
		VelocityVector.Z -= PassiveSinkRate * 0.25f;
	}

	// Apply wake turbulence downwash if active: rapidly forces glider down to chimney height (WakeTargetFloorAltitude)
	if (DownwashTimeRemaining > 0.0f)
	{
		if (GetActorLocation().Z <= WakeTargetFloorAltitude)
		{
			DownwashTimeRemaining = 0.0f;
			ActiveDownwashSpeed = 0.0f;
		}
		else
		{
			DownwashTimeRemaining = FMath::Max(0.0f, DownwashTimeRemaining - DeltaTime);
			VelocityVector.Z = -ActiveDownwashSpeed;
			MaxAllowedGlideZ = FMath::Max(WakeTargetFloorAltitude, GetActorLocation().Z - ActiveDownwashSpeed * DeltaTime);
			if (DownwashTimeRemaining <= 0.0f)
			{
				ActiveDownwashSpeed = 0.0f;
			}
		}
	}

	FVector DeltaMove = VelocityVector * DeltaTime;
	const float HorizontalStep = FVector(DeltaMove.X, DeltaMove.Y, 0.0f).Size();

	// Enforce Strict Net-Loss Glide Rule vs Solar Boost Ceiling
	if (bIsSolarBoosting)
	{
		MaxAllowedGlideZ = FMath::Min(GliderMaxCeilingAltitude, FMath::Max(MaxAllowedGlideZ, GetActorLocation().Z + DeltaMove.Z + 120.0f));
	}
	else
	{
		MaxAllowedGlideZ -= (HorizontalStep / FMath::Max(1.0f, GlideRatio)) * 0.5f;
	}

	FVector NewLocation = GetActorLocation() + DeltaMove;
	const float HardCeiling = bIsSolarBoosting ? GliderMaxCeilingAltitude : FMath::Min(GliderMaxCeilingAltitude, MaxAllowedGlideZ);

	if (NewLocation.Z > HardCeiling)
	{
		NewLocation.Z = HardCeiling;
	}
	if (DownwashTimeRemaining > 0.0f && NewLocation.Z < WakeTargetFloorAltitude)
	{
		NewLocation.Z = WakeTargetFloorAltitude;
		DownwashTimeRemaining = 0.0f;
		ActiveDownwashSpeed = 0.0f;
	}

	FHitResult MoveHit;
	SetActorLocation(NewLocation, true, &MoveHit);
	// The sweep's blocking hit is handled explicitly here as well as via OnComponentHit, since a swept
	// SetActorLocation does not reliably dispatch hit events; bHasCrashed makes a double call harmless
	if (MoveHit.bBlockingHit)
	{
		HandleGliderHit(GliderCollisionSphere, MoveHit.GetActor(), MoveHit.GetComponent(), FVector::ZeroVector, MoveHit);
	}
}

void AGliderPawn::UpdateSolarEnergyAndAltitudeChecks(float DeltaTime)
{
	const float CurrentZ = GetActorLocation().Z;

	// 1. Kill Floor (200u) Crash check
	if (CurrentZ <= KillFloorAltitude)
	{
		TriggerCrash(EGliderCrashCause::Ground);
		return;
	}

	// 2. Pull-Up Warning (600u) state check
	const bool bShouldWarnPullUp = (CurrentZ < PullUpWarningAltitude);
	if (bShouldWarnPullUp != bIsPullUpWarningActive)
	{
		bIsPullUpWarningActive = bShouldWarnPullUp;
		OnPullUpWarningChanged.Broadcast(bIsPullUpWarningActive);
	}

	// 3. Solar Boost vs Binary Solar Charge (+12.5%/s in sunlight, 0%/s in smog)
	const bool bWasBoosting = bIsSolarBoosting;
	if (bIsSolarBoostInputHeld && SolarCharge > 0.0f)
	{
		bIsSolarBoosting = true;
		SolarCharge = FMath::Max(0.0f, SolarCharge - SolarBoostDrainRatePerSec * DeltaTime);
		if (SolarCharge <= 0.0f)
		{
			bIsSolarBoostInputHeld = false;
			bIsSolarBoosting = false;
		}
	}
	else
	{
		bIsSolarBoosting = false;
		if (bIsInSunlight)
		{
			SolarCharge = FMath::Min(100.0f, SolarCharge + SolarChargeRatePerSec * DeltaTime);
		}
	}

	if (bWasBoosting != bIsSolarBoosting)
	{
		if (SolarBoostVFX)
		{
			if (bIsSolarBoosting)
			{
				SolarBoostVFX->Activate(true);
			}
			else
			{
				SolarBoostVFX->Deactivate();
			}
		}
		BP_OnSolarBoostStateChanged(bIsSolarBoosting);
	}
}

void AGliderPawn::UpdateDecoupledCamera(float DeltaTime)
{
	if (CameraBoom)
	{
		// Smooth ONLY the glider's body rotation follow, while applying mouse look (CameraYawOffset/PitchOffset) instantaneously!
		const FRotator DesiredGliderFollowRot(GetActorRotation().Pitch * 0.20f, GetActorRotation().Yaw, 0.0f);
		SmoothedCameraWorldRot = FMath::RInterpTo(SmoothedCameraWorldRot, DesiredGliderFollowRot, DeltaTime, CameraRotationLagSpeed);

		const FRotator FinalCameraWorldRot(
			FMath::Clamp(SmoothedCameraWorldRot.Pitch + CameraPitchOffset, -65.0f, 45.0f),
			SmoothedCameraWorldRot.Yaw + CameraYawOffset,
			0.0f);
		CameraBoom->SetWorldRotation(FinalCameraWorldRot);
	}

	if (FollowCamera)
	{
		const bool bHighSpeedOrBoost = bIsSolarBoosting || (CurrentSpeed > BaseVelocity + 650.0f);
		const float DesiredFOV = bHighSpeedOrBoost ? BoostOrDiveFOV : BaseFOV;
		FollowCamera->SetFieldOfView(FMath::FInterpTo(FollowCamera->FieldOfView, DesiredFOV, DeltaTime, 2.5f));
	}
}

void AGliderPawn::PerformSunlightTrace()
{
	const FVector StartLoc = GetActorLocation();
	if (StartLoc.Z < SmogBedCeilingAltitude)
	{
		bIsInSunlight = false;
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Vertical upward LineTrace checking against overhead SmogCanopy blockers
	const FVector EndLoc = StartLoc + FVector(0.0f, 0.0f, 6000.0f);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SunlightTrace), false, this);

	FHitResult Hit;
	const bool bBlocked = World->LineTraceSingleByChannel(Hit, StartLoc, EndLoc, ECC_Visibility, QueryParams);
	bIsInSunlight = !bBlocked;
}

FVector AGliderPawn::ComputeClampedAimDirection(AActor*& OutMagneticTarget, FVector& OutMagneticRimLocation) const
{
	OutMagneticTarget = nullptr;
	OutMagneticRimLocation = FVector::ZeroVector;

	const FVector SpawnLoc = GetProjectileSpawnLocation();
	const FVector GliderForward = GetActorForwardVector().GetSafeNormal();
	const FVector CamLoc = FollowCamera ? FollowCamera->GetComponentLocation() : SpawnLoc;
	const FVector CamDir = FollowCamera ? FollowCamera->GetForwardVector().GetSafeNormal() : GliderForward;

	const float MaxConeCos = FMath::Cos(FMath::DegreesToRadians(MaxAimConeHalfAngleDeg));
	const float MinAssistCos = FMath::Cos(FMath::DegreesToRadians(AimAssistConeHalfAngleDeg));
	float BestDot = MinAssistCos;

	// Track depth along camera ray for convergence (defaulting to 3000u, clamped 800u .. AimAssistMaxRange)
	float ClosestDepthAlongCamRay = 3000.0f;
	float BestAlignForDepth = 0.5f;

	if (UWorld* World = GetWorld())
	{
		for (const TWeakObjectPtr<AActor>& CandidatePtr : CachedSealableActors)
		{
			AActor* Candidate = CandidatePtr.Get();
			if (!Candidate || IBPI_SealableTarget::Execute_IsTargetSealed(Candidate))
			{
				continue;
			}

			TArray<FVector> CandidateRims = IBPI_SealableTarget::Execute_GetUnsealedRimLocations(Candidate);
			if (CandidateRims.Num() == 0)
			{
				CandidateRims.Add(IBPI_SealableTarget::Execute_GetTargetRimLocation(Candidate));
			}

			for (const FVector& RimLoc : CandidateRims)
			{
				const FVector ToRimCam = RimLoc - CamLoc;
				const float CamDistSq = ToRimCam.SizeSquared();
				if (CamDistSq > FMath::Square(AimAssistMaxRange) || CamDistSq < 100.0f)
				{
					continue;
				}

				const float DepthAlongCam = FVector::DotProduct(ToRimCam, CamDir);
				if (DepthAlongCam <= 100.0f)
				{
					continue;
				}

				const FVector DirFromCam = ToRimCam.GetSafeNormal();
				const FVector DirFromSpawn = (RimLoc - SpawnLoc).GetSafeNormal();

				// Verify that (RimLoc - SpawnLoc).GetSafeNormal() is within MaxAimConeHalfAngleDeg of GliderForward
				if (FVector::DotProduct(GliderForward, DirFromSpawn) < MaxConeCos)
				{
					continue;
				}

				const float AimDot = FVector::DotProduct(CamDir, DirFromCam);

				// Track depth of aligned unsealed rims in front of the camera
				if (AimDot > BestAlignForDepth)
				{
					BestAlignForDepth = AimDot;
					ClosestDepthAlongCamRay = FMath::Clamp(DepthAlongCam, 800.0f, AimAssistMaxRange);
				}

				// Check magnetic lock within assist cone
				if (AimDot > BestDot)
				{
					BestDot = AimDot;
					OutMagneticTarget = Candidate;
					OutMagneticRimLocation = RimLoc;
					ClosestDepthAlongCamRay = FMath::Clamp(DepthAlongCam, 800.0f, AimAssistMaxRange);
				}
			}
		}
	}

	FVector WorldTargetPoint;
	FVector DesiredAimDir;
	bool bClamped = false;

	// Single source of truth: the lock flag is true only when the rim is actually used below
	const bool bUseMagneticRim = (OutMagneticTarget != nullptr && !OutMagneticRimLocation.IsNearlyZero());

	if (bUseMagneticRim)
	{
		WorldTargetPoint = OutMagneticRimLocation;
		DesiredAimDir = (OutMagneticRimLocation - SpawnLoc).GetSafeNormal();
	}
	else
	{
		WorldTargetPoint = CamLoc + CamDir * ClosestDepthAlongCamRay;
		DesiredAimDir = (WorldTargetPoint - SpawnLoc).GetSafeNormal();

		const float DotWithGlider = FVector::DotProduct(GliderForward, DesiredAimDir);
		if (DotWithGlider < MaxConeCos)
		{
			bClamped = true;
			const float MaxConeRadians = FMath::DegreesToRadians(MaxAimConeHalfAngleDeg);
			const FVector RotationAxis = FVector::CrossProduct(GliderForward, DesiredAimDir).GetSafeNormal();
			if (!RotationAxis.IsNearlyZero())
			{
				DesiredAimDir = FQuat(RotationAxis, MaxConeRadians).RotateVector(GliderForward).GetSafeNormal();
			}
			else
			{
				DesiredAimDir = GliderForward;
			}
			WorldTargetPoint = SpawnLoc + DesiredAimDir * ClosestDepthAlongCamRay;
		}
	}

	CachedAimWorldTarget = WorldTargetPoint;
	bIsAimClampedToCone = bClamped;
	bHasMagneticRimLock = bUseMagneticRim;

	return DesiredAimDir;
}

void AGliderPawn::TriggerCrash(EGliderCrashCause Cause)
{
	if (bHasCrashed)
	{
		return;
	}

	bHasCrashed = true;
	LastCrashCause = Cause;

	// Freeze all live gameplay state so nothing keeps updating HUD/VFX after the crash
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SunlightTraceTimerHandle);
	}

	if (bIsPullUpWarningActive)
	{
		bIsPullUpWarningActive = false;
		OnPullUpWarningChanged.Broadcast(false);
	}

	const bool bWasBoosting = bIsSolarBoosting;
	bIsSolarBoosting = false;
	bIsSolarBoostInputHeld = false;
	if (bWasBoosting)
	{
		if (SolarBoostVFX)
		{
			SolarBoostVFX->Deactivate();
		}
		BP_OnSolarBoostStateChanged(false);
	}

	bKeyPitchUpHeld = false;
	bKeyPitchDownHeld = false;
	bKeyRollLeftHeld = false;
	bKeyRollRightHeld = false;
	RefreshKeyAxisInputs();

	OnGliderCrashed.Broadcast(this);
}

void AGliderPawn::HandleGliderOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (bHasCrashed || !OtherActor || OtherActor == this)
	{
		return;
	}

	if (OtherActor->Implements<UBPI_FlightHazard>())
	{
		// The only lethal overlap volume is the Type C heavy smoke pillar
		if (IBPI_FlightHazard::Execute_IsLethalCrashHazard(OtherActor, OtherComp))
		{
			TriggerCrash(EGliderCrashCause::ToxicPlume);
			return;
		}

		const float Downwash = IBPI_FlightHazard::Execute_GetWakeDownwashSpeed(OtherActor, OtherComp);
		if (Downwash > 0.0f)
		{
			ApplyWakeTurbulence(Downwash, 3.0f);
		}
	}
}

void AGliderPawn::HandleGliderHit(
	UPrimitiveComponent* HitComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	FVector NormalImpulse,
	const FHitResult& Hit)
{
	if (bHasCrashed || !OtherActor || OtherActor == this)
	{
		return;
	}

	// Hitting any blocking world geometry or a lethal flight hazard triggers Crash
	if (OtherActor->Implements<UBPI_FlightHazard>())
	{
		if (IBPI_FlightHazard::Execute_IsLethalCrashHazard(OtherActor, OtherComp))
		{
			TriggerCrash(EGliderCrashCause::Smokestack);
			return;
		}
	}
	else
	{
		TriggerCrash(EGliderCrashCause::Ground);
	}
}

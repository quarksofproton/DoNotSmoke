// Copyright Do Not Smoke Team. All Rights Reserved.

#include "Smokestack.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/StaticMesh.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

// Rev3 mesh asset paths, shared by the constructor loaders and ResolveRev3Mesh
namespace
{
	const TCHAR* const MeshSinglePath = TEXT("/Game/Game/Meshes/Rev3/SM_Smokestack_Single/StaticMeshes/SM_Smokestack_Single.SM_Smokestack_Single");
	const TCHAR* const MeshClusterPath = TEXT("/Game/Game/Meshes/Rev3/SM_Smokestack_Cluster/StaticMeshes/SM_Smokestack_Cluster.SM_Smokestack_Cluster");
	const TCHAR* const MeshCluster2Path = TEXT("/Game/Game/Meshes/Rev3/SM_Smokestack_Cluster_2/StaticMeshes/SM_Smokestack_Cluster_2.SM_Smokestack_Cluster_2");
	const TCHAR* const MeshCluster3Path = TEXT("/Game/Game/Meshes/Rev3/SM_Smokestack_Cluster_3/StaticMeshes/SM_Smokestack_Cluster_3.SM_Smokestack_Cluster_3");
	const TCHAR* const MeshHeavyPath = TEXT("/Game/Game/Meshes/Rev3/SM_Smokestack_Heavy/StaticMeshes/SM_Smokestack_Heavy.SM_Smokestack_Heavy");
	const TCHAR* const SealMeshSinglePath = TEXT("/Game/Game/Meshes/Rev3/SM_Seal_Single/StaticMeshes/SM_Seal_Single.SM_Seal_Single");
}

ASmokestack::ASmokestack()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	ChimneyBodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ChimneyBodyMesh"));
	ChimneyBodyMesh->SetupAttachment(SceneRoot);
	ChimneyBodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	auto InitBodyCollision = [this](const TCHAR* Name) -> UCapsuleComponent*
	{
		UCapsuleComponent* Comp = CreateDefaultSubobject<UCapsuleComponent>(Name);
		Comp->SetupAttachment(SceneRoot);
		Comp->SetRelativeLocation(FVector(0.0f, 0.0f, 600.0f));
		Comp->InitCapsuleSize(280.0f, 600.0f);
		Comp->SetCollisionProfileName(TEXT("BlockAll"));
		Comp->SetGenerateOverlapEvents(true);
		return Comp;
	};

	ChimneyBodyCollision = InitBodyCollision(TEXT("ChimneyBodyCollision"));
	ChimneyBodyCollision_2 = InitBodyCollision(TEXT("ChimneyBodyCollision_2"));
	ChimneyBodyCollision_3 = InitBodyCollision(TEXT("ChimneyBodyCollision_3"));

	auto InitRimSphere = [this](const TCHAR* Name) -> USphereComponent*
	{
		USphereComponent* Comp = CreateDefaultSubobject<USphereComponent>(Name);
		Comp->SetupAttachment(SceneRoot);
		Comp->SetRelativeLocation(FVector(0.0f, 0.0f, 1200.0f));
		Comp->InitSphereRadius(RimSphereRadius);
		Comp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Comp->SetCollisionResponseToAllChannels(ECR_Overlap);
		Comp->SetGenerateOverlapEvents(true);
		Comp->SetHiddenInGame(true);
		Comp->ShapeColor = FColor::Green;
		return Comp;
	};

	RimTargetSphere = InitRimSphere(TEXT("RimTargetSphere"));
	RimTargetSphere_2 = InitRimSphere(TEXT("RimTargetSphere_2"));
	RimTargetSphere_3 = InitRimSphere(TEXT("RimTargetSphere_3"));

	auto InitSealCap = [this](const TCHAR* Name, USphereComponent* ParentRim) -> UStaticMeshComponent*
	{
		UStaticMeshComponent* Comp = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Comp->SetupAttachment(ParentRim);
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Comp->SetVisibility(false);
		return Comp;
	};

	SealCapMesh = InitSealCap(TEXT("SealCapMesh"), RimTargetSphere);
	SealCapMesh_2 = InitSealCap(TEXT("SealCapMesh_2"), RimTargetSphere_2);
	SealCapMesh_3 = InitSealCap(TEXT("SealCapMesh_3"), RimTargetSphere_3);

	LethalPlumeCapsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("LethalPlumeCapsule"));
	LethalPlumeCapsule->SetupAttachment(RimTargetSphere);
	LethalPlumeCapsule->SetRelativeLocation(FVector(0.0f, 0.0f, LethalPlumeHalfHeight));
	LethalPlumeCapsule->InitCapsuleSize(LethalPlumeRadius, LethalPlumeHalfHeight);
	LethalPlumeCapsule->SetCollisionResponseToAllChannels(ECR_Overlap);
	LethalPlumeCapsule->SetGenerateOverlapEvents(true);
	LethalPlumeCapsule->SetHiddenInGame(true);
	LethalPlumeCapsule->ShapeColor = FColor::Red;

	auto InitConeSegment = [this](const TCHAR* Name) -> UCapsuleComponent*
	{
		UCapsuleComponent* Segment = CreateDefaultSubobject<UCapsuleComponent>(Name);
		Segment->SetupAttachment(RimTargetSphere);
		Segment->SetCollisionResponseToAllChannels(ECR_Overlap);
		Segment->SetGenerateOverlapEvents(true);
		Segment->SetHiddenInGame(true);
		Segment->ShapeColor = FColor::Red;
		return Segment;
	};
	LethalPlumeCone_1 = InitConeSegment(TEXT("LethalPlumeCone_1"));
	LethalPlumeCone_2 = InitConeSegment(TEXT("LethalPlumeCone_2"));
	LethalPlumeCone_3 = InitConeSegment(TEXT("LethalPlumeCone_3"));

	OverheadSmogCanopySphere = CreateDefaultSubobject<USphereComponent>(TEXT("OverheadSmogCanopySphere"));
	OverheadSmogCanopySphere->SetupAttachment(SceneRoot);
	OverheadSmogCanopySphere->SetWorldLocation(FVector(0.0f, 0.0f, CanopyWorldAltitude));
	OverheadSmogCanopySphere->InitSphereRadius(CanopyMinRadius);
	OverheadSmogCanopySphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	OverheadSmogCanopySphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	OverheadSmogCanopySphere->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	// The lifted dome is ~400 km wide; never draw its wireframe
	OverheadSmogCanopySphere->SetVisibility(false);

	CanopySmokeVFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("CanopySmokeVFX"));
	CanopySmokeVFX->SetupAttachment(SceneRoot);
	CanopySmokeVFX->SetUsingAbsoluteLocation(true);
	CanopySmokeVFX->bAutoActivate = true;
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> SkySmogAsset(TEXT("/Game/Game/VFX/NS_SkySmog.NS_SkySmog"));
	if (SkySmogAsset.Succeeded())
	{
		SkySmogSystem = SkySmogAsset.Object;
		CanopySmokeVFX->SetAsset(SkySmogSystem);
	}

	auto InitVFX = [this](const TCHAR* Name, USphereComponent* ParentRim, bool bAutoAct) -> UNiagaraComponent*
	{
		UNiagaraComponent* Comp = CreateDefaultSubobject<UNiagaraComponent>(Name);
		Comp->SetupAttachment(ParentRim);
		Comp->bAutoActivate = bAutoAct;
		return Comp;
	};

	ToxicSmogVFX = InitVFX(TEXT("ToxicSmogVFX"), RimTargetSphere, true);
	ToxicSmogVFX_2 = InitVFX(TEXT("ToxicSmogVFX_2"), RimTargetSphere_2, true);
	ToxicSmogVFX_3 = InitVFX(TEXT("ToxicSmogVFX_3"), RimTargetSphere_3, true);

	SealSparkleVFX = InitVFX(TEXT("SealSparkleVFX"), RimTargetSphere, false);
	SealSparkleVFX_2 = InitVFX(TEXT("SealSparkleVFX_2"), RimTargetSphere_2, false);
	SealSparkleVFX_3 = InitVFX(TEXT("SealSparkleVFX_3"), RimTargetSphere_3, false);

	// Load imported Do Not Smoke modular chimney & seal cap meshes (Revision 3)
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SingleAsset(MeshSinglePath);
	if (SingleAsset.Succeeded())
	{
		MeshSingle = SingleAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> ClusterAsset(MeshClusterPath);
	if (ClusterAsset.Succeeded())
	{
		MeshCluster = ClusterAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cluster2Asset(MeshCluster2Path);
	if (Cluster2Asset.Succeeded())
	{
		MeshCluster2 = Cluster2Asset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cluster3Asset(MeshCluster3Path);
	if (Cluster3Asset.Succeeded())
	{
		MeshCluster3 = Cluster3Asset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> HeavyAsset(MeshHeavyPath);
	if (HeavyAsset.Succeeded())
	{
		MeshHeavy = HeavyAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SealSingleAsset(SealMeshSinglePath);
	if (SealSingleAsset.Succeeded())
	{
		SealMeshSingle = SealSingleAsset.Object;
	}


	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> ToxicSmogAsset(TEXT("/Game/Game/VFX/NS_ToxicSmog.NS_ToxicSmog"));
	if (ToxicSmogAsset.Succeeded())
	{
		ToxicSmogSystem = ToxicSmogAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> SealSparkleAsset(TEXT("/Game/Game/VFX/NS_SealSparkle.NS_SealSparkle"));
	if (SealSparkleAsset.Succeeded())
	{
		SealSparkleSystem = SealSparkleAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<USoundBase> SealLatchAsset(TEXT("/Game/Game/Audio/SFX_Seal_Latch.SFX_Seal_Latch"));
	if (SealLatchAsset.Succeeded())
	{
		SealLatchSound = SealLatchAsset.Object;
	}

	HeavyRumbleAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("HeavyRumbleAudio"));
	HeavyRumbleAudio->SetupAttachment(RimTargetSphere);
	HeavyRumbleAudio->bAutoActivate = false;
	HeavyRumbleAudio->bOverrideAttenuation = true;
	HeavyRumbleAudio->AttenuationOverrides.bAttenuate = true;
	HeavyRumbleAudio->AttenuationOverrides.bSpatialize = true;
	HeavyRumbleAudio->AttenuationOverrides.AttenuationShapeExtents = FVector(1500.0f);
	HeavyRumbleAudio->AttenuationOverrides.FalloffDistance = 6000.0f;
	static ConstructorHelpers::FObjectFinder<USoundBase> HeavyRumbleAsset(TEXT("/Game/Game/Audio/SFX_Heavy_Smog_Rumble.SFX_Heavy_Smog_Rumble"));
	if (HeavyRumbleAsset.Succeeded())
	{
		HeavyRumbleAudio->SetSound(HeavyRumbleAsset.Object);
	}

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> HeavyToxicSmogAsset(TEXT("/Game/Game/VFX/NS_HeavyToxicSmog.NS_HeavyToxicSmog"));
	if (HeavyToxicSmogAsset.Succeeded())
	{
		HeavyToxicSmogSystem = HeavyToxicSmogAsset.Object;
	}

	if (ToxicSmogSystem)
	{
		ToxicSmogVFX->SetAsset(ToxicSmogSystem);
		ToxicSmogVFX_2->SetAsset(ToxicSmogSystem);
		ToxicSmogVFX_3->SetAsset(ToxicSmogSystem);
	}

	if (SealSparkleSystem)
	{
		SealSparkleVFX->SetAsset(SealSparkleSystem);
		SealSparkleVFX_2->SetAsset(SealSparkleSystem);
		SealSparkleVFX_3->SetAsset(SealSparkleSystem);
	}

	if (MeshSingle)
	{
		ChimneyBodyMesh->SetStaticMesh(MeshSingle);
	}
	if (SealMeshSingle)
	{
		SealCapMesh->SetStaticMesh(SealMeshSingle);
		SealCapMesh_2->SetStaticMesh(SealMeshSingle);
		SealCapMesh_3->SetStaticMesh(SealMeshSingle);
	}
}

void ASmokestack::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (OverheadSmogCanopySphere)
	{
		ApplyCanopyRadius(CanopyMinRadius);
	}

	RefreshArchetypeCollisionState();
}

void ASmokestack::BeginPlay()
{
	Super::BeginPlay();

	for (int32 i = 0; i < 3; ++i)
	{
		bMouthSealed[i] = bIsSealed;
	}

	// NS_SealSparkle reuses NS_SunMotes' user parameters: cylinder radius, spawn rate, sprite size multiplier
	UNiagaraComponent* SparkleVFXs[3] = { SealSparkleVFX, SealSparkleVFX_2, SealSparkleVFX_3 };
	for (UNiagaraComponent* Sparkle : SparkleVFXs)
	{
		if (Sparkle)
		{
			// The stack actor is scaled per archetype; the glint keeps its own size instead of stretching with it
			Sparkle->SetUsingAbsoluteScale(true);
			Sparkle->SetWorldScale3D(FVector::OneVector);
			Sparkle->SetVariableFloat(TEXT("SpreadRadius"), RimSphereRadius * SealSparkleSpreadFraction);
			Sparkle->SetVariableFloat(TEXT("SpawnRate"), SealSparkleSpawnRate);
			Sparkle->SetVariableFloat(TEXT("SizeScale"), SealSparkleSizeScale);
		}
	}

	RefreshArchetypeCollisionState();

	if (HeavyRumbleAudio && SmokestackType == ESmokestackType::HeavyLethal && !bIsSealed)
	{
		HeavyRumbleAudio->Play(FMath::FRandRange(0.0f, 5.0f));
	}
}

int32 ASmokestack::GetActiveMouthCount() const
{
	if (SmokestackType == ESmokestackType::Clustered)
	{
		return FMath::Clamp(ClusterStackCount, 2, 3);
	}
	return 1;
}

int32 ASmokestack::GetUnsealedMouthCount() const
{
	if (bIsSealed)
	{
		return 0;
	}

	const int32 ActiveMouths = GetActiveMouthCount();
	int32 UnsealedCount = 0;
	for (int32 i = 0; i < ActiveMouths; ++i)
	{
		if (!bMouthSealed[i])
		{
			++UnsealedCount;
		}
	}
	return UnsealedCount;
}

void ASmokestack::SetCanopyExpansionAlpha(float ExpansionAlpha)
{
	if (bIsSealed || !OverheadSmogCanopySphere)
	{
		return;
	}

	const float ClampedAlpha = FMath::Clamp(ExpansionAlpha, 0.0f, 1.0f);
	ApplyCanopyRadius(FMath::Lerp(CanopyMinRadius, CanopyMaxRadius, ClampedAlpha));
}

void ASmokestack::ApplyCanopyRadius(float Radius)
{
	if (OverheadSmogCanopySphere)
	{
		// Bottom cap of a lifted sphere: its circle at CanopyWorldAltitude has exactly the canopy radius
		const FVector ActorLoc = GetActorLocation();
		OverheadSmogCanopySphere->SetWorldLocation(FVector(ActorLoc.X, ActorLoc.Y, CanopyWorldAltitude + CanopyDomeLift));
		OverheadSmogCanopySphere->SetSphereRadius(FMath::Sqrt(FMath::Square(Radius) + FMath::Square(CanopyDomeLift)));
	}
	UpdateCanopyCloud(Radius);
}

void ASmokestack::UpdateCanopyCloud(float Radius)
{
	if (!CanopySmokeVFX)
	{
		return;
	}

	const float GrowthAlpha = FMath::GetRangePct(CanopyMinRadius, CanopyMaxRadius, Radius);
	CanopySmokeVFX->SetWorldLocation(FVector(GetActorLocation().X, GetActorLocation().Y, CanopyWorldAltitude + CanopySmokeOffsetZ));
	CanopySmokeVFX->SetVariableFloat(TEXT("SpreadRadius"), Radius);
	const float ClampedGrowth = FMath::Clamp(GrowthAlpha, 0.0f, 1.0f);
	CanopySmokeVFX->SetVariableFloat(TEXT("SpawnRate"), FMath::Lerp(CanopySmokeSpawnRateRange.X, CanopySmokeSpawnRateRange.Y, ClampedGrowth));
	CanopySmokeVFX->SetVariableFloat(TEXT("SizeScale"), FMath::Lerp(CanopySmokeSizeScaleRange.X, CanopySmokeSizeScaleRange.Y, ClampedGrowth));
}

UNiagaraSystem* ASmokestack::GetActiveSmogSystem() const
{
	return (SmokestackType == ESmokestackType::HeavyLethal && HeavyToxicSmogSystem) ? HeavyToxicSmogSystem.Get() : ToxicSmogSystem.Get();
}

bool ASmokestack::TrySealTarget_Implementation(AActor* SealInstigator)
{
	if (bIsSealed)
	{
		return false;
	}

	USphereComponent* RimSpheres[3] = { RimTargetSphere, RimTargetSphere_2, RimTargetSphere_3 };
	UNiagaraComponent* ToxicVFXs[3] = { ToxicSmogVFX, ToxicSmogVFX_2, ToxicSmogVFX_3 };
	UNiagaraComponent* SparkleVFXs[3] = { SealSparkleVFX, SealSparkleVFX_2, SealSparkleVFX_3 };

	const int32 ActiveMouths = GetActiveMouthCount();
	const FVector HitLocation = SealInstigator ? SealInstigator->GetActorLocation() : GetActorLocation();

	int32 BestMouthIdx = INDEX_NONE;
	float BestDistSq = TNumericLimits<float>::Max();

	for (int32 i = 0; i < ActiveMouths; ++i)
	{
		if (!bMouthSealed[i] && RimSpheres[i])
		{
			const float DistSq = FVector::DistSquared(HitLocation, RimSpheres[i]->GetComponentLocation());
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				BestMouthIdx = i;
			}
		}
	}

	if (BestMouthIdx == INDEX_NONE)
	{
		return false;
	}

	bMouthSealed[BestMouthIdx] = true;

	if (GetUnsealedMouthCount() == 0)
	{
		bIsSealed = true;
	}

	RefreshArchetypeCollisionState();

	if (ToxicVFXs[BestMouthIdx])
	{
		ToxicVFXs[BestMouthIdx]->Deactivate();
	}

	if (SparkleVFXs[BestMouthIdx])
	{
		SparkleVFXs[BestMouthIdx]->Activate(true);
	}

	if (SealLatchSound && RimSpheres[BestMouthIdx])
	{
		UGameplayStatics::PlaySoundAtLocation(this, SealLatchSound, RimSpheres[BestMouthIdx]->GetComponentLocation());
	}

	if (bIsSealed && HeavyRumbleAudio && HeavyRumbleAudio->IsPlaying())
	{
		HeavyRumbleAudio->FadeOut(1.5f, 0.0f);
	}

	OnSmokestackSealed.Broadcast(this);
	BP_OnSmokestackSealed();

	return true;
}

bool ASmokestack::IsTargetSealed_Implementation() const
{
	return bIsSealed;
}

FVector ASmokestack::GetTargetRimLocation_Implementation() const
{
	USphereComponent* const RimSpheres[3] = { RimTargetSphere, RimTargetSphere_2, RimTargetSphere_3 };
	const int32 ActiveMouths = GetActiveMouthCount();

	for (int32 i = 0; i < ActiveMouths; ++i)
	{
		if (!bMouthSealed[i] && RimSpheres[i])
		{
			return RimSpheres[i]->GetComponentLocation();
		}
	}

	return RimTargetSphere ? RimTargetSphere->GetComponentLocation() : GetActorLocation();
}

TArray<FVector> ASmokestack::GetUnsealedRimLocations_Implementation() const
{
	TArray<FVector> UnsealedLocations;
	if (bIsSealed)
	{
		return UnsealedLocations;
	}

	USphereComponent* const RimSpheres[3] = { RimTargetSphere, RimTargetSphere_2, RimTargetSphere_3 };
	const int32 ActiveMouths = GetActiveMouthCount();

	for (int32 i = 0; i < ActiveMouths; ++i)
	{
		if (!bMouthSealed[i] && RimSpheres[i])
		{
			UnsealedLocations.Add(RimSpheres[i]->GetComponentLocation());
		}
	}

	return UnsealedLocations;
}

bool ASmokestack::IsLethalCrashHazard_Implementation(const UPrimitiveComponent* HazardComponent) const
{
	if (!HazardComponent)
	{
		return false;
	}

	if (HazardComponent == ChimneyBodyCollision
		|| HazardComponent == ChimneyBodyCollision_2
		|| HazardComponent == ChimneyBodyCollision_3
		|| HazardComponent == ChimneyBodyMesh)
	{
		return true;
	}

	if (HazardComponent && (HazardComponent == LethalPlumeCapsule || HazardComponent == LethalPlumeCone_1
		|| HazardComponent == LethalPlumeCone_2 || HazardComponent == LethalPlumeCone_3))
	{
		return (SmokestackType == ESmokestackType::HeavyLethal) && !bIsSealed;
	}

	return false;
}

float ASmokestack::GetWakeDownwashSpeed_Implementation(const UPrimitiveComponent* HazardComponent) const
{
	return 0.0f;
}

void ASmokestack::RefreshArchetypeCollisionState()
{
	// Ensure Rev3 assets are resolved if null or pointing to obsolete cache
	auto ResolveRev3Mesh = [](TObjectPtr<UStaticMesh>& MeshPtr, const TCHAR* Path)
	{
		if (!MeshPtr || !MeshPtr->GetPathName().Contains(TEXT("Rev3")))
		{
			if (UStaticMesh* Loaded = LoadObject<UStaticMesh>(nullptr, Path))
			{
				MeshPtr = Loaded;
			}
		}
	};

	ResolveRev3Mesh(MeshSingle, MeshSinglePath);
	ResolveRev3Mesh(MeshCluster, MeshClusterPath);
	ResolveRev3Mesh(MeshCluster2, MeshCluster2Path);
	ResolveRev3Mesh(MeshCluster3, MeshCluster3Path);
	ResolveRev3Mesh(MeshHeavy, MeshHeavyPath);
	ResolveRev3Mesh(SealMeshSingle, SealMeshSinglePath);

	// Exact mouth heights & offsets from dimensions.json (Revision 3):
	// Assembly_Single: 1 mouth at (0, 0, 1200)
	// Assembly_Cluster_2: 2 mouths at (-340, 0, 600) and (340, 0, 600)
	// Assembly_Cluster_3: 3 mouths in equilateral triangle at (-340, 196.3, 600), (340, 196.3, 600), and (0, -392.6, 600)
	// Assembly_Heavy: 1 mouth at (0, 0, 1400)
	// Mouth heights below are the art's; they are then multiplied by the type height scale and HeightVariation
	float MouthHeightZ = 1200.0f;
	float ShaftRadius = 275.0f;
	int32 ActiveMouths = 1;
	FVector MouthOffsets[3] = { FVector::ZeroVector, FVector::ZeroVector, FVector::ZeroVector };
	UStaticMesh* DesiredBodyMesh = MeshSingle;

	switch (SmokestackType)
	{
	case ESmokestackType::Normal:
		MouthHeightZ = 1200.0f;
		ShaftRadius = 275.0f;
		ActiveMouths = 1;
		MouthOffsets[0] = FVector(0.0f, 0.0f, MouthHeightZ);
		DesiredBodyMesh = MeshSingle;
		break;

	case ESmokestackType::Clustered:
		MouthHeightZ = 600.0f;
		ShaftRadius = 265.0f;
		ActiveMouths = GetActiveMouthCount();
		if (ActiveMouths == 3)
		{
			MouthOffsets[0] = FVector(-340.0f, 196.3f, MouthHeightZ);
			MouthOffsets[1] = FVector(340.0f, 196.3f, MouthHeightZ);
			MouthOffsets[2] = FVector(0.0f, -392.6f, MouthHeightZ);
			DesiredBodyMesh = MeshCluster3 ? MeshCluster3 : MeshCluster;
		}
		else
		{
			MouthOffsets[0] = FVector(-340.0f, 0.0f, MouthHeightZ);
			MouthOffsets[1] = FVector(340.0f, 0.0f, MouthHeightZ);
			DesiredBodyMesh = MeshCluster2 ? MeshCluster2 : MeshCluster;
		}
		break;

	case ESmokestackType::HeavyLethal:
		MouthHeightZ = 1400.0f;
		ShaftRadius = 315.0f;
		ActiveMouths = 1;
		MouthOffsets[0] = FVector(0.0f, 0.0f, MouthHeightZ);
		DesiredBodyMesh = MeshHeavy;
		break;
	}

	// Stretch the assembly vertically; mouths, rims, collision and VFX all follow the scaled mouth height
	const float TypeHeightScale = SmokestackType == ESmokestackType::Normal ? NormalHeightScale
		: (SmokestackType == ESmokestackType::Clustered ? ClusteredHeightScale : HeavyHeightScale);
	const float HeightScale = FMath::Max(0.1f, TypeHeightScale * HeightVariation);
	MouthHeightZ *= HeightScale;
	for (FVector& Offset : MouthOffsets)
	{
		Offset.Z *= HeightScale;
	}

	if (ChimneyBodyMesh && DesiredBodyMesh)
	{
		ChimneyBodyMesh->SetStaticMesh(DesiredBodyMesh);
		ChimneyBodyMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, HeightScale));
	}

	UCapsuleComponent* BodyCapsules[3] = { ChimneyBodyCollision, ChimneyBodyCollision_2, ChimneyBodyCollision_3 };
	USphereComponent* RimSpheres[3] = { RimTargetSphere, RimTargetSphere_2, RimTargetSphere_3 };
	UStaticMeshComponent* SealCaps[3] = { SealCapMesh, SealCapMesh_2, SealCapMesh_3 };
	UNiagaraComponent* ToxicVFXs[3] = { ToxicSmogVFX, ToxicSmogVFX_2, ToxicSmogVFX_3 };
	UNiagaraComponent* SparkleVFXs[3] = { SealSparkleVFX, SealSparkleVFX_2, SealSparkleVFX_3 };

	const float BodyHalfHeight = (MouthHeightZ - 80.0f) * 0.5f;

	for (int32 i = 0; i < 3; ++i)
	{
		const bool bIsMouthActive = (i < ActiveMouths);
		const bool bIsThisMouthSealed = bIsSealed || bMouthSealed[i];

		if (BodyCapsules[i])
		{
			BodyCapsules[i]->SetCapsuleSize(ShaftRadius, BodyHalfHeight);
			BodyCapsules[i]->SetRelativeLocation(FVector(MouthOffsets[i].X, MouthOffsets[i].Y, BodyHalfHeight));
			BodyCapsules[i]->SetCollisionEnabled(bIsMouthActive ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
		}

		if (RimSpheres[i])
		{
			RimSpheres[i]->SetRelativeLocation(MouthOffsets[i]);
			RimSpheres[i]->SetSphereRadius(RimSphereRadius);
			RimSpheres[i]->SetCollisionResponseToAllChannels(ECR_Overlap);
			RimSpheres[i]->SetGenerateOverlapEvents(true);
			const bool bEnableRimTarget = bIsMouthActive && !bIsThisMouthSealed;
			RimSpheres[i]->SetCollisionEnabled(bEnableRimTarget ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
			RimSpheres[i]->SetVisibility(bEnableRimTarget);
		}

		if (SealCaps[i])
		{
			// One solar-panel cap for every archetype, sized to this mouth's shaft
			if (SealMeshSingle)
			{
				SealCaps[i]->SetStaticMesh(SealMeshSingle);
			}
			SealCaps[i]->SetRelativeScale3D(FVector(ShaftRadius / FMath::Max(SealMeshRadius, 1.0f)));
			SealCaps[i]->SetVisibility(bIsMouthActive && bIsThisMouthSealed);
		}

		if (ToxicVFXs[i])
		{
			UNiagaraSystem* SmogSystem = GetActiveSmogSystem();
			if (SmogSystem && ToxicVFXs[i]->GetAsset() != SmogSystem)
			{
				ToxicVFXs[i]->SetAsset(SmogSystem);
			}

			if (bIsMouthActive && !bIsThisMouthSealed)
			{
				if (!ToxicVFXs[i]->IsActive())
				{
					ToxicVFXs[i]->Activate(true);
				}
				ToxicVFXs[i]->SetVisibility(true);
			}
			else
			{
				ToxicVFXs[i]->Deactivate();
				ToxicVFXs[i]->SetVisibility(false);
			}
		}

		if (SparkleVFXs[i])
		{
			if (SealSparkleSystem && SparkleVFXs[i]->GetAsset() != SealSparkleSystem)
			{
				SparkleVFXs[i]->SetAsset(SealSparkleSystem);
			}

			if (bIsMouthActive && bIsThisMouthSealed)
			{
				if (!SparkleVFXs[i]->IsActive())
				{
					SparkleVFXs[i]->Activate(true);
				}
				SparkleVFXs[i]->SetVisibility(true);
			}
			else
			{
				SparkleVFXs[i]->Deactivate();
				SparkleVFXs[i]->SetVisibility(false);
			}
		}
	}

	// The plume (2 x LethalPlumeHalfHeight tall above the rim) is split into stacked segments that widen towards the top
	UCapsuleComponent* PlumeSegments[4] = { LethalPlumeCapsule, LethalPlumeCone_1, LethalPlumeCone_2, LethalPlumeCone_3 };
	const bool bEnableLethalPlume = (SmokestackType == ESmokestackType::HeavyLethal) && !bIsSealed;
	const float SegmentHalfHeight = LethalPlumeHalfHeight / UE_ARRAY_COUNT(PlumeSegments);
	for (int32 i = 0; i < UE_ARRAY_COUNT(PlumeSegments); ++i)
	{
		if (UCapsuleComponent* Segment = PlumeSegments[i])
		{
			const float Radius = FMath::Lerp(LethalPlumeRadius, LethalPlumeTopRadius, (i + 0.5f) / UE_ARRAY_COUNT(PlumeSegments));
			Segment->SetCapsuleSize(Radius, FMath::Max(SegmentHalfHeight, Radius));
			Segment->SetRelativeLocation(FVector(0.0f, 0.0f, SegmentHalfHeight * (2 * i + 1)));
			Segment->SetCollisionEnabled(bEnableLethalPlume ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
			Segment->SetVisibility(bEnableLethalPlume);
		}
	}

	if (OverheadSmogCanopySphere)
	{
		OverheadSmogCanopySphere->SetCollisionEnabled(bIsSealed ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryOnly);
	}

	// Sealing stops new sky smoke; existing particles drift out, opening the sun pocket gradually
	if (CanopySmokeVFX)
	{
		if (SkySmogSystem && CanopySmokeVFX->GetAsset() != SkySmogSystem)
		{
			CanopySmokeVFX->SetAsset(SkySmogSystem);
		}
		if (bIsSealed)
		{
			CanopySmokeVFX->Deactivate();
		}
		else if (!CanopySmokeVFX->IsActive())
		{
			CanopySmokeVFX->Activate(true);
		}
	}
}

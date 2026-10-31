// Copyright Do Not Smoke Team. All Rights Reserved.

#include "Smokestack.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/StaticMesh.h"
#include "NiagaraComponent.h"
#include "UObject/ConstructorHelpers.h"

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
		Comp->SetRelativeLocation(FVector(0.0f, 0.0f, 1280.0f));
		Comp->InitSphereRadius(RimSphereRadius);
		Comp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Comp->SetCollisionResponseToAllChannels(ECR_Overlap);
		Comp->SetGenerateOverlapEvents(true);
		Comp->SetHiddenInGame(false);
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
	LethalPlumeCapsule->SetHiddenInGame(false);
	LethalPlumeCapsule->ShapeColor = FColor::Red;

	OverheadSmogCanopySphere = CreateDefaultSubobject<USphereComponent>(TEXT("OverheadSmogCanopySphere"));
	OverheadSmogCanopySphere->SetupAttachment(SceneRoot);
	OverheadSmogCanopySphere->SetRelativeLocation(FVector(0.0f, 0.0f, CanopyWorldAltitude));
	OverheadSmogCanopySphere->InitSphereRadius(CanopyMinRadius);
	OverheadSmogCanopySphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	OverheadSmogCanopySphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	OverheadSmogCanopySphere->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

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

	CleanSteamVFX = InitVFX(TEXT("CleanSteamVFX"), RimTargetSphere, false);
	CleanSteamVFX_2 = InitVFX(TEXT("CleanSteamVFX_2"), RimTargetSphere_2, false);
	CleanSteamVFX_3 = InitVFX(TEXT("CleanSteamVFX_3"), RimTargetSphere_3, false);

	// Load imported Do Not Smoke modular chimney & seal cap meshes
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SingleAsset(TEXT("/Game/Game/Meshes/SM_Smokestack_Single/StaticMeshes/SM_Smokestack_Single.SM_Smokestack_Single"));
	if (SingleAsset.Succeeded())
	{
		MeshSingle = SingleAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> ClusterAsset(TEXT("/Game/Game/Meshes/SM_Smokestack_Cluster/StaticMeshes/SM_Smokestack_Cluster.SM_Smokestack_Cluster"));
	if (ClusterAsset.Succeeded())
	{
		MeshCluster = ClusterAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cluster2Asset(TEXT("/Game/Game/Meshes/SM_Smokestack_Cluster_2/StaticMeshes/SM_Smokestack_Cluster_2.SM_Smokestack_Cluster_2"));
	if (Cluster2Asset.Succeeded())
	{
		MeshCluster2 = Cluster2Asset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cluster3Asset(TEXT("/Game/Game/Meshes/SM_Smokestack_Cluster_3/StaticMeshes/SM_Smokestack_Cluster_3.SM_Smokestack_Cluster_3"));
	if (Cluster3Asset.Succeeded())
	{
		MeshCluster3 = Cluster3Asset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> HeavyAsset(TEXT("/Game/Game/Meshes/SM_Smokestack_Heavy/StaticMeshes/SM_Smokestack_Heavy.SM_Smokestack_Heavy"));
	if (HeavyAsset.Succeeded())
	{
		MeshHeavy = HeavyAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SealSingleAsset(TEXT("/Game/Game/Meshes/SM_Seal_Single/StaticMeshes/SM_Seal_Single.SM_Seal_Single"));
	if (SealSingleAsset.Succeeded())
	{
		SealMeshSingle = SealSingleAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SealClusterAsset(TEXT("/Game/Game/Meshes/SM_Seal_Cluster/StaticMeshes/SM_Seal_Cluster.SM_Seal_Cluster"));
	if (SealClusterAsset.Succeeded())
	{
		SealMeshCluster = SealClusterAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SealHeavyAsset(TEXT("/Game/Game/Meshes/SM_Seal_Heavy/StaticMeshes/SM_Seal_Heavy.SM_Seal_Heavy"));
	if (SealHeavyAsset.Succeeded())
	{
		SealMeshHeavy = SealHeavyAsset.Object;
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
		const float LocalCanopyZ = CanopyWorldAltitude - GetActorLocation().Z;
		OverheadSmogCanopySphere->SetRelativeLocation(FVector(0.0f, 0.0f, FMath::Max(1600.0f, LocalCanopyZ)));
		OverheadSmogCanopySphere->SetSphereRadius(CanopyMinRadius);
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

	RefreshArchetypeCollisionState();
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
	const float NewRadius = FMath::Lerp(CanopyMinRadius, CanopyMaxRadius, ClampedAlpha);
	OverheadSmogCanopySphere->SetSphereRadius(NewRadius);
}

bool ASmokestack::TrySealTarget_Implementation(AActor* SealInstigator)
{
	if (bIsSealed)
	{
		return false;
	}

	USphereComponent* RimSpheres[3] = { RimTargetSphere, RimTargetSphere_2, RimTargetSphere_3 };
	UNiagaraComponent* ToxicVFXs[3] = { ToxicSmogVFX, ToxicSmogVFX_2, ToxicSmogVFX_3 };
	UNiagaraComponent* CleanVFXs[3] = { CleanSteamVFX, CleanSteamVFX_2, CleanSteamVFX_3 };

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

	if (CleanVFXs[BestMouthIdx])
	{
		CleanVFXs[BestMouthIdx]->Activate(true);
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

	if (HazardComponent == LethalPlumeCapsule)
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
	// Exact mouth heights & offsets from dimensions.json:
	// Assembly_Single: 1 mouth at (0, 0, 1280)
	// Assembly_Cluster_2: 2 mouths at (-340, 0, 1320) and (340, 0, 1320)
	// Assembly_Cluster_3: 3 mouths at (-680, 0, 1320), (0, 0, 1320), and (680, 0, 1320)
	// Assembly_Heavy: 1 mouth at (0, 0, 1480)
	float MouthHeightZ = 1280.0f;
	float ShaftRadius = 275.0f;
	int32 ActiveMouths = 1;
	FVector MouthOffsets[3] = { FVector::ZeroVector, FVector::ZeroVector, FVector::ZeroVector };
	UStaticMesh* DesiredBodyMesh = MeshSingle;
	UStaticMesh* DesiredSealMesh = SealMeshSingle;

	switch (SmokestackType)
	{
	case ESmokestackType::Normal:
		MouthHeightZ = 1280.0f;
		ShaftRadius = 275.0f;
		ActiveMouths = 1;
		MouthOffsets[0] = FVector(0.0f, 0.0f, MouthHeightZ);
		DesiredBodyMesh = MeshSingle;
		DesiredSealMesh = SealMeshSingle;
		break;

	case ESmokestackType::Clustered:
		MouthHeightZ = 1320.0f;
		ShaftRadius = 265.0f;
		ActiveMouths = FMath::Clamp(ClusterStackCount, 2, 3);
		if (ActiveMouths == 3)
		{
			MouthOffsets[0] = FVector(-680.0f, 0.0f, MouthHeightZ);
			MouthOffsets[1] = FVector(0.0f, 0.0f, MouthHeightZ);
			MouthOffsets[2] = FVector(680.0f, 0.0f, MouthHeightZ);
			DesiredBodyMesh = MeshCluster3 ? MeshCluster3 : MeshCluster;
		}
		else
		{
			MouthOffsets[0] = FVector(-340.0f, 0.0f, MouthHeightZ);
			MouthOffsets[1] = FVector(340.0f, 0.0f, MouthHeightZ);
			DesiredBodyMesh = MeshCluster2 ? MeshCluster2 : MeshCluster;
		}
		DesiredSealMesh = SealMeshCluster ? SealMeshCluster : SealMeshSingle;
		break;

	case ESmokestackType::HeavyLethal:
		MouthHeightZ = 1480.0f;
		ShaftRadius = 315.0f;
		ActiveMouths = 1;
		MouthOffsets[0] = FVector(0.0f, 0.0f, MouthHeightZ);
		DesiredBodyMesh = MeshHeavy;
		DesiredSealMesh = SealMeshHeavy;
		break;
	}

	if (ChimneyBodyMesh && DesiredBodyMesh)
	{
		ChimneyBodyMesh->SetStaticMesh(DesiredBodyMesh);
	}

	UCapsuleComponent* BodyCapsules[3] = { ChimneyBodyCollision, ChimneyBodyCollision_2, ChimneyBodyCollision_3 };
	USphereComponent* RimSpheres[3] = { RimTargetSphere, RimTargetSphere_2, RimTargetSphere_3 };
	UStaticMeshComponent* SealCaps[3] = { SealCapMesh, SealCapMesh_2, SealCapMesh_3 };

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
			if (DesiredSealMesh)
			{
				SealCaps[i]->SetStaticMesh(DesiredSealMesh);
			}
			SealCaps[i]->SetVisibility(bIsMouthActive && bIsThisMouthSealed);
		}
	}

	if (LethalPlumeCapsule)
	{
		LethalPlumeCapsule->SetCapsuleSize(LethalPlumeRadius, LethalPlumeHalfHeight);
		LethalPlumeCapsule->SetRelativeLocation(FVector(0.0f, 0.0f, LethalPlumeHalfHeight));
		const bool bEnableLethalPlume = (SmokestackType == ESmokestackType::HeavyLethal) && !bIsSealed;
		LethalPlumeCapsule->SetCollisionEnabled(bEnableLethalPlume ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
		LethalPlumeCapsule->SetVisibility(bEnableLethalPlume);
	}

	if (OverheadSmogCanopySphere)
	{
		OverheadSmogCanopySphere->SetCollisionEnabled(bIsSealed ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryOnly);
	}
}

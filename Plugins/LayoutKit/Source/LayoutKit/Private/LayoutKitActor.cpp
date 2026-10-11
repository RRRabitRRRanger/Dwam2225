#include "LayoutKitActor.h"

#include "Components/DynamicMeshComponent.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"
#include "Engine/CollisionProfile.h"
#include "LayoutKitBuilder.h"
#include "LayoutKitTypes.h"
#include "Materials/MaterialInterface.h"

DEFINE_LOG_CATEGORY_STATIC(LogLayoutKit, Log, All);

using UE::Geometry::FDynamicMesh3;

ALayoutKitActor::ALayoutKitActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Mesh = CreateDefaultSubobject<UDynamicMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);

	InitExample();
}

void ALayoutKitActor::ResetToExample()
{
	Modify();
	InitExample();
	Rebuild();
}

void ALayoutKitActor::InitExample()
{
	LayoutKit::MakeExampleHouse(Layout, RootBox);
}

void ALayoutKitActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Rebuild();
}

void ALayoutKitActor::Rebuild()
{
	Problems.Reset();
	Layout.Validate(Problems);

	// L'échelle de l'acteur = la taille demandée ; le mesh, lui, ne doit pas être étiré
	const FVector Scale = GetActorScale3D();
	const FVector SafeScale(
		FMath::Max(FMath::Abs(Scale.X), 0.01),
		FMath::Max(FMath::Abs(Scale.Y), 0.01),
		FMath::Max(FMath::Abs(Scale.Z), 0.01));
	Mesh->SetRelativeScale3D(FVector(1.0 / SafeScale.X, 1.0 / SafeScale.Y, 1.0 / SafeScale.Z));

	TArray<FLayoutKitPlacedBox> Boxes;
	LayoutKit::SolveLayout(Layout, RootBox, SafeScale, Boxes, Problems, LOA);

	FDynamicMesh3 Result;
	Result.EnableAttributes();
	TArray<FVector2D> Points;
	TArray<bool> Sharp;
	for (const FLayoutKitPlacedBox& Box : Boxes)
	{
		LayoutKit::BuildFootprint(Box, Points, Sharp);
		LayoutKit::AppendPrism(Result, Points, Sharp, 0.0, Box.Height);
	}

	Mesh->SetMesh(MoveTemp(Result));
	if (Material)
	{
		Mesh->SetMaterial(0, Material);
	}

	if (bGenerateCollision)
	{
		Mesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		Mesh->SetComplexAsSimpleCollisionEnabled(true, true);
	}
	else
	{
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetComplexAsSimpleCollisionEnabled(false, true);
	}

	for (const FString& Problem : Problems)
	{
		UE_LOG(LogLayoutKit, Warning, TEXT("%s : %s"), *GetName(), *Problem);
	}
}

void ALayoutKitActor::SetLOA(float NewLOA)
{
	LOA = FMath::Max(NewLOA, 0.f);
	Rebuild();
}

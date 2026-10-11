#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"
#include "GraphKitTypes.h"
#include "LayoutKitBuilder.h"
#include "LayoutKitTypes.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// Lancer : Session Frontend > Automation > « LayoutKit », ou
//   UnrealEditor-Cmd.exe <projet> -ExecCmds="Automation RunTests LayoutKit; Quit" -nullrhi -unattended

namespace LayoutKitTestPrivate
{
	/** Le corps et son entrée, comme l'exemple de l'acteur. */
	FGraphKitGraph MakeHouse(ELayoutKitSide Side = ELayoutKitSide::Front, float Alignment = 0.5f)
	{
		FGraphKitGraph G;
		G.bDirected = true;

		FLayoutKitBox Body;
		Body.Width = FLayoutKitLength(1000.f, ELayoutKitSizeRule::Stretch);
		Body.Depth = FLayoutKitLength(800.f, ELayoutKitSizeRule::Stretch);
		Body.Height = FLayoutKitLength(600.f, ELayoutKitSizeRule::Stretch);
		Body.BackLeftRadius = 300.f;
		Body.BackRightRadius = 300.f;
		G.AddNode(TEXT("Corps"), FGameplayTagContainer(), FInstancedStruct::Make(Body));

		FLayoutKitBox Entrance;
		Entrance.Width = FLayoutKitLength(300.f, ELayoutKitSizeRule::Fixed);
		Entrance.Depth = FLayoutKitLength(250.f, ELayoutKitSizeRule::Fixed);
		Entrance.Height = FLayoutKitLength(350.f, ELayoutKitSizeRule::Fixed);
		G.AddNode(TEXT("Entrée"), FGameplayTagContainer(), FInstancedStruct::Make(Entrance));

		FLayoutKitAttach Attach;
		Attach.Side = Side;
		Attach.Alignment = Alignment;
		Attach.Overlap = 20.f;
		G.AddEdge(TEXT("Corps"), TEXT("Entrée"), 1.f, FGameplayTagContainer(), FInstancedStruct::Make(Attach));
		return G;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLayoutKitRadiiTest, "LayoutKit.ClampRadii",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLayoutKitRadiiTest::RunTest(const FString& Parameters)
{
	// Boîte 400 × 200 : rayon max = 100 ; à l'avant (largeur 400), 300 + 300 > 400 -> réduits
	float Radii[4] = { 300.f, 300.f, 50.f, 0.f };
	LayoutKit::ClampRadii(400.f, 200.f, Radii);
	TestEqual(TEXT("Avant-gauche limité à la moitié du petit côté"), Radii[0], 100.f, 0.01f);
	TestEqual(TEXT("Avant-droite limité aussi"), Radii[1], 100.f, 0.01f);
	TestEqual(TEXT("Arrière-droite inchangé"), Radii[2], 50.f, 0.01f);
	TestTrue(TEXT("Côté droit (profondeur 200) : 100 + 50 tient"), Radii[1] + Radii[2] <= 200.f + 0.01f);

	float Tight[4] = { 80.f, 80.f, 80.f, 80.f };
	LayoutKit::ClampRadii(100.f, 300.f, Tight);
	TestTrue(TEXT("Avant (largeur 100) : la somme tient"), Tight[0] + Tight[1] <= 100.f + 0.01f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLayoutKitFootprintTest, "LayoutKit.Footprint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLayoutKitFootprintTest::RunTest(const FString& Parameters)
{
	// Boîte vive : 4 points ; boîte avec 2 arrondis de 4 segments : 2 + 2 × 5 points
	FLayoutKitPlacedBox Box;
	Box.Width = 1000.f;
	Box.Depth = 800.f;
	Box.Height = 600.f;
	Box.Segments = 4;

	TArray<FVector2D> Points;
	TArray<bool> Sharp;
	LayoutKit::BuildFootprint(Box, Points, Sharp);
	TestEqual(TEXT("Boîte vive : 4 coins"), Points.Num(), 4);

	Box.Radii[2] = 300.f;
	Box.Radii[3] = 300.f;
	LayoutKit::BuildFootprint(Box, Points, Sharp);
	TestEqual(TEXT("Deux arrondis de 4 segments"), Points.Num(), 12);

	// Convexe et dans le sens trigonométrique : tous les produits vectoriels positifs
	bool bConvex = true;
	for (int32 I = 0; I < Points.Num(); ++I)
	{
		const FVector2D A = Points[I];
		const FVector2D B = Points[(I + 1) % Points.Num()];
		const FVector2D C = Points[(I + 2) % Points.Num()];
		bConvex &= FVector2D::CrossProduct(B - A, C - B) >= -1e-3;
	}
	TestTrue(TEXT("Contour convexe, sens trigonométrique"), bConvex);

	// Tous les points dans la boîte
	bool bInside = true;
	for (const FVector2D& P : Points)
	{
		bInside &= FMath::Abs(P.X) <= 400.01 && FMath::Abs(P.Y) <= 500.01;
	}
	TestTrue(TEXT("Contour dans la boîte"), bInside);

	// Deux arrondis à 400 sur la largeur 800 de l'arrière : ils se rejoignent, sans point en double
	Box.Width = 800.f;
	Box.Depth = 800.f;
	Box.Radii[2] = 400.f;
	Box.Radii[3] = 400.f;
	LayoutKit::BuildFootprint(Box, Points, Sharp);
	bool bNoDuplicate = true;
	for (int32 I = 0; I < Points.Num(); ++I)
	{
		bNoDuplicate &= FVector2D::Distance(Points[I], Points[(I + 1) % Points.Num()]) > 0.01;
	}
	TestTrue(TEXT("Pas de point en double quand deux arrondis se rejoignent"), bNoDuplicate);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLayoutKitPlacementTest, "LayoutKit.Placement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLayoutKitPlacementTest::RunTest(const FString& Parameters)
{
	using namespace LayoutKitTestPrivate;

	// Entrée à l'avant, centrée : centre à x = 400 + 250 / 2 - 20 = 505, y = 0
	TArray<FLayoutKitPlacedBox> Boxes;
	TArray<FString> Problems;
	LayoutKit::SolveLayout(MakeHouse(), TEXT("Corps"), FVector::OneVector, Boxes, Problems);
	TestEqual(TEXT("Deux boîtes"), Boxes.Num(), 2);
	TestEqual(TEXT("Aucun problème"), Problems.Num(), 0);
	if (Boxes.Num() == 2)
	{
		TestEqual(TEXT("Entrée : x"), Boxes[1].Center.X, 505.0, 0.01);
		TestEqual(TEXT("Entrée : y"), Boxes[1].Center.Y, 0.0, 0.01);
		TestEqual(TEXT("Entrée : même orientation"), Boxes[1].Quarter, 0);
	}

	// Entrée à droite : tournée d'un quart, centre à y = 500 + 105
	LayoutKit::SolveLayout(MakeHouse(ELayoutKitSide::Right), TEXT("Corps"), FVector::OneVector, Boxes, Problems);
	if (Boxes.Num() == 2)
	{
		TestEqual(TEXT("Droite : x"), Boxes[1].Center.X, 0.0, 0.01);
		TestEqual(TEXT("Droite : y"), Boxes[1].Center.Y, 605.0, 0.01);
		TestEqual(TEXT("Droite : un quart de tour"), Boxes[1].Quarter, 1);
	}

	// Alignement au début du côté avant (alignement 0) : l'entrée touche le bord gauche
	LayoutKit::SolveLayout(MakeHouse(ELayoutKitSide::Front, 0.f), TEXT("Corps"), FVector::OneVector, Boxes, Problems);
	if (Boxes.Num() == 2)
	{
		TestEqual(TEXT("Alignement 0 : y = -(1000 - 300) / 2"), Boxes[1].Center.Y, -350.0, 0.01);
	}

	// Racine inconnue : signalée
	Problems.Reset();
	LayoutKit::SolveLayout(MakeHouse(), TEXT("Tour"), FVector::OneVector, Boxes, Problems);
	TestEqual(TEXT("Racine inconnue : aucune boîte"), Boxes.Num(), 0);
	TestEqual(TEXT("Racine inconnue : un problème"), Problems.Num(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLayoutKitScaleTest, "LayoutKit.FixedAndStretch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLayoutKitScaleTest::RunTest(const FString& Parameters)
{
	using namespace LayoutKitTestPrivate;

	// Échelle × 2 : le corps (étirable) double, l'entrée (fixe) garde sa taille et reste collée à l'avant
	TArray<FLayoutKitPlacedBox> Boxes;
	TArray<FString> Problems;
	LayoutKit::SolveLayout(MakeHouse(), TEXT("Corps"), FVector(2.0), Boxes, Problems);
	if (Boxes.Num() == 2)
	{
		TestEqual(TEXT("Corps : largeur doublée"), Boxes[0].Width, 2000.f, 0.01f);
		TestEqual(TEXT("Corps : hauteur doublée"), Boxes[0].Height, 1200.f, 0.01f);
		TestEqual(TEXT("Entrée : largeur fixe"), Boxes[1].Width, 300.f, 0.01f);
		TestEqual(TEXT("Entrée : hauteur fixe"), Boxes[1].Height, 350.f, 0.01f);
		TestEqual(TEXT("Entrée : toujours collée à l'avant (800 + 105)"), Boxes[1].Center.X, 905.0, 0.01);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLayoutKitPrismTest, "LayoutKit.Prism",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLayoutKitPrismTest::RunTest(const FString& Parameters)
{
	using namespace LayoutKitTestPrivate;
	using UE::Geometry::FDynamicMesh3;

	TArray<FLayoutKitPlacedBox> Boxes;
	TArray<FString> Problems;
	LayoutKit::SolveLayout(MakeHouse(), TEXT("Corps"), FVector::OneVector, Boxes, Problems);

	bool bAllClosed = true;
	bool bAllFacingOut = true;
	for (const FLayoutKitPlacedBox& Box : Boxes)
	{
		FDynamicMesh3 Mesh;
		Mesh.EnableAttributes();
		TArray<FVector2D> Points;
		TArray<bool> Sharp;
		LayoutKit::BuildFootprint(Box, Points, Sharp);
		LayoutKit::AppendPrism(Mesh, Points, Sharp, 0.0, Box.Height);

		bAllClosed &= Mesh.IsClosed() && Mesh.TriangleCount() == 4 * Points.Num();

		// Chaque face regarde vers l'extérieur : sa normale s'éloigne du centre du prisme
		const FVector3d Center(Box.Center.X, Box.Center.Y, 0.5 * Box.Height);
		for (const int32 Tid : Mesh.TriangleIndicesItr())
		{
			const FVector3d Centroid = Mesh.GetTriCentroid(Tid);
			bAllFacingOut &= Mesh.GetTriNormal(Tid).Dot(Centroid - Center) > 0.0;
		}
	}
	TestTrue(TEXT("Prismes fermés, 4 triangles par point du contour"), bAllClosed);
	TestTrue(TEXT("Toutes les faces regardent vers l'extérieur"), bAllFacingOut);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLayoutKitLOATest, "LayoutKit.LOA",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLayoutKitLOATest::RunTest(const FString& Parameters)
{
	// L'exemple : corps, entrée (toujours), terrasse (jusqu'au LOA 0, s'efface), grange (jusqu'au LOA 1, absorbée)
	FGraphKitGraph House;
	FName Root;
	LayoutKit::MakeExampleHouse(House, Root);

	auto Solve = [&](float Level, TArray<FLayoutKitPlacedBox>& Boxes)
	{
		TArray<FString> Problems;
		LayoutKit::SolveLayout(House, Root, FVector::OneVector, Boxes, Problems, Level);
		return Problems.Num();
	};
	auto Find = [](const TArray<FLayoutKitPlacedBox>& Boxes, const TCHAR* Name) -> const FLayoutKitPlacedBox*
	{
		return Boxes.FindByPredicate([Name](const FLayoutKitPlacedBox& B) { return B.Name == FName(Name); });
	};

	TArray<FLayoutKitPlacedBox> Boxes;
	TestEqual(TEXT("Exemple sans problème"), Solve(0.f, Boxes), 0);
	TestEqual(TEXT("LOA 0 : quatre boîtes"), Boxes.Num(), 4);

	Solve(0.5f, Boxes);
	if (const FLayoutKitPlacedBox* Terrace = Find(Boxes, TEXT("Terrasse")))
	{
		TestEqual(TEXT("LOA 0,5 : la terrasse à moitié repliée (largeur)"), Terrace->Width, 250.f, 0.01f);
		TestEqual(TEXT("LOA 0,5 : la terrasse à moitié repliée (hauteur)"), Terrace->Height, 20.f, 0.01f);
	}
	else
	{
		AddError(TEXT("LOA 0,5 : la terrasse devrait encore exister"));
	}

	Solve(1.f, Boxes);
	TestEqual(TEXT("LOA 1 : la terrasse a disparu"), Boxes.Num(), 3);
	TestNull(TEXT("LOA 1 : plus de terrasse"), Find(Boxes, TEXT("Terrasse")));

	Solve(1.5f, Boxes);
	if (const FLayoutKitPlacedBox* Barn = Find(Boxes, TEXT("Grange")))
	{
		TestEqual(TEXT("LOA 1,5 : la grange s'élargit vers le corps"), Barn->Width, 900.f, 0.01f);
		TestEqual(TEXT("LOA 1,5 : la grange monte vers le corps"), Barn->Height, 525.f, 0.01f);
	}

	Solve(2.f, Boxes);
	const FLayoutKitPlacedBox* Body = Find(Boxes, TEXT("Corps"));
	const FLayoutKitPlacedBox* Barn = Find(Boxes, TEXT("Grange"));
	if (Body && Barn)
	{
		TestEqual(TEXT("LOA 2 : la grange a la largeur du corps"), Barn->Width, Body->Width, 0.01f);
		TestEqual(TEXT("LOA 2 : la grange a la hauteur du corps"), Barn->Height, Body->Height, 0.01f);
		TestEqual(TEXT("LOA 2 : coins vifs"), Barn->Radii[0] + Barn->Radii[1] + Barn->Radii[2] + Barn->Radii[3], 0.f, 0.01f);
	}
	TestNotNull(TEXT("LOA 2 : l'entrée est toujours là"), Find(Boxes, TEXT("Entrée")));

	Solve(4.f, Boxes);
	TestEqual(TEXT("LOA 4 : corps, entrée, et la grange fondue dans le corps"), Boxes.Num(), 3);
	return true;
}

#endif

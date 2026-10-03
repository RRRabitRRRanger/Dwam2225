#include "WatershedHierarchy.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// Petits terrains dont les niveaux se calculent à la main (aire des cellules = 1).
// Lancer : Session Frontend > Automation > « Watershed », ou en ligne de commande
//   UnrealEditor-Cmd.exe Dwam2225.uproject -ExecCmds="Automation RunTests Watershed; Quit" -nullrhi -unattended

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWatershedTwoPitsClosedTest, "Watershed.Hierarchy.TwoPitsClosed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWatershedTwoPitsClosedTest::RunTest(const FString& Parameters)
{
	// Coupe 1D, bords fermés :  5 | 0 | 3 | 1 | 5   -> creux A (cellule 1) et B (cellule 3), col à 3
	FWatershedGridDesc Desc;
	Desc.SizeX = 5;
	Desc.SizeY = 1;
	Desc.CellSize = 1.0;
	Desc.bEdgesDrain = false;
	const TArray<float> Heights = { 5.f, 0.f, 3.f, 1.f, 5.f };

	FWatershedHierarchy H;
	H.Build(Desc, Heights);

	const int32 A = H.GetLabel(1);
	const int32 B = H.GetLabel(3);
	TestNotEqual(TEXT("Deux creux distincts"), A, B);
	TestEqual(TEXT("Col A"), H.GetNodes()[A].OutletZ, 3.0);
	TestEqual(TEXT("Capacité A (3 - 0)"), H.GetNodes()[A].Capacity, 3.0);
	TestEqual(TEXT("Capacité B (3 - 1)"), H.GetNodes()[B].Capacity, 2.0);

	// Fill
	H.AddWater(1, 2.0);
	TestEqual(TEXT("A à moitié : niveau 2"), H.GetLevel(H.GetLakeOfCell(1)), 2.0, 0.02);

	// Spill : A plein, le surplus (1) passe dans B
	H.AddWater(1, 2.0);
	TestEqual(TEXT("A plein : niveau 3"), H.GetLevel(H.GetLakeOfCell(1)), 3.0, 0.02);
	TestEqual(TEXT("B reçoit 1 : niveau 2"), H.GetLevel(H.GetLakeOfCell(3)), 2.0, 0.02);

	// Merge : B plein, un seul lac. Volume total 6 : 3h - (0 + 3 + 1) = 6 -> h = 10/3
	H.AddWater(3, 2.0);
	const int32 LakeA = H.GetLakeOfCell(1);
	TestEqual(TEXT("Un seul lac"), LakeA, H.GetLakeOfCell(3));
	TestEqual(TEXT("Niveau fusionné 10/3"), H.GetLevel(LakeA), 10.0 / 3.0, 0.02);
	TestEqual(TEXT("Rien de perdu"), H.GetLostVolume(), 0.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWatershedDrainTest, "Watershed.Hierarchy.EdgesDrain",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWatershedDrainTest::RunTest(const FString& Parameters)
{
	// 5 × 3, bords à 4 (évacuent). Rangée du milieu : 0 | 2 | 1
	FWatershedGridDesc Desc;
	Desc.SizeX = 5;
	Desc.SizeY = 3;
	Desc.CellSize = 1.0;
	Desc.bEdgesDrain = true;
	TArray<float> Heights;
	Heights.Init(4.f, 15);
	Heights[1 + 5] = 0.f;
	Heights[2 + 5] = 2.f;
	Heights[3 + 5] = 1.f;

	FWatershedHierarchy H;
	H.Build(Desc, Heights);

	const int32 Lake = H.GetLakeOfCell(1 + 5);
	TestTrue(TEXT("Cellule intérieure dans un bassin"), Lake != INDEX_NONE);
	TestEqual(TEXT("Bord : extérieur"), H.GetLakeOfCell(0), int32(INDEX_NONE));

	// Capacité totale sous le col extérieur (4) : (4-0) + (4-2) + (4-1) = 9. On verse 20 : 11 perdus.
	const double Lost = H.AddWater(1 + 5, 20.0);
	TestEqual(TEXT("Volume perdu"), Lost, 11.0, 1e-6);
	const int32 Merged = H.GetLakeOfCell(1 + 5);
	TestEqual(TEXT("Lac fusionné"), Merged, H.GetLakeOfCell(3 + 5));
	TestEqual(TEXT("Niveau au col"), H.GetLevel(Merged), 4.0, 0.02);

	// Eau versée sur un bord : perdue
	TestEqual(TEXT("Bord : tout est perdu"), H.AddWater(0, 5.0), 5.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWatershedFlatTest, "Watershed.Hierarchy.FlatClosed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWatershedFlatTest::RunTest(const FString& Parameters)
{
	// Plaine plate fermée 4 × 4 : un seul creux (plateau regroupé), capacité infinie
	FWatershedGridDesc Desc;
	Desc.SizeX = 4;
	Desc.SizeY = 4;
	Desc.CellSize = 1.0;
	Desc.bEdgesDrain = false;
	TArray<float> Heights;
	Heights.Init(0.f, 16);

	FWatershedHierarchy H;
	H.Build(Desc, Heights);
	TestEqual(TEXT("Un seul creux (+ extérieur)"), H.GetNumLeaves(), 2);

	H.AddWater(5, 16.0);
	TestEqual(TEXT("16 sur 16 cellules : niveau 1"), H.GetLevel(H.GetLakeOfCell(0)), 1.0, 0.02);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWatershedNestedTest, "Watershed.Hierarchy.Nested",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWatershedNestedTest::RunTest(const FString& Parameters)
{
	// Trois creux fermés : 9 | 0 | 2 | 1 | 5 | 0 | 9
	// A (cellule 1) et B (cellule 3) fusionnent à 2 ; puis AB et C (cellule 5) à 5.
	FWatershedGridDesc Desc;
	Desc.SizeX = 7;
	Desc.SizeY = 1;
	Desc.CellSize = 1.0;
	Desc.bEdgesDrain = false;
	const TArray<float> Heights = { 9.f, 0.f, 2.f, 1.f, 5.f, 0.f, 9.f };

	FWatershedHierarchy H;
	H.Build(Desc, Heights);

	// Verser dans C : il se remplit jusqu'à 5 (capacité 5), le surplus passe côté AB
	H.AddWater(5, 6.0);
	TestEqual(TEXT("C plein"), H.GetLevel(H.GetLakeOfCell(5)), 5.0, 0.02);
	// AB reçoit 1 : il entre dans B (côté du col), B : capacité 1 (2 - 1) -> plein, rien dans A
	TestEqual(TEXT("B plein à 2"), H.GetLevel(H.GetLakeOfCell(3)), 2.0, 0.02);
	TestEqual(TEXT("A vide"), H.GetLevel(H.GetLakeOfCell(1)), 0.0, 0.02);

	// Encore 2.5 dans C : traverse B (plein), remplit A (2), le dernier 0.5 déborde de A vers B plein :
	// fusion AB. Volume 3.5 sur les cellules 0, 2, 1 : 3h - 3 = 3.5 -> h = 6.5 / 3
	H.AddWater(5, 2.5);
	const int32 AB = H.GetLakeOfCell(1);
	TestEqual(TEXT("AB fusionné"), AB, H.GetLakeOfCell(3));
	TestEqual(TEXT("Niveau AB"), H.GetLevel(AB), 6.5 / 3.0, 0.02);
	TestNotEqual(TEXT("C encore séparé"), AB, H.GetLakeOfCell(5));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWatershedConservationTest, "Watershed.Hierarchy.ConservationLarge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWatershedConservationTest::RunTest(const FString& Parameters)
{
	// Terrain bruité 512 × 512 (cellules de 2 m) : beaucoup de creux, d'imbrications et de plateaux.
	// Vérifie que l'eau est conservée (lacs + perdu = versé) et mesure les temps.
	FWatershedGridDesc Desc;
	Desc.SizeX = 512;
	Desc.SizeY = 512;
	Desc.CellSize = 200.0;
	Desc.bEdgesDrain = true;

	TArray<float> Heights;
	Heights.SetNumUninitialized(Desc.Num());
	FRandomStream Random(1234);
	for (int32 Y = 0; Y < Desc.SizeY; ++Y)
	{
		for (int32 X = 0; X < Desc.SizeX; ++X)
		{
			const float H = 800.f * FMath::Sin(X * 0.031f) * FMath::Cos(Y * 0.027f)
				+ 300.f * FMath::Sin(X * 0.11f + Y * 0.07f)
				+ Random.FRandRange(0.f, 60.f);
			// Quelques plateaux arrondis (hauteurs égales)
			Heights[X + Y * Desc.SizeX] = FMath::RoundToFloat(H / 20.f) * 20.f;
		}
	}

	FWatershedHierarchy H;
	double Start = FPlatformTime::Seconds();
	H.Build(Desc, Heights);
	const double BuildMs = (FPlatformTime::Seconds() - Start) * 1000.0;

	// 50 mm de pluie partout
	const double PerCell = Desc.CellSize * Desc.CellSize * 5.0;
	Start = FPlatformTime::Seconds();
	for (int32 Cell = 0; Cell < Desc.Num(); ++Cell)
	{
		H.AddWater(Cell, PerCell);
	}
	const double RainMs = (FPlatformTime::Seconds() - Start) * 1000.0;

	double InLakes = 0.0;
	int32 WetLakes = 0;
	for (const FWatershedHierarchy::FNode& Node : H.GetNodes())
	{
		if (Node.bActive)
		{
			InLakes += Node.Water;
			WetLakes += Node.Water > 0.0 ? 1 : 0;
		}
	}
	const double Total = PerCell * Desc.Num();
	const double Error = FMath::Abs(InLakes + H.GetLostVolume() - Total) / Total;

	// Niveaux de tous les lacs mouillés (coût de la dichotomie)
	Start = FPlatformTime::Seconds();
	bool bLevelsOk = true;
	for (int32 I = 1; I < H.GetNodes().Num(); ++I)
	{
		const FWatershedHierarchy::FNode& Node = H.GetNodes()[I];
		if (Node.bActive && Node.Water > 0.0)
		{
			const double Level = H.GetLevel(I);
			bLevelsOk &= Level >= Node.MinZ - 0.01 && (Node.OutletZ == TNumericLimits<double>::Max() || Level <= Node.OutletZ + 0.01);
		}
	}
	const double LevelsMs = (FPlatformTime::Seconds() - Start) * 1000.0;

	AddInfo(FString::Printf(TEXT("%d creux, %d nœuds, %d lacs mouillés. Construction %.1f ms, pluie %.1f ms, niveaux %.1f ms. Erreur de conservation %.2e"),
		H.GetNumLeaves() - 1, H.GetNodes().Num() - 1, WetLakes, BuildMs, RainMs, LevelsMs, Error));
	TestTrue(TEXT("Eau conservée (lacs + perdu = versé)"), Error < 1e-9);
	TestTrue(TEXT("Chaque niveau entre le fond et le col"), bLevelsOk);
	TestTrue(TEXT("Des lacs se forment"), WetLakes > 0);
	return true;
}

#endif

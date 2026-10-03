#include "RopeSolver.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// Lancer : Session Frontend > Automation > « Rope », ou
//   UnrealEditor-Cmd.exe Dwam2225.uproject -ExecCmds="Automation RunTests Rope; Quit" -nullrhi -unattended

namespace RopeTestPrivate
{
	constexpr float SubDt = 1.f / 120.f;

	FRopeSolverParams DefaultParams()
	{
		FRopeSolverParams Params;
		Params.Iterations = 32;
		Params.NodeRadius = 2.f;
		Params.Friction = 0.5f;
		return Params;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRopeHangingTest, "Rope.Solver.HangingBetweenPins",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRopeHangingTest::RunTest(const FString& Parameters)
{
	using namespace RopeTestPrivate;

	// Corde de 3 m entre deux points écartés de 2 m : elle doit pendre sans s'étirer
	FRopeSolver Solver;
	const FVector A(0.f, 0.f, 0.f);
	const FVector B(200.f, 0.f, 0.f);
	Solver.Init(A, B, 30, 300.f);
	const FRopeSolverParams Params = DefaultParams();

	for (int32 Step = 0; Step < 600; ++Step)
	{
		Solver.SetPin(0, true, A);
		Solver.SetPin(30, true, B);
		Solver.Step(SubDt, Params, {});
	}

	float Lowest = 0.f;
	for (const FVector& P : Solver.GetPositions())
	{
		Lowest = FMath::Min(Lowest, float(P.Z));
	}
	const float Stretch = Solver.GetCurrentLength() / Solver.GetTotalRestLength();
	AddInfo(FString::Printf(TEXT("Étirement %.4f, point le plus bas %.1f cm"), Stretch, Lowest));
	TestTrue(TEXT("Longueur conservée à 2 % près"), FMath::Abs(Stretch - 1.f) < 0.02f);
	TestTrue(TEXT("La corde pend (flèche > 50 cm)"), Lowest < -50.f);
	TestTrue(TEXT("Extrémités aux épingles"), Solver.GetPositions()[0].Equals(A, 0.01) && Solver.GetPositions()[30].Equals(B, 0.01));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRopeDrapeTest, "Rope.Solver.DrapeOverCapsule",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRopeDrapeTest::RunTest(const FString& Parameters)
{
	using namespace RopeTestPrivate;

	// Barre horizontale (capsule couchée selon Y, rayon 20) ; corde libre lâchée au-dessus
	FRopeShape Bar;
	Bar.Type = ERopeShapeType::Capsule;
	Bar.Transform = FTransform(FQuat(FVector::XAxisVector, PI / 2.f), FVector::ZeroVector);
	Bar.Radius = 20.f;
	Bar.HalfHeight = 100.f;

	FRopeSolver Solver;
	Solver.Init(FVector(-100.f, 0.f, 40.f), FVector(100.f, 0.f, 40.f), 40, 200.f);
	const FRopeSolverParams Params = DefaultParams();
	const FRopeShape Shapes[] = { Bar };

	for (int32 Step = 0; Step < 360; ++Step)
	{
		Solver.Step(SubDt, Params, Shapes);
	}

	float WorstPenetration = 0.f;
	for (const FVector& P : Solver.GetPositions())
	{
		FVector Normal;
		WorstPenetration = FMath::Max(WorstPenetration, Params.NodeRadius - Bar.SignedDistance(P, Normal));
	}
	const TArray<FVector>& Nodes = Solver.GetPositions();
	const float MiddleZ = Nodes[20].Z;
	AddInfo(FString::Printf(TEXT("Pénétration max %.2f cm, milieu à z = %.1f, bouts à z = %.1f / %.1f"),
		WorstPenetration, MiddleZ, Nodes[0].Z, Nodes.Last().Z));
	TestTrue(TEXT("Ne traverse pas la barre (< 0,5 cm)"), WorstPenetration < 0.5f);
	TestTrue(TEXT("Le milieu repose sur la barre"), FMath::Abs(MiddleZ - 22.f) < 3.f);
	TestTrue(TEXT("Les bouts pendent de part et d'autre"), Nodes[0].Z < 0.f && Nodes.Last().Z < 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRopeWrapTest, "Rope.Solver.WrapTiedAndTightened",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRopeWrapTest::RunTest(const FString& Parameters)
{
	using namespace RopeTestPrivate;

	// Poteau vertical (rayon 20) ; corde en hélice de 3 tours, bouts attachés, serrée à 97 %, gravité.
	// Trop courte pour ses 3 tours : elle doit rester tendue, plaquée, stable (sans vibrer ni exploser).
	FRopeShape Post;
	Post.Type = ERopeShapeType::Capsule;
	Post.Transform = FTransform::Identity;
	Post.Radius = 20.f;
	Post.HalfHeight = 200.f;

	const int32 NumNodes = 61;
	const float Turns = 3.f;
	const float Spacing = 6.f;
	const float NodesPerTurn = (NumNodes - 1) / Turns;
	const float HelixRadius = (Post.Radius + 2.f) / FMath::Cos(PI / NodesPerTurn);
	TArray<FVector> Points;
	for (int32 I = 0; I < NumNodes; ++I)
	{
		const float T = float(I) / (NumNodes - 1);
		const float Angle = 2.f * PI * Turns * T;
		Points.Add(FVector(FMath::Cos(Angle) * HelixRadius, FMath::Sin(Angle) * HelixRadius, (T - 0.5f) * Turns * Spacing));
	}

	FRopeSolver Solver;
	Solver.InitFromPoints(Points);
	const float InitialLength = Solver.GetTotalRestLength();
	FRopeSolverParams Params = DefaultParams();
	Params.Friction = 0.8f;
	Params.bSelfCollision = true;
	const FRopeShape Shapes[] = { Post };
	const FVector StartPin = Points[0];
	const FVector EndPin = Points.Last();

	TArray<FVector> Before;
	for (int32 Step = 0; Step < 360; ++Step)
	{
		const float Alpha = FMath::Min(1.f, Step / 120.f);
		Solver.SetTotalRestLength(InitialLength * FMath::Lerp(1.f, 0.97f, Alpha));
		Solver.SetPin(0, true, StartPin);
		Solver.SetPin(NumNodes - 1, true, EndPin);
		Before = Solver.GetPositions();
		Solver.Step(SubDt, Params, Shapes);
	}

	float MaxRadial = 0.f;
	float WorstPenetration = 0.f;
	float MaxSpeed = 0.f;
	bool bFinite = true;
	const TArray<FVector>& Nodes = Solver.GetPositions();
	for (int32 I = 0; I < NumNodes; ++I)
	{
		const FVector& P = Nodes[I];
		bFinite &= !P.ContainsNaN();
		MaxRadial = FMath::Max(MaxRadial, float(FVector2D(P.X, P.Y).Size()));
		FVector Normal;
		WorstPenetration = FMath::Max(WorstPenetration, Params.NodeRadius - Post.SignedDistance(P, Normal));
		MaxSpeed = FMath::Max(MaxSpeed, float(FVector::Distance(P, Before[I]) / SubDt));
	}
	AddInfo(FString::Printf(TEXT("Rayon max %.1f cm, pénétration max %.2f cm, étirement %.3f, vitesse max finale %.1f cm/s"),
		MaxRadial, WorstPenetration, Solver.GetCurrentLength() / Solver.GetTotalRestLength(), MaxSpeed));
	TestTrue(TEXT("Positions finies"), bFinite);
	TestTrue(TEXT("Reste plaquée (tous les nœuds à moins de 26 cm de l'axe)"), MaxRadial < 26.f);
	TestTrue(TEXT("Ne traverse pas le poteau (< 0,5 cm)"), WorstPenetration < 0.5f);
	TestTrue(TEXT("Stable (vitesse finale < 20 cm/s)"), MaxSpeed < 20.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRopeFrictionTest, "Rope.Solver.FrictionOnSlope",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRopeFrictionTest::RunTest(const FString& Parameters)
{
	using namespace RopeTestPrivate;

	// Corde couchée sur une pente de 20° : avec frottement elle tient presque, sans frottement elle glisse
	const FQuat Tilt(FVector::YAxisVector, FMath::DegreesToRadians(20.f));
	FRopeShape Slope;
	Slope.Type = ERopeShapeType::Box;
	Slope.Transform = FTransform(Tilt, FVector::ZeroVector);
	Slope.Extent = FVector(1000.f, 200.f, 10.f);

	const FVector Up = Tilt.GetAxisZ();
	const FVector Along = Tilt.GetAxisY();   // horizontal, perpendiculaire à la pente : la corde ne roule pas
	const FVector Start = Up * 12.f - Along * 50.f;
	const FVector End = Up * 12.f + Along * 50.f;

	auto Slide = [&](float Friction)
	{
		FRopeSolver Solver;
		Solver.Init(Start, End, 20, 100.f);
		FRopeSolverParams Params = DefaultParams();
		Params.Friction = Friction;
		const FRopeShape Shapes[] = { Slope };
		for (int32 Step = 0; Step < 240; ++Step)
		{
			Solver.Step(SubDt, Params, Shapes);
		}
		FVector Center = FVector::ZeroVector;
		for (const FVector& P : Solver.GetPositions())
		{
			Center += P / Solver.GetNumNodes();
		}
		return float(FVector::Distance(Center, (Start + End) * 0.5f));
	};

	const float WithFriction = Slide(0.8f);
	const float WithoutFriction = Slide(0.f);
	AddInfo(FString::Printf(TEXT("Glissement en 2 s : %.1f cm avec frottement 0,8 ; %.1f cm sans"), WithFriction, WithoutFriction));
	TestTrue(TEXT("Sans frottement, elle dévale"), WithoutFriction > 300.f);
	TestTrue(TEXT("Avec frottement, elle tient presque (< 10 cm)"), WithFriction < 10.f);
	return true;
}

#endif

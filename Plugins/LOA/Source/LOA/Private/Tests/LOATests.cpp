#include "GraphKitTypes.h"
#include "LOATypes.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// Lancer : Session Frontend > Automation > « LOA. », ou
//   UnrealEditor-Cmd.exe <projet> -ExecCmds="Automation RunTests LOA.; Quit" -nullrhi -unattended

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLOAPresenceTest, "LOA.Presence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLOAPresenceTest::RunTest(const FString& Parameters)
{
	// Visible jusqu'au niveau 1 : entier jusqu'à 1, à moitié à 1,5, disparu à 2
	FLOAElement Element;
	Element.VisibleUpTo = 1;
	TestEqual(TEXT("Niveau 0"), Element.Presence(0.f), 1.f);
	TestEqual(TEXT("Niveau 1"), Element.Presence(1.f), 1.f);
	TestEqual(TEXT("Niveau 1,5 : se replie"), Element.Presence(1.5f), 0.5f);
	TestEqual(TEXT("Niveau 2 : disparu"), Element.Presence(2.f), 0.f);
	TestEqual(TEXT("Niveau 5"), Element.Presence(5.f), 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLOACascadeTest, "LOA.Cascade",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLOACascadeTest::RunTest(const FString& Parameters)
{
	// Maison -> Grange (jusqu'au niveau 1) -> Lucarne (jusqu'au niveau 3) : la lucarne suit la grange
	FGraphKitGraph G;
	G.bDirected = true;
	FLOAElement Grange;
	Grange.VisibleUpTo = 1;
	FLOAElement Lucarne;
	Lucarne.VisibleUpTo = 3;
	G.AddNode(TEXT("Maison"));
	G.AddNode(TEXT("Grange"), FGameplayTagContainer(), FInstancedStruct::Make(Grange));
	G.AddNode(TEXT("Lucarne"), FGameplayTagContainer(), FInstancedStruct::Make(Lucarne));
	G.AddEdge(TEXT("Maison"), TEXT("Grange"));
	G.AddEdge(TEXT("Grange"), TEXT("Lucarne"));

	auto Getter = [](const FGraphKitNode& Node) { return Node.Payload.GetPtr<FLOAElement>(); };

	TMap<FName, float> Presence;
	LOA::ComputePresence(G, TEXT("Maison"), 1.5f, Getter, Presence);
	TestEqual(TEXT("Racine toujours là"), Presence.FindRef(TEXT("Maison")), 1.f);
	TestEqual(TEXT("Grange à moitié"), Presence.FindRef(TEXT("Grange")), 0.5f);
	TestEqual(TEXT("La lucarne ne peut pas être plus présente que la grange"), Presence.FindRef(TEXT("Lucarne")), 0.5f);

	LOA::ComputePresence(G, TEXT("Maison"), 0.f, Getter, Presence);
	TestEqual(TEXT("Niveau 0 : tout est là"), Presence.FindRef(TEXT("Lucarne")), 1.f);

	LOA::ComputePresence(G, TEXT("Château"), 0.f, Getter, Presence);
	TestEqual(TEXT("Racine inconnue : rien"), Presence.Num(), 0);
	return true;
}

#endif

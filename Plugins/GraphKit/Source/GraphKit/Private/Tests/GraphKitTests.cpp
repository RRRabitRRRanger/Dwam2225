#include "GraphKitTypes.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// Lancer : Session Frontend > Automation > « GraphKit », ou
//   UnrealEditor-Cmd.exe <projet> -ExecCmds="Automation RunTests GraphKit; Quit" -nullrhi -unattended

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGraphKitNodesTest, "GraphKit.Nodes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGraphKitNodesTest::RunTest(const FString& Parameters)
{
	FGraphKitGraph G;
	TestTrue(TEXT("Ajout"), G.AddNode(TEXT("Village")));
	TestFalse(TEXT("Nom déjà pris"), G.AddNode(TEXT("Village")));
	TestFalse(TEXT("Nom vide"), G.AddNode(NAME_None));
	TestTrue(TEXT("Ajout 2"), G.AddNode(TEXT("Prairie")));
	TestEqual(TEXT("Deux nœuds"), G.Nodes.Num(), 2);

	TestTrue(TEXT("Renommer"), G.RenameNode(TEXT("Prairie"), TEXT("Mine")));
	TestFalse(TEXT("Renommer vers un nom pris"), G.RenameNode(TEXT("Mine"), TEXT("Village")));
	TestTrue(TEXT("Le nouveau nom existe"), G.HasNode(TEXT("Mine")));
	TestFalse(TEXT("L'ancien n'existe plus"), G.HasNode(TEXT("Prairie")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGraphKitEdgesTest, "GraphKit.Edges",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGraphKitEdgesTest::RunTest(const FString& Parameters)
{
	// Non orienté : un triangle Village – Pont – Rive
	FGraphKitGraph G;
	G.AddNode(TEXT("Village"));
	G.AddNode(TEXT("Pont"));
	G.AddNode(TEXT("Rive"));
	TestEqual(TEXT("Arête 0"), G.AddEdge(TEXT("Village"), TEXT("Pont"), 3.f), 0);
	TestEqual(TEXT("Arête 1"), G.AddEdge(TEXT("Pont"), TEXT("Rive"), 1.f), 1);
	TestEqual(TEXT("Vers un nœud absent"), G.AddEdge(TEXT("Village"), TEXT("Louvre")), int32(INDEX_NONE));
	TestEqual(TEXT("Boucle refusée"), G.AddEdge(TEXT("Pont"), TEXT("Pont")), int32(INDEX_NONE));

	TArray<FGraphKitNeighbor> N;
	G.GetNeighbors(TEXT("Pont"), N);
	TestEqual(TEXT("Non orienté : le pont a deux voisins"), N.Num(), 2);

	// Renommer met à jour les arêtes
	G.RenameNode(TEXT("Pont"), TEXT("Gué"));
	G.GetNeighbors(TEXT("Gué"), N);
	TestEqual(TEXT("Le gué garde ses deux voisins"), N.Num(), 2);

	// Retirer un nœud retire ses arêtes
	G.RemoveNode(TEXT("Gué"));
	TestEqual(TEXT("Plus d'arête"), G.Edges.Num(), 0);

	TArray<FString> Problems;
	G.Validate(Problems);
	TestEqual(TEXT("Graphe valide"), Problems.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGraphKitDirectedTest, "GraphKit.Directed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGraphKitDirectedTest::RunTest(const FString& Parameters)
{
	// Orienté : Corps -> Entrée
	FGraphKitGraph G;
	G.bDirected = true;
	G.AddNode(TEXT("Corps"));
	G.AddNode(TEXT("Entrée"));
	G.AddEdge(TEXT("Corps"), TEXT("Entrée"), 2.f);

	TArray<FGraphKitNeighbor> N;
	G.GetNeighbors(TEXT("Corps"), N, EGraphKitDirection::Outgoing);
	TestEqual(TEXT("Corps : un enfant"), N.Num(), 1);
	TestTrue(TEXT("Sortante"), N.Num() == 1 && N[0].bOutgoing && N[0].Node == TEXT("Entrée") && N[0].Distance == 2.f);

	G.GetNeighbors(TEXT("Entrée"), N, EGraphKitDirection::Outgoing);
	TestEqual(TEXT("Entrée : pas d'enfant"), N.Num(), 0);
	G.GetNeighbors(TEXT("Entrée"), N, EGraphKitDirection::Incoming);
	TestEqual(TEXT("Entrée : un parent"), N.Num(), 1);

	TestEqual(TEXT("Retirer dans le mauvais sens : rien"), G.RemoveEdges(TEXT("Entrée"), TEXT("Corps")), 0);
	TestEqual(TEXT("Retirer dans le bon sens"), G.RemoveEdges(TEXT("Corps"), TEXT("Entrée")), 1);

	// Validation : une arête écrite à la main vers un nœud absent
	FGraphKitEdge& Bad = G.Edges.AddDefaulted_GetRef();
	Bad.From = TEXT("Corps");
	Bad.To = TEXT("Tour");
	TArray<FString> Problems;
	G.Validate(Problems);
	TestEqual(TEXT("Un problème signalé"), Problems.Num(), 1);
	return true;
}

#endif

#include "CharacterAssemblyComponent.h"
#include "CharacterKitLibrary.h"
#include "CharacterPreset.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "RenderingThread.h"
#include "TextureResource.h"

#if WITH_DEV_AUTOMATION_TESTS

// Lancer : Tools/RunTests.ps1 CharacterKit (éditeur avec fenêtre), ou Session Frontend > Automation > « CharacterKit ».
// Captures écrites dans Saved/Automation/CharacterKit/.

namespace CharacterKitTestPrivate
{
	const TCHAR* MannequinPath = TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple");
	const TCHAR* WalkPath = TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Walk/MF_Unarmed_Walk_Fwd.MF_Unarmed_Walk_Fwd");

	/** Un monde de jeu en mémoire, sans .umap, détruit à la fin du test. */
	struct FTestWorld
	{
		UWorld* World = nullptr;

		FTestWorld()
		{
			World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("CharacterKitTestWorld"));
			FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
			Context.SetCurrentWorld(World);
			World->InitializeActorsForPlay(FURL());
			World->BeginPlay();
		}

		~FTestWorld()
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
		}

		/** Avance le temps. Les mannequins sont posés explicitement (le tick d'un monde créé en mémoire ne fait pas jouer l'animation). */
		void Tick(int32 Frames, float Dt = 1.f / 30.f)
		{
			for (int32 i = 0; i < Frames; ++i)
			{
				Time += Dt;
				for (USkeletalMeshComponent* Skel : Skeletons)
				{
					Skel->SetPosition(FMath::Fmod(Time, FMath::Max(Skel->GetSingleNodeInstance() ? Skel->GetSingleNodeInstance()->GetLength() : 1.f, 0.01f)), false);
					Skel->TickAnimation(0.f, false);
					Skel->RefreshBoneTransforms();
				}
				World->Tick(LEVELTICK_All, Dt);
			}
		}

		float Time = 0.f;
		TArray<USkeletalMeshComponent*> Skeletons;
	};

	FCharacterPart MakePart(const TCHAR* Slot, const TCHAR* Bone, UStaticMesh* Mesh, const FVector& Loc, const FRotator& Rot, const FVector& Scale)
	{
		FCharacterPart Part;
		Part.Slot = Slot;
		Part.Bone = Bone;
		Part.Mesh = Mesh;
		Part.Offset = FTransform(Rot, Loc, Scale);
		return Part;
	}

	/** Les pièces du guide de test (Guide_CharacterKit) : formes du moteur sur la tête, le buste, un bras, une jambe. */
	FCharacterDescription MakeGuideDescription()
	{
		UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
		UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

		FCharacterDescription D;
		D.Parts.Add(MakePart(TEXT("Head"), TEXT("head"), Sphere, FVector(10, 0, 0), FRotator::ZeroRotator, FVector(0.22, 0.2, 0.25)));
		D.Parts.Add(MakePart(TEXT("Torso"), TEXT("spine_03"), Cube, FVector(10, 0, 0), FRotator::ZeroRotator, FVector(0.35, 0.25, 0.4)));
		D.Parts.Add(MakePart(TEXT("UpperArm_L"), TEXT("upperarm_l"), Cylinder, FVector(15, 0, 0), FRotator(90, 0, 0), FVector(0.1, 0.1, 0.3)));
		D.Parts.Add(MakePart(TEXT("LowerArm_L"), TEXT("lowerarm_l"), Cylinder, FVector(13, 0, 0), FRotator(90, 0, 0), FVector(0.08, 0.08, 0.26)));
		D.Parts.Add(MakePart(TEXT("Thigh_L"), TEXT("thigh_l"), Cylinder, FVector(-22, 0, 0), FRotator(90, 0, 0), FVector(0.14, 0.14, 0.42)));
		D.Parts.Add(MakePart(TEXT("Calf_L"), TEXT("calf_l"), Cylinder, FVector(-20, 0, 0), FRotator(90, 0, 0), FVector(0.11, 0.11, 0.4)));
		return D;
	}

	/** Un acteur mannequin qui marche, avec le component d'assemblage. */
	AActor* SpawnMannequin(FTestWorld& TW, USkeletalMesh* Mesh, UAnimSequence* Anim, UCharacterAssemblyComponent*& OutAssembly, USkeletalMeshComponent*& OutSkel)
	{
		UWorld* World = TW.World;
		AActor* Actor = World->SpawnActor<AActor>();
		OutSkel = NewObject<USkeletalMeshComponent>(Actor, TEXT("Body"));
		Actor->SetRootComponent(OutSkel);
		OutSkel->SetSkeletalMesh(Mesh);
		OutSkel->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
		OutSkel->RegisterComponent();
		OutSkel->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		OutSkel->PlayAnimation(Anim, true);
		OutSkel->bEnableUpdateRateOptimizations = false;
		TW.Skeletons.Add(OutSkel);

		OutAssembly = NewObject<UCharacterAssemblyComponent>(Actor, TEXT("Assembly"));
		OutAssembly->bUsePreset = false;
		OutAssembly->Description = MakeGuideDescription();
		OutAssembly->RegisterComponent();
		return Actor;
	}

	/** Rend la scène depuis un point de vue et enregistre un PNG. */
	bool Capture(UWorld* World, const FVector& Eye, const FVector& Target, const FString& FileName)
	{
		AActor* Rig = World->SpawnActor<AActor>();
		USceneCaptureComponent2D* Cap = NewObject<USceneCaptureComponent2D>(Rig);
		Rig->SetRootComponent(Cap);
		Cap->RegisterComponent();

		UTextureRenderTarget2D* RT = NewObject<UTextureRenderTarget2D>(Rig);
		RT->InitCustomFormat(640, 640, PF_B8G8R8A8, false);
		RT->UpdateResourceImmediate(true);

		Cap->TextureTarget = RT;
		Cap->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
		Cap->bCaptureEveryFrame = false;
		Cap->bCaptureOnMovement = false;
		Cap->FOVAngle = 40.f;
		Cap->PostProcessSettings.bOverride_AutoExposureMethod = true;
		Cap->PostProcessSettings.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
		Cap->PostProcessSettings.bOverride_AutoExposureBias = true;
		Cap->PostProcessSettings.AutoExposureBias = 10.f;
		Cap->SetWorldLocationAndRotation(Eye, (Target - Eye).Rotation());
		Cap->CaptureScene();
		FlushRenderingCommands();

		TArray<FColor> Pixels;
		FTextureRenderTargetResource* Res = RT->GameThread_GetRenderTargetResource();
		if (!Res || !Res->ReadPixels(Pixels))
		{
			Rig->Destroy();
			return false;
		}
		for (FColor& C : Pixels)
		{
			C.A = 255;
		}
		const FString Dir = FPaths::ProjectSavedDir() / TEXT("Automation/CharacterKit");
		IFileManager::Get().MakeDirectory(*Dir, true);
		const FImageView View(Pixels.GetData(), RT->SizeX, RT->SizeY);
		const bool bOk = FImageUtils::SaveImageByExtension(*(Dir / FileName), View);
		Rig->Destroy();
		return bOk;
	}

	void AddLights(UWorld* World)
	{
		AActor* Lights = World->SpawnActor<AActor>();
		UDirectionalLightComponent* Sun = NewObject<UDirectionalLightComponent>(Lights);
		Lights->SetRootComponent(Sun);
		Sun->SetIntensity(10.f);
		Sun->RegisterComponent();
		Sun->SetWorldRotation(FRotator(-40.f, -60.f, 0.f));

		// Sol gris, pour lire les pieds et les ombres
		UStaticMeshComponent* Ground = NewObject<UStaticMeshComponent>(Lights);
		Ground->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")));
		Ground->SetupAttachment(Sun);
		Ground->RegisterComponent();
		Ground->SetWorldTransform(FTransform(FRotator::ZeroRotator, FVector::ZeroVector, FVector(10.f)));
		USkyLightComponent* Sky = NewObject<USkyLightComponent>(Lights);
		Sky->SourceType = ESkyLightSourceType::SLS_SpecifiedCubemap;
		Sky->SetIntensity(1.f);
		Sky->SetupAttachment(Sun);
		Sky->RegisterComponent();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCharacterKitPartsFollowBonesTest, "CharacterKit.Assembly.PartsFollowBones",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCharacterKitPartsFollowBonesTest::RunTest(const FString& Parameters)
{
	using namespace CharacterKitTestPrivate;

	USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, MannequinPath);
	UAnimSequence* Walk = LoadObject<UAnimSequence>(nullptr, WalkPath);
	if (!TestNotNull(TEXT("Mannequin chargé"), Mesh) || !TestNotNull(TEXT("Marche chargée"), Walk))
	{
		return false;
	}

	FTestWorld TW;
	UCharacterAssemblyComponent* Assembly = nullptr;
	USkeletalMeshComponent* Skel = nullptr;
	SpawnMannequin(TW, Mesh, Walk, Assembly, Skel);
	TW.Tick(2);

	const FCharacterDescription Desc = Assembly->GetCurrentDescription();
	for (const FCharacterPart& Part : Desc.Parts)
	{
		TestNotNull(*FString::Printf(TEXT("Pièce '%s' créée"), *Part.Slot.ToString()), Assembly->GetPartComponent(Part.Slot));
	}

	// Pendant 3 s de marche : chaque pièce reste exactement à son offset dans le repère de son os, et les os bougent
	double MaxError = 0.0;
	double MaxBoneTravel = 0.0;
	TMap<FName, FVector> StartBone;
	for (int32 Frame = 0; Frame < 90; ++Frame)
	{
		TW.Tick(1);
		for (const FCharacterPart& Part : Desc.Parts)
		{
			const UStaticMeshComponent* Comp = Assembly->GetPartComponent(Part.Slot);
			if (!Comp)
			{
				continue;
			}
			const FTransform Bone = Skel->GetSocketTransform(Part.Bone);
			const FTransform Expected = Part.Offset * Bone;
			MaxError = FMath::Max(MaxError, FVector::Dist(Expected.GetLocation(), Comp->GetComponentLocation()));
			const FVector& Start = StartBone.FindOrAdd(Part.Bone, Bone.GetLocation());
			MaxBoneTravel = FMath::Max(MaxBoneTravel, FVector::Dist(Start, Bone.GetLocation()));
		}
	}
	AddInfo(FString::Printf(TEXT("Écart max pièce / os : %.4f cm ; déplacement max d'un os : %.1f cm"), MaxError, MaxBoneTravel));
	TestTrue(TEXT("Les pièces restent sur leurs os (< 0,1 mm)"), MaxError < 0.01);
	TestTrue(TEXT("L'animation fait bouger les os (> 5 cm)"), MaxBoneTravel > 5.0);

	// Repère des os utilisés : où est l'enfant, dans le repère du parent (pour régler les offsets du guide)
	for (const FCharacterPart& Part : Desc.Parts)
	{
		const int32 Index = Skel->GetBoneIndex(Part.Bone);
		const FTransform Bone = Skel->GetSocketTransform(Part.Bone);
		FString Children;
		for (int32 i = 0; i < Skel->GetNumBones(); ++i)
		{
			if (Skel->GetBoneName(i) != NAME_None && Skel->GetParentBone(Skel->GetBoneName(i)) == Part.Bone)
			{
				const FVector Local = Bone.InverseTransformPosition(Skel->GetSocketLocation(Skel->GetBoneName(i)));
				Children += FString::Printf(TEXT(" %s(%.1f, %.1f, %.1f)"), *Skel->GetBoneName(i).ToString(), Local.X, Local.Y, Local.Z);
			}
		}
		AddInfo(FString::Printf(TEXT("Os %s [%d] -> enfants :%s"), *Part.Bone.ToString(), Index, *Children));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCharacterKitSwapAndDescriptionTest, "CharacterKit.Assembly.SwapAndDescription",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCharacterKitSwapAndDescriptionTest::RunTest(const FString& Parameters)
{
	using namespace CharacterKitTestPrivate;

	USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, MannequinPath);
	UAnimSequence* Walk = LoadObject<UAnimSequence>(nullptr, WalkPath);
	UStaticMesh* Cone = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone"));
	if (!Mesh || !Walk || !Cone)
	{
		AddError(TEXT("Assets de test introuvables"));
		return false;
	}

	FTestWorld TW;
	UCharacterAssemblyComponent* Assembly = nullptr;
	USkeletalMeshComponent* Skel = nullptr;
	SpawnMannequin(TW, Mesh, Walk, Assembly, Skel);
	TW.Tick(1);

	// Swap : la pièce visible change, la description aussi, l'emplacement inconnu est refusé
	TestTrue(TEXT("Swap de la tête accepté"), Assembly->SetPartMesh(TEXT("Head"), Cone));
	TestEqual(TEXT("La pièce visible est le cône"), Assembly->GetPartComponent(TEXT("Head"))->GetStaticMesh().Get(), Cone);
	const FCharacterPart* Head = Assembly->GetCurrentDescription().Parts.FindByPredicate([](const FCharacterPart& P) { return P.Slot == TEXT("Head"); });
	TestTrue(TEXT("La description garde le swap"), Head && Head->Mesh == Cone);
	TestFalse(TEXT("Emplacement inconnu refusé"), Assembly->SetPartMesh(TEXT("Tail"), Cone));

	// Description copiée vers un autre personnage = même assemblage
	UCharacterAssemblyComponent* Other = nullptr;
	USkeletalMeshComponent* OtherSkel = nullptr;
	AActor* OtherActor = SpawnMannequin(TW, Mesh, Walk, Other, OtherSkel);
	OtherActor->SetActorLocation(FVector(0, 150, 0));
	Other->SetDescription(Assembly->GetCurrentDescription());
	TestEqual(TEXT("Même nombre de pièces"), Other->GetCurrentDescription().Parts.Num(), Assembly->GetCurrentDescription().Parts.Num());
	TestEqual(TEXT("La copie a la tête en cône"), Other->GetPartComponent(TEXT("Head"))->GetStaticMesh().Get(), Cone);

	// Os inconnu : la pièce est ignorée, pas de plantage
	FCharacterDescription Bad = Assembly->GetCurrentDescription();
	Bad.Parts.Add(MakePart(TEXT("Ghost"), TEXT("no_such_bone"), Cone, FVector::ZeroVector, FRotator::ZeroRotator, FVector::OneVector));
	AddExpectedMessage(TEXT("introuvable pour la pièce 'Ghost'"), EAutomationExpectedMessageFlags::Contains, 1);
	Other->SetDescription(Bad);
	TestNull(TEXT("Pièce sur os inconnu ignorée"), Other->GetPartComponent(TEXT("Ghost")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCharacterKitCaptureTest, "CharacterKit.Visual.WalkCaptures",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCharacterKitCaptureTest::RunTest(const FString& Parameters)
{
	using namespace CharacterKitTestPrivate;

	USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, MannequinPath);
	UAnimSequence* Walk = LoadObject<UAnimSequence>(nullptr, WalkPath);
	if (!Mesh || !Walk)
	{
		AddError(TEXT("Assets de test introuvables"));
		return false;
	}

	FTestWorld TW;
	AddLights(TW.World);
	UCharacterAssemblyComponent* Assembly = nullptr;
	USkeletalMeshComponent* Skel = nullptr;
	SpawnMannequin(TW, Mesh, Walk, Assembly, Skel);
	Assembly->SetDescription(UCharacterKitLibrary::MakeProxyDescription(Mesh));

	// Le mannequin regarde vers +Y (pieds à l'origine) : profil depuis +X (gauche du perso), face depuis +Y,
	// à plusieurs instants de la marche
	const FVector Target(0, 0, 95);
	const FVector Side(380, 40, 130);
	const FVector Front(60, 380, 130);
	for (int32 Shot = 0; Shot < 3; ++Shot)
	{
		TW.Tick(6);
		TestTrue(TEXT("Capture profil"), Capture(TW.World, Side, Target, FString::Printf(TEXT("Walk_%d_side.png"), Shot)));
	}
	TestTrue(TEXT("Capture face"), Capture(TW.World, Front, Target, TEXT("Walk_front.png")));
	Assembly->bHideSkeletonMesh = true;
	Assembly->Assemble();
	TW.Tick(1);
	TestTrue(TEXT("Capture pièces seules, profil"), Capture(TW.World, Side, Target, TEXT("PartsOnly_side.png")));
	TestTrue(TEXT("Capture pièces seules, face"), Capture(TW.World, Front, Target, TEXT("PartsOnly_front.png")));
	TestTrue(TEXT("Capture pièces seules, trois quarts"), Capture(TW.World, FVector(270, 270, 160), Target, TEXT("PartsOnly_34.png")));
	for (int32 Shot = 0; Shot < 2; ++Shot)
	{
		TW.Tick(8);
		TestTrue(TEXT("Capture marche, trois quarts"), Capture(TW.World, FVector(270, 270, 160), Target, FString::Printf(TEXT("PartsWalk_%d_34.png"), Shot)));
	}
	AddInfo(FString::Printf(TEXT("Captures : %s"), *(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()) / TEXT("Automation/CharacterKit"))));
	return true;
}

#endif

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCharacterKitProxySymmetryTest, "CharacterKit.Proxy.Symmetry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCharacterKitProxySymmetryTest::RunTest(const FString& Parameters)
{
	using namespace CharacterKitTestPrivate;

	USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, MannequinPath);
	if (!TestNotNull(TEXT("Mannequin chargé"), Mesh))
	{
		return false;
	}
	const FCharacterDescription Desc = UCharacterKitLibrary::MakeProxyDescription(Mesh);
	TestTrue(TEXT("23 pièces : 9 au centre (bassin, 5 vertèbres, 2 cou, tête) + 7 paires (clavicule, bras, avant-bras, main, cuisse, mollet, pied)"), Desc.Parts.Num() == 23);

	// En pose de référence, chaque pièce gauche a sa jumelle droite au miroir du plan X = 0
	FTestWorld TW;
	UCharacterAssemblyComponent* Assembly = nullptr;
	USkeletalMeshComponent* Skel = nullptr;
	SpawnMannequin(TW, Mesh, nullptr, Assembly, Skel);
	Assembly->SetDescription(Desc);
	double MaxMirrorError = 0.0;
	int32 Pairs = 0;
	for (const FCharacterPart& Part : Desc.Parts)
	{
		FString Name = Part.Slot.ToString();
		if (!Name.EndsWith(TEXT("_l")))
		{
			continue;
		}
		const FName Twin(Name.LeftChop(2) + TEXT("_r"));
		const UStaticMeshComponent* L = Assembly->GetPartComponent(Part.Slot);
		const UStaticMeshComponent* R = Assembly->GetPartComponent(Twin);
		if (!TestTrue(*FString::Printf(TEXT("Jumelle de %s"), *Name), L && R))
		{
			continue;
		}
		FVector PL = L->GetComponentLocation();
		const FVector PR = R->GetComponentLocation();
		PL.X = -PL.X;
		MaxMirrorError = FMath::Max(MaxMirrorError, FVector::Dist(PL, PR));
		++Pairs;
	}
	AddInfo(FString::Printf(TEXT("%d pièces, %d paires gauche / droite, écart au miroir max : %.2f cm"), Desc.Parts.Num(), Pairs, MaxMirrorError));
	TestTrue(TEXT("Symétrie gauche / droite (< 0,5 cm)"), MaxMirrorError < 0.5);
	return true;
}

#endif

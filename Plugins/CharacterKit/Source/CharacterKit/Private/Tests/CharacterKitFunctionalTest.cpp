#include "CharacterAssemblyComponent.h"
#include "CharacterKitLibrary.h"
#include "CharacterKitAnimInstance.h"
#include "CharacterBodyProfile.h"
#include "Materials/MaterialInstanceDynamic.h"
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
#include "ShaderCompiler.h"
#include "DistanceFieldAtlas.h"

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
					if (UCharacterKitAnimInstance* Anim = Cast<UCharacterKitAnimInstance>(Skel->GetAnimInstance()))
					{
						Anim->Time = Time;
					}
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
		OutSkel->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		OutSkel->SetAnimInstanceClass(UCharacterKitAnimInstance::StaticClass());
		OutSkel->bEnableUpdateRateOptimizations = false;
		OutSkel->RegisterComponent();
		if (UCharacterKitAnimInstance* AnimInstance = Cast<UCharacterKitAnimInstance>(OutSkel->GetAnimInstance()))
		{
			AnimInstance->Sequence = Anim;
		}
		TW.Skeletons.Add(OutSkel);

		OutAssembly = NewObject<UCharacterAssemblyComponent>(Actor, TEXT("Assembly"));
		OutAssembly->bUsePreset = false;
		OutAssembly->Description = MakeGuideDescription();
		OutAssembly->RegisterComponent();
		return Actor;
	}

	/** Rend la scène depuis un point de vue et enregistre un PNG. */
	bool Capture(UWorld* World, const FVector& Eye, const FVector& Target, const FString& FileName, int32 Width = 640, int32 Height = 640, float Fov = 40.f, float ExposureBias = 4.5f)
	{
		AActor* Rig = World->SpawnActor<AActor>();
		USceneCaptureComponent2D* Cap = NewObject<USceneCaptureComponent2D>(Rig);
		Rig->SetRootComponent(Cap);
		Cap->RegisterComponent();

		// Image brute : couleur de la scène AVANT le post-process d'Unreal (ni exposition ni courbe de tons),
		// encodée en sRGB ici. C'est ce que donne un matériau unlit qui calcule sa propre lumière.
		const bool bRaw = ExposureBias < 0.f;
		UTextureRenderTarget2D* RT = NewObject<UTextureRenderTarget2D>(Rig);
		RT->InitCustomFormat(Width, Height, bRaw ? PF_FloatRGBA : PF_B8G8R8A8, !bRaw);
		RT->UpdateResourceImmediate(true);

		Cap->TextureTarget = RT;
		Cap->CaptureSource = bRaw ? ESceneCaptureSource::SCS_SceneColorHDR : ESceneCaptureSource::SCS_FinalColorLDR;
		Cap->bCaptureEveryFrame = false;
		Cap->bCaptureOnMovement = false;
		Cap->bAlwaysPersistRenderingState = true; // le GDF est mis en cache dans l'état de vue : sans lui, il reste vide
		Cap->FOVAngle = Fov;
		Cap->PostProcessSettings.bOverride_AutoExposureMethod = true;
		Cap->PostProcessSettings.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
		Cap->PostProcessSettings.bOverride_AutoExposureBias = true;
		Cap->PostProcessSettings.AutoExposureBias = ExposureBias;
		Cap->SetWorldLocationAndRotation(Eye, (Target - Eye).Rotation());
		Cap->CaptureScene();
		FlushRenderingCommands();

		TArray<FColor> Pixels;
		FTextureRenderTargetResource* Res = RT->GameThread_GetRenderTargetResource();
		if (bRaw)
		{
			TArray<FLinearColor> Linear;
			if (!Res || !Res->ReadLinearColorPixels(Linear, FReadSurfaceDataFlags(RCM_MinMax)))
			{
				Rig->Destroy();
				return false;
			}
			Pixels.Reserve(Linear.Num());
			for (const FLinearColor& L : Linear)
			{
				Pixels.Add(FLinearColor(L.R, L.G, L.B, 1.f).ToFColorSRGB());
			}
		}
		else if (!Res || !Res->ReadPixels(Pixels))
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

	UStaticMeshComponent* AddLights(UWorld* World)
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
		return Ground;
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
	TestTrue(TEXT("35 pièces : 9 au centre (bassin, 5 vertèbres, 2 cou, tête) + 7 paires (clavicule, bras, avant-bras, main, cuisse, mollet, pied) + 6 paires de rotules"), Desc.Parts.Num() == 35);

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

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCharacterKitBodyRatiosTest, "CharacterKit.Visual.BodyRatios",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCharacterKitBodyRatiosTest::RunTest(const FString& Parameters)
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
	const TArray<TPair<FString, FCharacterBodyProfile>> Presets = CharacterBodyProfile::Presets();
	TArray<USkeletalMeshComponent*> Skels;
	FString Names;
	for (int32 i = 0; i < Presets.Num(); ++i)
	{
		UCharacterAssemblyComponent* Assembly = nullptr;
		USkeletalMeshComponent* Skel = nullptr;
		AActor* Actor = SpawnMannequin(TW, Mesh, Walk, Assembly, Skel);
		// Rangés de gauche à droite face à la caméra (le mannequin regarde vers +Y)
		Actor->SetActorLocation(FVector((i - (Presets.Num() - 1) * 0.5) * -95.0, 0, 0));
		FCharacterDescription D = UCharacterKitLibrary::MakeProxyDescription(Mesh);
		D.Profile = Presets[i].Value;
		Assembly->bHideSkeletonMesh = true;
		Assembly->SetDescription(D);
		Skels.Add(Skel);
		Names += Presets[i].Key + TEXT(" ");
	}
	AddInfo(FString::Printf(TEXT("De gauche à droite : %s"), *Names));

	// Pose de référence (temps 0) : les pieds doivent toucher le sol quel que soit le profil
	TW.Tick(1, 0.f);
	const float RefFoot = Skels[0]->GetSocketLocation(TEXT("foot_l")).Z;
	for (int32 i = 0; i < Skels.Num(); ++i)
	{
		const float Expected = RefFoot * Presets[i].Value.Height;
		const float Foot = Skels[i]->GetSocketLocation(TEXT("foot_l")).Z;
		AddInfo(FString::Printf(TEXT("%s : cheville à %.1f cm (attendu %.1f), tête à %.1f cm"), *Presets[i].Key, Foot, Expected,
			Skels[i]->GetSocketLocation(TEXT("head")).Z));
		TestTrue(*FString::Printf(TEXT("%s : pieds au sol (< 3 cm)"), *Presets[i].Key), FMath::Abs(Foot - Expected) < 3.f);
	}

	const FVector Target(0, 0, 90);
	TestTrue(TEXT("Capture ratios, face"), Capture(TW.World, FVector(0, 720, 120), Target, TEXT("Ratios_front.png"), 1280, 560, 50.f));
	TW.Tick(8);
	TestTrue(TEXT("Capture ratios, marche"), Capture(TW.World, FVector(-420, 620, 160), Target, TEXT("Ratios_walk_34.png"), 1280, 560, 50.f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCharacterKitJointBlendTest, "CharacterKit.Visual.JointBlend",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCharacterKitJointBlendTest::RunTest(const FString& Parameters)
{
	using namespace CharacterKitTestPrivate;

	USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, MannequinPath);
	UAnimSequence* Walk = LoadObject<UAnimSequence>(nullptr, WalkPath);
	UMaterialInterface* Debug = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/_Claude/M_Claude_JointBlendDebug.M_Claude_JointBlendDebug"));
	if (!Mesh || !Walk)
	{
		AddError(TEXT("Assets de test introuvables"));
		return false;
	}
	if (!Debug)
	{
		AddWarning(TEXT("M_Claude_JointBlendDebug absent : lancer Tools/RunPython.ps1 Tools/Claude/create_joint_blend_material.py"));
		return true;
	}

	FTestWorld TW;
	UStaticMeshComponent* Ground = AddLights(TW.World);
	UCharacterAssemblyComponent* Assembly = nullptr;
	USkeletalMeshComponent* Skel = nullptr;
	SpawnMannequin(TW, Mesh, Walk, Assembly, Skel);

	// Un matériau dynamique partagé par toutes les pièces : on change de mode entre les captures
	UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Debug, Assembly);
	FCharacterDescription D = UCharacterKitLibrary::MakeProxyDescription(Mesh);
	for (FCharacterPart& Part : D.Parts)
	{
		Part.Material = MID;
	}
	Assembly->bHideSkeletonMesh = true;
	Assembly->SetDescription(D);
	// Le shader ne compile qu'au premier rendu (en attendant, Unreal affiche son matériau par défaut, le damier) :
	// une capture de chauffe, puis on attend la fin de la compilation
	TW.Tick(12);
	// Les champs de distance des meshes (MDF) se construisent en arrière-plan : les attendre, sinon le GDF est vide
	if (GDistanceFieldAsyncQueue)
	{
		GDistanceFieldAsyncQueue->BlockUntilAllBuildsComplete();
	}
	for (int32 Warm = 0; Warm < 6; ++Warm)
	{
		TW.Tick(1, 0.f);
		Capture(TW.World, FVector(260, 260, 150), FVector(0, 0, 95), TEXT("_warmup.png"));
		if (GShaderCompilingManager)
		{
			GShaderCompilingManager->FinishAllCompilation();
		}
	}
	IFileManager::Get().Delete(*(FPaths::ProjectSavedDir() / TEXT("Automation/CharacterKit/_warmup.png")));

	// Image brute (sans post-process d'Unreal), éclairage calculé dans le matériau. Exposition < 0 = capture brute.
	constexpr float Raw = -1.f;
	static const TCHAR* ModeNames[] = { TEXT("hybride"), TEXT("origine"), TEXT("masque"), TEXT("surface"), TEXT("jonctions") };

	// Le sol prend le même matériau, sans mélange (mode « origine ») : XOR + soleil maison
	UMaterialInstanceDynamic* GroundMID = UMaterialInstanceDynamic::Create(Debug, Assembly);
	GroundMID->SetScalarParameterValue(TEXT("Mode"), 1.f);
	// Le XOR de Quilez pèse surtout sur sa plus grande octave (128 cases) : 1,2 cm donne un motif d'environ 1,5 m
	GroundMID->SetScalarParameterValue(TEXT("Cell"), 1.2f);
	Ground->SetMaterial(0, GroundMID);

	// Bonhomme × 2,5 (idée de l'auteur) : le GDF a la même finesse en cm, donc relativement 2,5 fois plus fine.
	// Sondes à l'échelle (5 et 12 cm × 2,5).
	Skel->GetOwner()->SetActorScale3D(FVector(2.5f));
	MID->SetScalarParameterValue(TEXT("R1"), 12.5f);
	MID->SetScalarParameterValue(TEXT("R2"), 30.f);
	MID->SetScalarParameterValue(TEXT("Cell"), 0.35f);
	for (int32 Warm = 0; Warm < 4; ++Warm)
	{
		TW.Tick(1, 0.f);
		Capture(TW.World, FVector(650, 650, 375), FVector(0, 0, 240), TEXT("_warmup.png"), 640, 640, 40.f, Raw);
	}
	IFileManager::Get().Delete(*(FPaths::ProjectSavedDir() / TEXT("Automation/CharacterKit/_warmup.png")));

	// Haut du corps (buste, bras) et bas du corps (bassin, jambes), en trois quarts face, pour chaque mode
	const FVector Upper(0, 0, 330), Lower(0, 0, 120);
	for (int32 Mode : { 1, 0, 3, 2, 4 })
	{
		MID->SetScalarParameterValue(TEXT("Mode"), Mode);
		const FString M = FString::Printf(TEXT("Mode%d_%s"), Mode, ModeNames[Mode]);
		TestTrue(TEXT("Capture haut"), Capture(TW.World, Upper + FVector(260, 260, 40), Upper, M + TEXT("_haut.png"), 640, 640, 40.f, Raw));
		TestTrue(TEXT("Capture bas"), Capture(TW.World, Lower + FVector(280, 280, 30), Lower, M + TEXT("_bas.png"), 640, 640, 40.f, Raw));
	}
	return true;
}

#endif

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCharacterKitTwoSpheresTest, "CharacterKit.Visual.TwoSpheres",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCharacterKitTwoSpheresTest::RunTest(const FString& Parameters)
{
	using namespace CharacterKitTestPrivate;

	UMaterialInterface* Debug = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/_Claude/M_Claude_JointBlendDebug.M_Claude_JointBlendDebug"));
	UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (!Debug || !Sphere)
	{
		AddError(TEXT("Assets de test introuvables"));
		return false;
	}

	// Demande de l'auteur : deux sphères d'échelle 2 et 3 (rayons 100 et 150 cm) qui se coupent, avec et sans le shader
	FTestWorld TW;
	UStaticMeshComponent* Ground = AddLights(TW.World);
	AActor* Holder = TW.World->SpawnActor<AActor>();
	UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Debug, Holder);
	MID->SetScalarParameterValue(TEXT("Cell"), 0.6f);
	auto AddSphere = [&](const FVector& Center, float Scale)
	{
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(Holder);
		C->SetStaticMesh(Sphere);
		C->SetMaterial(0, MID);
		C->SetMobility(EComponentMobility::Movable);
		if (!Holder->GetRootComponent())
		{
			Holder->SetRootComponent(C);
		}
		C->RegisterComponent();
		C->SetWorldTransform(FTransform(FQuat::Identity, Center, FVector(Scale)));
	};
	AddSphere(FVector(-110, 0, 150), 2.f);
	AddSphere(FVector(110, 0, 150), 3.f);
	UMaterialInstanceDynamic* GroundMID = UMaterialInstanceDynamic::Create(Debug, Holder);
	GroundMID->SetScalarParameterValue(TEXT("Mode"), 1.f);
	GroundMID->SetScalarParameterValue(TEXT("Cell"), 1.2f);
	Ground->SetMaterial(0, GroundMID);

	constexpr float Raw = -1.f;
	const FVector Eye(0, 650, 260), Target(0, 0, 150);
	if (GDistanceFieldAsyncQueue)
	{
		GDistanceFieldAsyncQueue->BlockUntilAllBuildsComplete();
	}
	for (int32 Warm = 0; Warm < 6; ++Warm)
	{
		TW.Tick(1, 0.f);
		Capture(TW.World, Eye, Target, TEXT("_warmup.png"), 640, 480, 50.f, Raw);
		if (GShaderCompilingManager)
		{
			GShaderCompilingManager->FinishAllCompilation();
		}
	}
	IFileManager::Get().Delete(*(FPaths::ProjectSavedDir() / TEXT("Automation/CharacterKit/_warmup.png")));

	static const TCHAR* ModeNames[] = { TEXT("hybride"), TEXT("origine"), TEXT("masque"), TEXT("surface"), TEXT("jonctions") };
	// Trois réglages : actuel (sondes 5 / 40, base 0,3), sans base, puis sondes plus grandes qu'une case du GDF (25 / 60)
	struct FSetting { const TCHAR* Tag; float R1, R2, Base; };
	for (const FSetting& S : { FSetting{ TEXT("actuel"), 5.f, 40.f, 0.3f }, FSetting{ TEXT("base0"), 5.f, 40.f, 0.f }, FSetting{ TEXT("sondes25"), 25.f, 60.f, 0.f } })
	{
		MID->SetScalarParameterValue(TEXT("R1"), S.R1);
		MID->SetScalarParameterValue(TEXT("R2"), S.R2);
		MID->SetScalarParameterValue(TEXT("BaseBlend"), S.Base);
		for (int32 Mode : { 1, 0, 3, 2, 4 })
		{
			MID->SetScalarParameterValue(TEXT("Mode"), Mode);
			TestTrue(TEXT("Capture sphères"), Capture(TW.World, Eye, Target,
				FString::Printf(TEXT("Spheres_%s_%d_%s.png"), S.Tag, Mode, ModeNames[Mode]), 640, 480, 50.f, Raw));
		}
	}
	// Mesure de la distance lue par la sonde proche (mode 5), pour plusieurs hauteurs de sonde
	MID->SetScalarParameterValue(TEXT("Mode"), 5.f);
	for (float R : { 5.f, 25.f, 60.f })
	{
		MID->SetScalarParameterValue(TEXT("R1"), R);
		Capture(TW.World, Eye, Target, FString::Printf(TEXT("Spheres_mesure_R%d.png"), FMath::RoundToInt(R)), 640, 480, 50.f, Raw);
	}
	MID->SetScalarParameterValue(TEXT("R1"), 25.f);

	// Gros plan sur l'intersection, sondes 25 / 60
	for (int32 Mode : { 1, 0, 2 })
	{
		MID->SetScalarParameterValue(TEXT("Mode"), Mode);
		Capture(TW.World, FVector(-40, 330, 230), FVector(-20, 0, 150), FString::Printf(TEXT("Spheres_zoom_%d_%s.png"), Mode, ModeNames[Mode]), 640, 480, 40.f, Raw);
	}
	return true;
}

#endif

#include "SDFEyeComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Modules/ModuleManager.h"

IMPLEMENT_MODULE(FDefaultModuleImpl, SDFEyesRuntime)

namespace SDFEyePrivate
{
	// La sphère du moteur fait 50 de rayon ; le globe en occupe 1 / 1,2 (le reste : paupières, cils)
	constexpr float SphereRadius = 50.f;
	constexpr float SupportRatio = 1.2f;

	float Hash(int32 N)
	{
		return FMath::Frac(FMath::Sin(N * 127.1f) * 43758.5453f);
	}
}

USDFEyeComponent::USDFEyeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bTickInEditor = true;
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CastShadow = false;
	bAffectDistanceFieldLighting = false;
	SetGenerateOverlapEvents(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereFinder.Succeeded())
	{
		SetStaticMesh(SphereFinder.Object);
	}
}

void USDFEyeComponent::OnRegister()
{
	Super::OnRegister();
	Seed = GetTypeHash(GetFName()) & 0xffff;
	UpdateEye();
}

#if WITH_EDITOR
void USDFEyeComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	UpdateEye();
}
#endif

void USDFEyeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	MicroTime += DeltaTime;
	UpdateEye();
}

void USDFEyeComponent::EvaluateMicroLife(float& OutBlink, FVector2D& OutGaze) const
{
	using namespace SDFEyePrivate;

	// Clignements : un par intervalle de 2 à 6 s (tiré par intervalle), fermeture rapide, réouverture plus lente
	// avec un léger rebond. Les deux yeux d'un visage partagent le même Seed s'ils doivent cligner ensemble.
	OutBlink = 0.f;
	double Start = 0.0;
	for (int32 i = 0; i < 64; ++i)
	{
		const double Next = Start + 2.0 + 4.0 * Hash(Seed + i);
		if (MicroTime < Next)
		{
			const double T = MicroTime - (Next - 0.32);
			if (T > 0.0)
			{
				const float Close = FMath::Clamp(float(T / 0.07), 0.f, 1.f);
				const float Open = FMath::Clamp(float((T - 0.09) / 0.23), 0.f, 1.f);
				const float Bounce = 0.08f * FMath::Sin(Open * PI) * (1.f - Open);
				OutBlink = FMath::Clamp(Close * (1.f - Open) + Bounce, 0.f, 1.f);
			}
			break;
		}
		Start = Next;
	}

	// Micro-saccades : petits sauts de 0,5 à 1° toutes les 0,3 à 1 s, maintenus entre deux sauts
	const int32 Step = FMath::FloorToInt32(MicroTime / 0.6);
	const float A = FMath::DegreesToRadians(0.5f + 0.5f * Hash(Seed * 7 + Step));
	const float Dir = 2.f * PI * Hash(Seed * 13 + Step);
	OutGaze = FVector2D(FMath::Cos(Dir), FMath::Sin(Dir)) * A;
}

void USDFEyeComponent::UpdateEye()
{
	using namespace SDFEyePrivate;

	if (!EyeMaterial)
	{
		return;
	}
	if (!MID || MID->Parent != EyeMaterial)
	{
		MID = UMaterialInstanceDynamic::Create(EyeMaterial, this);
		SetMaterial(0, MID);
	}

	// La sphère support : rayon du globe × 1,2, échelle propre (indépendante du parent)
	const float SupportScale = GlobeRadius * SupportRatio / SphereRadius;
	SetRelativeScale3D_Direct(FVector(SupportScale));
	UpdateComponentToWorld();

	const FSDFEyeShapeSettings& S = Shape ? Shape->Settings : ShapeOverride;
	float B = Blink;
	FVector2D G = Gaze;
	if (bMicroLife)
	{
		float LifeBlink;
		FVector2D LifeGaze;
		EvaluateMicroLife(LifeBlink, LifeGaze);
		B = FMath::Max(B, LifeBlink);
		G += LifeGaze;
	}

	auto Set = [this](const TCHAR* Name, float Value) { MID->SetScalarParameterValue(Name, Value); };
	Set(TEXT("EyeRadius"), SphereRadius / SupportRatio);
	Set(TEXT("Mirror"), bMirror ? 1.f : 0.f);
	Set(TEXT("Blink"), B);
	Set(TEXT("GazeX"), G.X);
	Set(TEXT("GazeY"), G.Y);
	Set(TEXT("Follow"), Follow);
	Set(TEXT("Pupil"), Pupil);
	Set(TEXT("EU"), S.EU); Set(TEXT("TU"), S.TU); Set(TEXT("PU"), S.PU);
	Set(TEXT("EL"), S.EL); Set(TEXT("TL"), S.TL); Set(TEXT("PL"), S.PL);
	Set(TEXT("Width"), S.Width); Set(TEXT("Tilt"), S.Tilt); Set(TEXT("Close"), S.Close);
	Set(TEXT("FoldGap"), S.FoldGap); Set(TEXT("Fold"), S.Fold); Set(TEXT("Hood"), S.Hood);
	Set(TEXT("Thick"), S.Thick); Set(TEXT("Lash"), S.Lash); Set(TEXT("Curl"), S.Curl);
	Set(TEXT("Car"), S.Car); Set(TEXT("Veins"), S.Veins);
}

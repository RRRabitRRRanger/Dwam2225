#include "DwamLightTypes.h"
#include "Math/Float16.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// Lancer : Session Frontend > Automation > « DwamLighting », ou
//   UnrealEditor-Cmd.exe <projet> -ExecCmds="Automation RunTests DwamLighting; Quit" -nullrhi -unattended

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDwamLightPackingColorTest, "DwamLighting.Packing.Color",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDwamLightPackingColorTest::RunTest(const FString& Parameters)
{
	using namespace DwamLightPacking;

	TestEqual(TEXT("Le noir reste noir"), PackColorRGB9E5(FVector3f::ZeroVector), 0u);

	const FVector3f Colors[] = {
		FVector3f(1.f, 1.f, 1.f), FVector3f(1.f, 0.5f, 0.25f), FVector3f(0.02f, 0.8f, 0.3f),
		FVector3f(8.f, 2.f, 0.5f), FVector3f(0.001f, 0.002f, 0.003f), FVector3f(1000.f, 10.f, 500.f),
		FVector3f(0.999f, 0.999f, 0.999f), FVector3f(2.f, 2.f, 2.f)
	};
	for (const FVector3f& Color : Colors)
	{
		const FVector3f Back = UnpackColorRGB9E5(PackColorRGB9E5(Color));
		// 9 bits de mantisse : l'erreur de chaque canal est au plus un demi-pas du plus grand canal
		const float Tolerance = Color.GetMax() / 256.f;
		TestTrue(FString::Printf(TEXT("Couleur %s -> %s"), *Color.ToString(), *Back.ToString()),
			Color.Equals(Back, Tolerance));
	}

	// Les valeurs négatives et hors plage ne débordent pas
	const FVector3f Clamped = UnpackColorRGB9E5(PackColorRGB9E5(FVector3f(-1.f, 1e9f, 0.f)));
	TestTrue(TEXT("Clamp"), Clamped.X == 0.f && Clamped.Y > 60000.f && Clamped.Z == 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDwamLightPackingOrientationTest, "DwamLighting.Packing.Orientation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDwamLightPackingOrientationTest::RunTest(const FString& Parameters)
{
	using namespace DwamLightPacking;

	FRandomStream Random(2225);
	float WorstAxisDegrees = 0.f;
	float WorstQuatDegrees = 0.f;

	TArray<FVector3f> Axes = {
		FVector3f(1, 0, 0), FVector3f(-1, 0, 0), FVector3f(0, 1, 0), FVector3f(0, -1, 0), FVector3f(0, 0, 1), FVector3f(0, 0, -1)
	};
	for (int32 Index = 0; Index < 2000; ++Index)
	{
		Axes.Add(FVector3f(Random.GetUnitVector()));
	}
	for (const FVector3f& Axis : Axes)
	{
		const FVector3f Back = UnpackAxisOct16(PackAxisOct16(Axis));
		// atan2(|A × B|, A · B) : précis pour les petits angles, là où acos(A · B) plafonne vers 0.03 degré en float
		WorstAxisDegrees = FMath::Max(WorstAxisDegrees, FMath::RadiansToDegrees(FMath::Atan2((Axis ^ Back).Size(), Axis | Back)));
	}
	TestTrue(FString::Printf(TEXT("Axe octaédrique : pire écart %.4f degrés"), WorstAxisDegrees), WorstAxisDegrees < 0.01f);

	TArray<FQuat4f> Rotations = {
		FQuat4f::Identity, FQuat4f(1, 0, 0, 0), FQuat4f(0, 1, 0, 0), FQuat4f(0, 0, 1, 0), FQuat4f(0, 0, 0, -1),
		FQuat4f(0.5f, 0.5f, 0.5f, 0.5f), FQuat4f(-0.5f, 0.5f, -0.5f, 0.5f)
	};
	for (int32 Index = 0; Index < 2000; ++Index)
	{
		Rotations.Add(FQuat4f(FQuat(Random.GetUnitVector(), Random.FRandRange(0.f, UE_TWO_PI))));
	}
	for (const FQuat4f& Rotation : Rotations)
	{
		const FQuat4f Back = UnpackQuatSmallest3(PackQuatSmallest3(Rotation));
		WorstQuatDegrees = FMath::Max(WorstQuatDegrees, FMath::RadiansToDegrees(Rotation.AngularDistance(Back)));
	}
	TestTrue(FString::Printf(TEXT("Quaternion : pire écart %.4f degrés"), WorstQuatDegrees), WorstQuatDegrees < 0.3f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDwamLightPackingLayoutTest, "DwamLighting.Packing.Layout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDwamLightPackingLayoutTest::RunTest(const FString& Parameters)
{
	using namespace DwamLightPacking;

	TestEqual(TEXT("256 px = 127 lumières"), CapacityForWidth(256), 127);
	TestEqual(TEXT("2 px = aucune lumière"), CapacityForWidth(2), 0);

	auto AsFloat = [](uint32 Bits) { float Value; FMemory::Memcpy(&Value, &Bits, sizeof(Value)); return Value; };

	uint32 Header[8];
	PackHeader(42, Header);
	TestEqual(TEXT("Nombre de lumières"), AsFloat(Header[0]), 42.f);
	TestEqual(TEXT("Version"), AsFloat(Header[1]), (float)FormatVersion);
	TestEqual(TEXT("Nombre magique"), AsFloat(Header[2]), Magic);
	TestEqual(TEXT("Motif de test"), Header[4], TestPattern[0]);
	TestEqual(TEXT("Motif de test (fin)"), Header[7], TestPattern[3]);

	// Loin de l'origine : la position relative garde sa précision
	FDwamLightData Light;
	Light.Position = FVector(10000000.25, -20000000.5, 300.125);
	Light.Range = 750.f;
	Light.Type = EDwamLightType::Spot;
	Light.Rotation = FQuat4f(FQuat(FRotator(-30.0, 45.0, 0.0)));
	Light.CosOuter = 0.5f;
	Light.CosInner = 0.9f;
	Light.Layers = 0xA5;
	Light.bCastShadow = true;
	Light.ShadowSoftness = 1.f;
	Light.FalloffExponent = 3.5f;

	const FVector PreViewTranslation(-10000000.0, 20000000.0, 0.0);
	uint32 Words[8];
	PackLight(Light, PreViewTranslation, Words);

	TestEqual(TEXT("X relatif"), AsFloat(Words[0]), 0.25f);
	TestEqual(TEXT("Y relatif"), AsFloat(Words[1]), -0.5f);
	TestEqual(TEXT("Z relatif"), AsFloat(Words[2]), 300.125f);
	TestEqual(TEXT("Portée"), AsFloat(Words[3]), 750.f);

	const uint32 Flags = Words[7];
	TestEqual(TEXT("Type"), Flags & 3u, (uint32)EDwamLightType::Spot);
	TestEqual(TEXT("Ombre"), (Flags >> 2) & 1u, 1u);
	TestEqual(TEXT("Layers"), (Flags >> 3) & 255u, 0xA5u);
	TestEqual(TEXT("Douceur d'ombre"), (Flags >> 11) & 255u, 255u);
	TestEqual(TEXT("Exposant du falloff"), UnpackFalloffExponent(Flags), 3.5f);
	TestEqual(TEXT("Bits réservés à zéro"), Flags >> 27, 0u);

	TestEqual(TEXT("Cône extérieur"), (float)(Words[6] & 0xFFFFu) / 65535.f, 0.5f, 1e-4f);
	TestEqual(TEXT("Cône intérieur"), (float)(Words[6] >> 16) / 65535.f, 0.9f, 1e-4f);
	TestTrue(TEXT("Axe du spot"), UnpackAxisOct16(Words[5]).Equals(Light.Rotation.GetAxisX(), 1e-3f));

	// Rect : demi-tailles en demi-flottants
	Light.Type = EDwamLightType::Rect;
	Light.HalfSize = FVector2f(62.5f, 200.f);
	PackLight(Light, PreViewTranslation, Words);
	FFloat16 HalfWidth, HalfHeight;
	HalfWidth.Encoded = (uint16)(Words[6] & 0xFFFFu);
	HalfHeight.Encoded = (uint16)(Words[6] >> 16);
	TestEqual(TEXT("Demi-largeur"), HalfWidth.GetFloat(), 62.5f);
	TestEqual(TEXT("Demi-hauteur"), HalfHeight.GetFloat(), 200.f);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

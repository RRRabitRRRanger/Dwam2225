#pragma once

#include "CoreMinimal.h"
#include "DwamLightTypes.generated.h"

DWAMLIGHTINGRUNTIME_API DECLARE_LOG_CATEGORY_EXTERN(LogDwamLighting, Log, All);

/** Forme d'une lumière maison. La directionnelle (le soleil) reste à part. */
UENUM(BlueprintType)
enum class EDwamLightType : uint8
{
	Point,
	Spot,
	Rect
};

/** Layers d'éclairage : une lumière n'éclaire que les matériaux qui partagent au moins un layer avec elle. */
UENUM(meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EDwamLightLayer : uint8
{
	None = 0 UMETA(Hidden),
	Layer0 = 1 << 0,
	Layer1 = 1 << 1,
	Layer2 = 1 << 2,
	Layer3 = 1 << 3,
	Layer4 = 1 << 4,
	Layer5 = 1 << 5,
	Layer6 = 1 << 6,
	Layer7 = 1 << 7
};
ENUM_CLASS_FLAGS(EDwamLightLayer);

/** Une lumière telle que collectée sur le game thread (monde absolu, en double). */
struct FDwamLightData
{
	FVector Position = FVector::ZeroVector;
	/** Axe X = direction d'émission (spot, rect) ; Y = largeur, Z = hauteur (rect). */
	FQuat4f Rotation = FQuat4f::Identity;
	/** RGB linéaire, température et intensité comprises. */
	FVector3f Color = FVector3f::OneVector;
	float Range = 1000.f;
	float FalloffExponent = 2.f;
	/** Spot : cosinus des demi-angles intérieur et extérieur. */
	float CosInner = 1.f;
	float CosOuter = 0.f;
	/** Rect : demi-largeur et demi-hauteur (cm). */
	FVector2f HalfSize = FVector2f::ZeroVector;
	float ShadowSoftness = 0.5f;
	uint8 Layers = 1;
	EDwamLightType Type = EDwamLightType::Point;
	bool bCastShadow = false;
};

/**
 * Format de la texture des lumières (RGBA32F, une ligne). Miroir exact de Shaders/Lights.ush :
 * toute modification ici se répercute là-bas, et FormatVersion augmente.
 *
 *   pixel 0 : nombre de lumières | version | nombre magique | 0        (vrais flottants)
 *   pixel 1 : motif de test (bits bruts), relu par DwamLightsSelfTest
 *   lumière i, pixel 2 + 2i : position X, Y, Z (relative à la caméra) | rayon englobant
 *   lumière i, pixel 3 + 2i : couleur RGB9E5 | orientation | forme | drapeaux   (entiers, bits bruts)
 */
namespace DwamLightPacking
{
	inline constexpr int32 FormatVersion = 1;
	inline constexpr float Magic = 2225.f;
	inline constexpr int32 HeaderPixels = 2;
	inline constexpr int32 PixelsPerLight = 2;
	inline constexpr int32 ChannelsPerPixel = 4;

	/** Motif de test : NaN avec charge utile, dénormal, tous les bits, zéro négatif. */
	inline constexpr uint32 TestPattern[4] = { 0x7FC12345u, 0x00000001u, 0xFFFFFFFFu, 0x80000000u };

	/** Nombre de lumières que peut porter une texture de cette largeur. */
	DWAMLIGHTINGRUNTIME_API int32 CapacityForWidth(int32 Width);

	/** RGB HDR en 32 bits : 9 bits par canal, exposant partagé sur 5 bits. Maximum ≈ 65408. */
	DWAMLIGHTINGRUNTIME_API uint32 PackColorRGB9E5(const FVector3f& Color);
	DWAMLIGHTINGRUNTIME_API FVector3f UnpackColorRGB9E5(uint32 Packed);

	/** Direction unitaire en octaédrique, 16 + 16 bits. */
	DWAMLIGHTINGRUNTIME_API uint32 PackAxisOct16(const FVector3f& Axis);
	DWAMLIGHTINGRUNTIME_API FVector3f UnpackAxisOct16(uint32 Packed);

	/** Quaternion unitaire : indice de la plus grande composante (2 bits) + les trois autres (10 bits chacune). */
	DWAMLIGHTINGRUNTIME_API uint32 PackQuatSmallest3(const FQuat4f& Rotation);
	DWAMLIGHTINGRUNTIME_API FQuat4f UnpackQuatSmallest3(uint32 Packed);

	/** Deux valeurs 0..1 sur 16 bits chacune (A dans les bits bas). */
	DWAMLIGHTINGRUNTIME_API uint32 PackUnorm16Pair(float A, float B);
	/** Deux demi-flottants (A dans les bits bas). */
	DWAMLIGHTINGRUNTIME_API uint32 PackHalfPair(float A, float B);

	/** Bits 0-1 type | 2 ombre | 3-10 layers | 11-18 douceur d'ombre | 19-26 exposant du falloff × 16. */
	DWAMLIGHTINGRUNTIME_API uint32 PackFlags(const FDwamLightData& Light);
	DWAMLIGHTINGRUNTIME_API float UnpackFalloffExponent(uint32 Flags);

	/** Écrit les 2 pixels d'en-tête (8 mots de 32 bits). */
	DWAMLIGHTINGRUNTIME_API void PackHeader(int32 NumLights, uint32* OutWords);

	/** Écrit les 2 pixels d'une lumière (8 mots de 32 bits). PreViewTranslation : celle de la vue rendue. */
	DWAMLIGHTINGRUNTIME_API void PackLight(const FDwamLightData& Light, const FVector& PreViewTranslation, uint32* OutWords);
}

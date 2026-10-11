#include "DwamLightTypes.h"

#include "Math/Float16.h"
#include <cmath>

DEFINE_LOG_CATEGORY(LogDwamLighting);

namespace DwamLightPacking
{
	namespace Private
	{
		inline uint32 FloatBits(float Value)
		{
			uint32 Bits;
			FMemory::Memcpy(&Bits, &Value, sizeof(Bits));
			return Bits;
		}

		inline uint32 Quantize(float Value01, uint32 MaxValue)
		{
			return (uint32)FMath::RoundToInt(FMath::Clamp(Value01, 0.f, 1.f) * (float)MaxValue);
		}
	}

	int32 CapacityForWidth(int32 Width)
	{
		return FMath::Max(0, (Width - HeaderPixels) / PixelsPerLight);
	}

	// ---------------------------------------------------------------- Couleur

	uint32 PackColorRGB9E5(const FVector3f& Color)
	{
		// Format « shared exponent » standard : N = 9 bits de mantisse, biais B = 15
		const float MaxValue = 65408.f; // (511 / 512) × 2^16
		const float R = FMath::Clamp(Color.X, 0.f, MaxValue);
		const float G = FMath::Clamp(Color.Y, 0.f, MaxValue);
		const float B = FMath::Clamp(Color.Z, 0.f, MaxValue);
		const float MaxChannel = FMath::Max3(R, G, B);
		if (!(MaxChannel > 0.f))
		{
			return 0u;
		}

		// frexp : MaxChannel = f × 2^e avec f dans [0.5, 1) : exposant exact, sans erreur d'arrondi de log2
		int Exponent = 0;
		std::frexp(MaxChannel, &Exponent);
		int32 SharedExp = FMath::Max(-16, Exponent - 1) + 1 + 15;
		float Denominator = std::ldexp(1.f, SharedExp - 15 - 9);
		if (FMath::FloorToInt(MaxChannel / Denominator + 0.5f) == 512)
		{
			Denominator *= 2.f;
			++SharedExp;
		}

		const uint32 RM = (uint32)FMath::FloorToInt(R / Denominator + 0.5f);
		const uint32 GM = (uint32)FMath::FloorToInt(G / Denominator + 0.5f);
		const uint32 BM = (uint32)FMath::FloorToInt(B / Denominator + 0.5f);
		return RM | (GM << 9) | (BM << 18) | ((uint32)SharedExp << 27);
	}

	FVector3f UnpackColorRGB9E5(uint32 Packed)
	{
		const float Scale = std::ldexp(1.f, (int)(Packed >> 27) - 24);
		return FVector3f((float)(Packed & 511u), (float)((Packed >> 9) & 511u), (float)((Packed >> 18) & 511u)) * Scale;
	}

	// ---------------------------------------------------------------- Orientation

	uint32 PackAxisOct16(const FVector3f& Axis)
	{
		FVector3f N = Axis.GetSafeNormal(UE_SMALL_NUMBER, FVector3f::ForwardVector);
		N /= (FMath::Abs(N.X) + FMath::Abs(N.Y) + FMath::Abs(N.Z));

		float U = N.X;
		float V = N.Y;
		if (N.Z < 0.f)
		{
			// Repli de l'hémisphère bas sur les coins du carré
			U = (1.f - FMath::Abs(N.Y)) * (N.X >= 0.f ? 1.f : -1.f);
			V = (1.f - FMath::Abs(N.X)) * (N.Y >= 0.f ? 1.f : -1.f);
		}
		return Private::Quantize(U * 0.5f + 0.5f, 65535u) | (Private::Quantize(V * 0.5f + 0.5f, 65535u) << 16);
	}

	FVector3f UnpackAxisOct16(uint32 Packed)
	{
		const float U = (float)(Packed & 0xFFFFu) / 65535.f * 2.f - 1.f;
		const float V = (float)(Packed >> 16) / 65535.f * 2.f - 1.f;
		FVector3f N(U, V, 1.f - FMath::Abs(U) - FMath::Abs(V));
		const float T = FMath::Clamp(-N.Z, 0.f, 1.f);
		N.X += (N.X >= 0.f) ? -T : T;
		N.Y += (N.Y >= 0.f) ? -T : T;
		return N.GetSafeNormal();
	}

	uint32 PackQuatSmallest3(const FQuat4f& Rotation)
	{
		const FQuat4f Q = Rotation.GetNormalized();
		float C[4] = { Q.X, Q.Y, Q.Z, Q.W };

		int32 Largest = 0;
		for (int32 i = 1; i < 4; ++i)
		{
			if (FMath::Abs(C[i]) > FMath::Abs(C[Largest]))
			{
				Largest = i;
			}
		}
		// q et -q sont la même rotation : on rend la plus grande composante positive, elle se redéduit des trois autres
		const float Sign = (C[Largest] < 0.f) ? -1.f : 1.f;

		uint32 Packed = (uint32)Largest << 30;
		int32 Shift = 0;
		for (int32 i = 0; i < 4; ++i)
		{
			if (i != Largest)
			{
				// Les trois autres composantes tiennent dans [-1/√2, 1/√2]
				Packed |= Private::Quantize(C[i] * Sign * UE_SQRT_2 * 0.5f + 0.5f, 1023u) << Shift;
				Shift += 10;
			}
		}
		return Packed;
	}

	FQuat4f UnpackQuatSmallest3(uint32 Packed)
	{
		const int32 Largest = (int32)(Packed >> 30);
		float C[4];
		float SumSquares = 0.f;
		int32 Shift = 0;
		for (int32 i = 0; i < 4; ++i)
		{
			if (i != Largest)
			{
				C[i] = ((float)((Packed >> Shift) & 1023u) / 1023.f * 2.f - 1.f) * UE_INV_SQRT_2;
				SumSquares += C[i] * C[i];
				Shift += 10;
			}
		}
		C[Largest] = FMath::Sqrt(FMath::Max(0.f, 1.f - SumSquares));
		return FQuat4f(C[0], C[1], C[2], C[3]).GetNormalized();
	}

	// ---------------------------------------------------------------- Forme et drapeaux

	uint32 PackUnorm16Pair(float A, float B)
	{
		return Private::Quantize(A, 65535u) | (Private::Quantize(B, 65535u) << 16);
	}

	uint32 PackHalfPair(float A, float B)
	{
		return (uint32)FFloat16(A).Encoded | ((uint32)FFloat16(B).Encoded << 16);
	}

	uint32 PackFlags(const FDwamLightData& Light)
	{
		const uint32 Falloff = (uint32)FMath::Clamp(FMath::RoundToInt(Light.FalloffExponent * 16.f), 1, 255);
		return ((uint32)Light.Type & 3u)
			| ((Light.bCastShadow ? 1u : 0u) << 2)
			| ((uint32)Light.Layers << 3)
			| (Private::Quantize(Light.ShadowSoftness, 255u) << 11)
			| (Falloff << 19);
	}

	float UnpackFalloffExponent(uint32 Flags)
	{
		return (float)((Flags >> 19) & 255u) / 16.f;
	}

	// ---------------------------------------------------------------- Pixels

	void PackHeader(int32 NumLights, uint32* OutWords)
	{
		OutWords[0] = Private::FloatBits((float)NumLights);
		OutWords[1] = Private::FloatBits((float)FormatVersion);
		OutWords[2] = Private::FloatBits(Magic);
		OutWords[3] = 0u;
		for (int32 i = 0; i < 4; ++i)
		{
			OutWords[4 + i] = TestPattern[i];
		}
	}

	void PackLight(const FDwamLightData& Light, const FVector& PreViewTranslation, uint32* OutWords)
	{
		// La soustraction se fait en double : la précision reste constante loin de l'origine du monde
		const FVector3f Relative(Light.Position + PreViewTranslation);
		OutWords[0] = Private::FloatBits(Relative.X);
		OutWords[1] = Private::FloatBits(Relative.Y);
		OutWords[2] = Private::FloatBits(Relative.Z);
		// Rayon englobant : c'est lui que le shader (et plus tard le culling) teste en premier
		const float BoundRadius = Light.Range + (Light.Type == EDwamLightType::Rect ? Light.HalfSize.Size() : 0.f);
		OutWords[3] = Private::FloatBits(BoundRadius);

		OutWords[4] = PackColorRGB9E5(Light.Color);
		switch (Light.Type)
		{
		case EDwamLightType::Spot:
			OutWords[5] = PackAxisOct16(Light.Rotation.GetAxisX());
			OutWords[6] = PackUnorm16Pair(Light.CosOuter, Light.CosInner);
			break;
		case EDwamLightType::Rect:
			OutWords[5] = PackQuatSmallest3(Light.Rotation);
			OutWords[6] = PackHalfPair(Light.HalfSize.X, Light.HalfSize.Y);
			break;
		default:
			OutWords[5] = PackAxisOct16(Light.Rotation.GetAxisX());
			OutWords[6] = 0u;
			break;
		}
		OutWords[7] = PackFlags(Light);
	}
}

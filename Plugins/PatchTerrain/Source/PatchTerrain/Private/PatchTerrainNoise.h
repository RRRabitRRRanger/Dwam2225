#pragma once

#include "CoreMinimal.h"

/** Bruits partagés par les presets et les expressions (déterministes : même entrée, même sortie sur toutes les plateformes). */
namespace PatchTerrainNoise
{
	/** Bruit de Perlin simple, environ dans [-1, 1]. */
	inline double Perlin(const FVector2D& Q)
	{
		return FMath::PerlinNoise2D(Q);
	}

	/** Bruit fractal normalisé, environ dans [-1, 1]. */
	inline double FBm(const FVector2D& Q, int32 Octaves, double Persistence)
	{
		double Sum = 0.0;
		double Amp = 1.0;
		double Norm = 0.0;
		double Freq = 1.0;
		for (int32 O = 0; O < Octaves; ++O)
		{
			Sum += Amp * FMath::PerlinNoise2D(Q * Freq);
			Norm += Amp;
			Amp *= Persistence;
			Freq *= 2.0;
		}
		return Norm > 0.0 ? Sum / Norm : 0.0;
	}

	/** Valeur pseudo-aléatoire dans [-1, 1] attachée à un nœud entier du réseau (hachage entier, identique partout). */
	inline double LatticeValue(int32 IX, int32 IY)
	{
		uint32 H = static_cast<uint32>(IX) * 374761393u + static_cast<uint32>(IY) * 668265263u;
		H = (H ^ (H >> 13)) * 1274126177u;
		H ^= H >> 16;
		return static_cast<double>(H) / 4294967295.0 * 2.0 - 1.0;
	}

	/** Value noise : valeurs aléatoires aux nœuds entiers, interpolées en quintique. Environ dans [-1, 1], plus « pâteux » que Perlin. */
	inline double Value(const FVector2D& Q)
	{
		const double FX = FMath::FloorToDouble(Q.X);
		const double FY = FMath::FloorToDouble(Q.Y);
		const int32 IX = static_cast<int32>(FX);
		const int32 IY = static_cast<int32>(FY);
		const double TX = Q.X - FX;
		const double TY = Q.Y - FY;
		// Quintique : dérivées première et seconde nulles aux nœuds, pas de facettes visibles
		const double SX = TX * TX * TX * (TX * (TX * 6.0 - 15.0) + 10.0);
		const double SY = TY * TY * TY * (TY * (TY * 6.0 - 15.0) + 10.0);
		const double A = FMath::Lerp(LatticeValue(IX, IY), LatticeValue(IX + 1, IY), SX);
		const double B = FMath::Lerp(LatticeValue(IX, IY + 1), LatticeValue(IX + 1, IY + 1), SX);
		return FMath::Lerp(A, B, SY);
	}

	/** Value noise fractal normalisé, environ dans [-1, 1]. */
	inline double ValueFBm(const FVector2D& Q, int32 Octaves, double Persistence)
	{
		double Sum = 0.0;
		double Amp = 1.0;
		double Norm = 0.0;
		double Freq = 1.0;
		for (int32 O = 0; O < Octaves; ++O)
		{
			Sum += Amp * Value(Q * Freq);
			Norm += Amp;
			Amp *= Persistence;
			Freq *= 2.0;
		}
		return Norm > 0.0 ? Sum / Norm : 0.0;
	}

	/** Bruit à bosses (valeur absolue du Perlin) normalisé dans [0, 1] environ. */
	inline double Billow(const FVector2D& Q, int32 Octaves, double Persistence)
	{
		double Sum = 0.0;
		double Amp = 1.0;
		double Norm = 0.0;
		double Freq = 1.0;
		for (int32 O = 0; O < Octaves; ++O)
		{
			Sum += Amp * FMath::Abs(FMath::PerlinNoise2D(Q * Freq));
			Norm += Amp;
			Amp *= Persistence;
			Freq *= 2.0;
		}
		// |Perlin| dépasse rarement 0.7 : on étire vers [0, 1]
		return Norm > 0.0 ? FMath::Min(Sum / Norm * 1.6, 1.0) : 0.0;
	}

	/** Bruit à crêtes normalisé dans [0, 1]. */
	inline double Ridged(const FVector2D& Q, int32 Octaves, double Persistence)
	{
		double Sum = 0.0;
		double Amp = 1.0;
		double Norm = 0.0;
		double Freq = 1.0;
		for (int32 O = 0; O < Octaves; ++O)
		{
			double N = 1.0 - FMath::Abs(FMath::PerlinNoise2D(Q * Freq));
			N *= N;
			Sum += Amp * N;
			Norm += Amp;
			Amp *= Persistence;
			Freq *= 2.0;
		}
		return Norm > 0.0 ? Sum / Norm : 0.0;
	}
}

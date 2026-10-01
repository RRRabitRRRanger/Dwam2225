#include "PatchTerrainSurface.h"

#include "Algo/BinarySearch.h"
#include "Math/RandomStream.h"
#include "PatchTerrainNoise.h"

namespace PatchTerrainMath
{
	/**
	 * Dérivée au nœud k d'une courbe Catmull-Rom non uniforme (par rapport au paramètre global).
	 * Différence pondérée par les écarts de nœuds ; différence simple aux extrémités.
	 */
	static FVector NodeTangent(const FVector* P, const double* K, int32 Count, int32 k)
	{
		if (Count < 2)
		{
			return FVector::ZeroVector;
		}
		if (k <= 0)
		{
			return (P[1] - P[0]) / (K[1] - K[0]);
		}
		if (k >= Count - 1)
		{
			return (P[Count - 1] - P[Count - 2]) / (K[Count - 1] - K[Count - 2]);
		}
		const double H0 = K[k] - K[k - 1];
		const double H1 = K[k + 1] - K[k];
		const FVector D0 = (P[k] - P[k - 1]) / H0;
		const FVector D1 = (P[k + 1] - P[k]) / H1;
		return (D0 * H1 + D1 * H0) / (H0 + H1);
	}

	/** Hermite cubique sur le segment [Seg, Seg+1], tangentes Catmull-Rom non uniformes. */
	static FVector EvalCurve(const FVector* P, const double* K, int32 Count, int32 Seg, double X)
	{
		if (Count == 1)
		{
			return P[0];
		}
		const double H = K[Seg + 1] - K[Seg];
		const double T = (X - K[Seg]) / H;
		const double T2 = T * T;
		const double T3 = T2 * T;

		const double H00 = 2.0 * T3 - 3.0 * T2 + 1.0;
		const double H10 = T3 - 2.0 * T2 + T;
		const double H01 = -2.0 * T3 + 3.0 * T2;
		const double H11 = T3 - T2;

		return P[Seg] * H00
			+ NodeTangent(P, K, Count, Seg) * (H * H10)
			+ P[Seg + 1] * H01
			+ NodeTangent(P, K, Count, Seg + 1) * (H * H11);
	}

	/** Index du segment contenant X, borné à [0, N-2]. */
	static int32 FindSegment(const TArray<double>& Knots, double X)
	{
		const int32 Upper = Algo::UpperBound(Knots, X);
		return FMath::Clamp(Upper - 1, 0, Knots.Num() - 2);
	}

}

bool FPatchTerrainSurface::Setup(
	TArray<double> InKnotsX,
	TArray<double> InKnotsY,
	TArray<FVector> InPoints,
	const TArray<FPatchTerrainModifier>& InModifiers,
	int32 InSeed,
	TArray<FString>* OutModifierStatus)
{
	bValid = false;
	if (OutModifierStatus)
	{
		OutModifierStatus->Init(FString(), InModifiers.Num());
	}
	KnotsX = MoveTemp(InKnotsX);
	KnotsY = MoveTemp(InKnotsY);
	Points = MoveTemp(InPoints);
	Modifiers.Reset();

	const int32 NX = KnotsX.Num();
	const int32 NY = KnotsY.Num();
	if (NX < 2 || NY < 2 || Points.Num() != NX * NY)
	{
		return false;
	}

	// Les nœuds doivent être strictement croissants (sinon division par zéro dans les tangentes)
	for (int32 I = 1; I < NX; ++I)
	{
		if (KnotsX[I] <= KnotsX[I - 1])
		{
			return false;
		}
	}
	for (int32 J = 1; J < NY; ++J)
	{
		if (KnotsY[J] <= KnotsY[J - 1])
		{
			return false;
		}
	}

	const int32 NumCols = NX - 1;
	const int32 NumRows = NY - 1;

	for (int32 Index = 0; Index < InModifiers.Num(); ++Index)
	{
		const FPatchTerrainModifier& Src = InModifiers[Index];
		if (!Src.bEnabled)
		{
			continue;
		}

		// Les expressions sont compilées une seule fois ici ; une expression invalide est ignorée
		TSharedPtr<FPatchTerrainExpression> Expr;
		if (Src.Type == EPatchTerrainModifierType::Expression)
		{
			Expr = MakeShared<FPatchTerrainExpression>();
			FString Error;
			if (!Expr->Compile(Src.Expression, Error))
			{
				if (OutModifierStatus)
				{
					(*OutModifierStatus)[Index] = TEXT("Erreur : ") + Error;
				}
				continue;
			}
			if (OutModifierStatus)
			{
				(*OutModifierStatus)[Index] = TEXT("OK");
			}
		}

		FPreparedModifier& Mod = Modifiers.AddDefaulted_GetRef();
		Mod.Params = Src;
		Mod.Expression = Expr;

		// Décalage de bruit déterministe : dépend de la seed globale, de la position dans la pile et du SeedOffset
		const uint32 Hash = HashCombine(HashCombine(GetTypeHash(InSeed), GetTypeHash(Index)), GetTypeHash(Src.SeedOffset));
		FRandomStream Stream(static_cast<int32>(Hash));
		Mod.NoiseOffset = FVector2D(Stream.FRandRange(-10000.0, 10000.0), Stream.FRandRange(-10000.0, 10000.0));

		const double Rad = FMath::DegreesToRadians(static_cast<double>(Src.Direction));
		Mod.Dir = FVector2D(FMath::Cos(Rad), FMath::Sin(Rad));

		// Zone : rectangle de cases converti en coordonnées paramétriques
		const int32 C0 = FMath::Clamp(FMath::Min(Src.FirstCell.X, Src.LastCell.X), 0, NumCols - 1);
		const int32 C1 = FMath::Clamp(FMath::Max(Src.FirstCell.X, Src.LastCell.X), 0, NumCols - 1);
		const int32 R0 = FMath::Clamp(FMath::Min(Src.FirstCell.Y, Src.LastCell.Y), 0, NumRows - 1);
		const int32 R1 = FMath::Clamp(FMath::Max(Src.FirstCell.Y, Src.LastCell.Y), 0, NumRows - 1);
		Mod.RectMin = FVector2D(KnotsX[C0], KnotsY[R0]);
		Mod.RectMax = FVector2D(KnotsX[C1 + 1], KnotsY[R1 + 1]);
		if (Src.bWholeTerrain)
		{
			// u, v couvrent alors tout le canevas
			Mod.RectMin = FVector2D(KnotsX[0], KnotsY[0]);
			Mod.RectMax = FVector2D(KnotsX.Last(), KnotsY.Last());
		}
	}

	bValid = true;
	return true;
}

FVector FPatchTerrainSurface::EvaluateBase(double X, double Y) const
{
	if (!bValid)
	{
		return FVector::ZeroVector;
	}

	const int32 NX = KnotsX.Num();
	const int32 NY = KnotsY.Num();

	X = FMath::Clamp(X, KnotsX[0], KnotsX[NX - 1]);
	Y = FMath::Clamp(Y, KnotsY[0], KnotsY[NY - 1]);

	const int32 I = PatchTerrainMath::FindSegment(KnotsX, X);
	const int32 J = PatchTerrainMath::FindSegment(KnotsY, Y);

	// Produit tensoriel : d'abord le long de X sur les lignes J-1..J+2, puis le long de Y.
	// Aux bords, on prend moins de lignes : les tangentes deviennent unilatérales, comme sur la courbe complète.
	const int32 R0 = FMath::Max(0, J - 1);
	const int32 R1 = FMath::Min(NY - 1, J + 2);
	const int32 Count = R1 - R0 + 1;

	FVector Column[4];
	double ColumnKnots[4];
	for (int32 R = R0; R <= R1; ++R)
	{
		Column[R - R0] = PatchTerrainMath::EvalCurve(&Points[R * NX], KnotsX.GetData(), NX, I, X);
		ColumnKnots[R - R0] = KnotsY[R];
	}

	return PatchTerrainMath::EvalCurve(Column, ColumnKnots, Count, J - R0, Y);
}

double FPatchTerrainSurface::ModifierWeight(const FPreparedModifier& Mod, double X, double Y)
{
	if (Mod.Params.bWholeTerrain)
	{
		return 1.0;
	}

	// Distance au rectangle (0 à l'intérieur)
	const double DX = FMath::Max3(Mod.RectMin.X - X, 0.0, X - Mod.RectMax.X);
	const double DY = FMath::Max3(Mod.RectMin.Y - Y, 0.0, Y - Mod.RectMax.Y);
	const double Dist = FMath::Sqrt(DX * DX + DY * DY);

	const double Falloff = Mod.Params.Falloff;
	if (Falloff <= 0.0)
	{
		return Dist > 0.0 ? 0.0 : 1.0;
	}
	return 1.0 - FMath::SmoothStep(0.0, Falloff, Dist);
}

double FPatchTerrainSurface::ApplyModifiers(double Z, double X, double Y) const
{
	using namespace PatchTerrainNoise;

	const FVector2D P(X, Y);

	for (const FPreparedModifier& Mod : Modifiers)
	{
		const double W = ModifierWeight(Mod, X, Y);
		if (W <= 0.0)
		{
			continue;
		}

		const FPatchTerrainModifier& Prm = Mod.Params;
		const double Wavelength = FMath::Max(1.0, static_cast<double>(Prm.Wavelength));
		const FVector2D Q = P / Wavelength + Mod.NoiseOffset;

		switch (Prm.Type)
		{
		case EPatchTerrainModifierType::Waves:
		{
			const double Phase = FVector2D::DotProduct(P, Mod.Dir) / Wavelength;
			Z += W * Prm.Amplitude * FMath::Sin(UE_DOUBLE_TWO_PI * Phase);
			break;
		}
		case EPatchTerrainModifierType::Noise:
		{
			Z += W * Prm.Amplitude * FBm(Q, Prm.Octaves, Prm.Persistence);
			break;
		}
		case EPatchTerrainModifierType::Ridges:
		{
			Z += W * Prm.Amplitude * Ridged(Q, Prm.Octaves, Prm.Persistence);
			break;
		}
		case EPatchTerrainModifierType::Billow:
		{
			Z += W * Prm.Amplitude * Billow(Q, Prm.Octaves, Prm.Persistence);
			break;
		}
		case EPatchTerrainModifierType::ValueNoise:
		{
			Z += W * Prm.Amplitude * ValueFBm(Q, Prm.Octaves, Prm.Persistence);
			break;
		}
		case EPatchTerrainModifierType::Dunes:
		{
			// Phase perturbée par du bruit pour casser la régularité des crêtes
			const double Phase = FVector2D::DotProduct(P, Mod.Dir) / Wavelength + 0.35 * FBm(Q * 0.5, FMath::Min(Prm.Octaves, 3), 0.5);
			const double S = FMath::Frac(Phase);
			// Montée douce au vent (75 % de la période), descente raide sous le vent
			const double Profile = S < 0.75
				? FMath::SmoothStep(0.0, 0.75, S)
				: 1.0 - FMath::SmoothStep(0.75, 1.0, S);
			const double Variation = 0.75 + 0.25 * FBm(Q * 0.25, 2, 0.5);
			Z += W * Prm.Amplitude * Profile * Variation;
			break;
		}
		case EPatchTerrainModifierType::Expression:
		{
			if (Mod.Expression.IsValid())
			{
				FPatchTerrainExpression::FContext Ctx;
				Ctx.X = X;
				Ctx.Y = Y;
				Ctx.Z = Z;
				Ctx.U = (X - Mod.RectMin.X) / FMath::Max(Mod.RectMax.X - Mod.RectMin.X, 1.0);
				Ctx.V = (Y - Mod.RectMin.Y) / FMath::Max(Mod.RectMax.Y - Mod.RectMin.Y, 1.0);
				Ctx.NoiseOffset = Mod.NoiseOffset;
				Z += W * Mod.Expression->Evaluate(Ctx);
			}
			break;
		}
		case EPatchTerrainModifierType::Terraces:
		{
			const double Step = FMath::Max(1.0, static_cast<double>(Prm.TerraceStep));
			const double K = Z / Step;
			const double Floor = FMath::FloorToDouble(K);
			const double Fr = K - Floor;
			// Plus Sharpness est haut, plus la transition entre deux marches est courte
			const double Soft = 1.0 - FMath::Clamp(static_cast<double>(Prm.TerraceSharpness), 0.0, 0.99);
			const double Shaped = FMath::SmoothStep(0.5 - 0.5 * Soft, 0.5 + 0.5 * Soft, Fr);
			const double Terraced = (Floor + Shaped) * Step;
			Z = FMath::Lerp(Z, Terraced, W);
			break;
		}
		}
	}

	return Z;
}

FVector FPatchTerrainSurface::Evaluate(double X, double Y) const
{
	FVector P = EvaluateBase(X, Y);
	if (bValid && Modifiers.Num() > 0)
	{
		const double CX = FMath::Clamp(X, KnotsX[0], KnotsX.Last());
		const double CY = FMath::Clamp(Y, KnotsY[0], KnotsY.Last());
		P.Z = ApplyModifiers(P.Z, CX, CY);
	}
	if (bValid && Stamps.Num() > 0)
	{
		// Les tampons agissent sur la position réelle de la surface (pas sur les coordonnées du canevas)
		P.Z = ApplyStamps(P.Z, P.X, P.Y);
	}
	if (bValid && Lattices.Num() > 0)
	{
		P = ApplyLattices(P);
	}
	return P;
}

void FPatchTerrainSurface::EvaluateWithNormal(double X, double Y, double Epsilon, FVector& OutPosition, FVector& OutNormal) const
{
	OutPosition = Evaluate(X, Y);

	if (!bValid)
	{
		OutNormal = FVector::UpVector;
		return;
	}

	// Différences finies centrées, rabattues à l'intérieur du canevas sur les bords
	const double XA = FMath::Max(X - Epsilon, KnotsX[0]);
	const double XB = FMath::Min(X + Epsilon, KnotsX.Last());
	const double YA = FMath::Max(Y - Epsilon, KnotsY[0]);
	const double YB = FMath::Min(Y + Epsilon, KnotsY.Last());

	const FVector DU = Evaluate(XB, Y) - Evaluate(XA, Y);
	const FVector DV = Evaluate(X, YB) - Evaluate(X, YA);

	// X vers l'avant, Y vers la droite : X ^ Y = +Z dans Unreal
	FVector N = FVector::CrossProduct(DU, DV);
	if (!N.Normalize())
	{
		N = FVector::UpVector;
	}
	OutNormal = N;
}

// ============================================================================ Tampons

void FPatchTerrainSurface::SetStamps(const TArray<FPatchTerrainStampDesc>& InStamps, int32 InSeed, TArray<FString>* OutStatus)
{
	Stamps.Reset();
	if (OutStatus)
	{
		OutStatus->Init(FString(), InStamps.Num());
	}

	for (int32 Index = 0; Index < InStamps.Num(); ++Index)
	{
		const FPatchTerrainStampDesc& Src = InStamps[Index];
		if (Src.HalfSize.X < 1.0 || Src.HalfSize.Y < 1.0)
		{
			continue;
		}

		TSharedPtr<FPatchTerrainExpression> Expr;
		if (Src.Shape == EPatchTerrainStampShape::Expression)
		{
			Expr = MakeShared<FPatchTerrainExpression>();
			FString Error;
			if (!Expr->Compile(Src.Expression, Error))
			{
				if (OutStatus)
				{
					(*OutStatus)[Index] = TEXT("Erreur : ") + Error;
				}
				continue;
			}
			if (OutStatus)
			{
				(*OutStatus)[Index] = TEXT("OK");
			}
		}

		FPreparedStamp& Stamp = Stamps.AddDefaulted_GetRef();
		Stamp.Desc = Src;
		Stamp.Expression = Expr;

		const double Rad = FMath::DegreesToRadians(Src.Yaw);
		Stamp.CosYaw = FMath::Cos(Rad);
		Stamp.SinYaw = FMath::Sin(Rad);

		// Le motif du bruit dépend de la seed globale et de celle du tampon, pas de son rang : réordonner ne change rien
		const uint32 Hash = HashCombine(GetTypeHash(InSeed), GetTypeHash(Src.Seed) + 0x9E3779B9u);
		FRandomStream Stream(static_cast<int32>(Hash));
		Stamp.NoiseOffset = FVector2D(Stream.FRandRange(-10000.0, 10000.0), Stream.FRandRange(-10000.0, 10000.0));

		if (Src.Shape == EPatchTerrainStampShape::Ramp && Src.bAutoFitEnds)
		{
			// Hauteur du terrain sous chaque extrémité, avec seulement les tampons déjà appliqués avant celui-ci
			const int32 Before = Stamps.Num() - 1;
			const FVector2D Axis(Stamp.CosYaw, Stamp.SinYaw);
			const FVector2D C(Src.Center.X, Src.Center.Y);
			const FVector2D Start = C - Axis * Src.HalfSize.X;
			const FVector2D End = C + Axis * Src.HalfSize.X;
			Stamp.RampZ0 = HeightBeforeStamp(Start.X, Start.Y, Before);
			Stamp.RampZ1 = HeightBeforeStamp(End.X, End.Y, Before);
		}
	}
}

double FPatchTerrainSurface::StampShapeValue(const FPreparedStamp& Stamp, double S, double T, double R, double X, double Y, double Z)
{
	const FPatchTerrainStampDesc& D = Stamp.Desc;
	const double Shaping = FMath::Clamp(D.Shaping, 0.0, 1.0);
	const double Freq = FMath::Max(D.Frequency, 0.01);
	const double AbsS = FMath::Abs(S);
	const double AbsT = FMath::Abs(T);

	switch (D.Shape)
	{
	case EPatchTerrainStampShape::Hill:
	{
		const double Round = 0.5 * (1.0 + FMath::Cos(UE_DOUBLE_PI * R));
		const double Pointy = FMath::Square(1.0 - R);
		return FMath::Lerp(Pointy, Round, Shaping);
	}
	case EPatchTerrainStampShape::Plateau:
		// Sommet plat : les bords doux viennent du masque (Douceur des bords)
		return 1.0;

	case EPatchTerrainStampShape::Ramp:
	{
		const double U = 0.5 * (S + 1.0);
		return FMath::Lerp(U, FMath::SmoothStep(0.0, 1.0, U), Shaping);
	}
	case EPatchTerrainStampShape::Ridge:
	{
		const double Round = 0.5 * (1.0 + FMath::Cos(UE_DOUBLE_PI * AbsT));
		const double Sharp = FMath::Pow(1.0 - AbsT, 1.5);
		return FMath::Lerp(Round, Sharp, Shaping);
	}
	case EPatchTerrainStampShape::Valley:
	{
		const double Round = 0.5 * (1.0 + FMath::Cos(UE_DOUBLE_PI * AbsT));
		const double Steep = 1.0 - FMath::SmoothStep(0.15, 0.6, AbsT);
		return -FMath::Lerp(Round, Steep, Shaping);
	}
	case EPatchTerrainStampShape::Crater:
	{
		constexpr double RimRadius = 0.65;
		const double Bowl = -(1.0 - FMath::SmoothStep(0.0, RimRadius, R));
		const double Rim = FMath::Exp(-FMath::Square((R - RimRadius) / 0.15));
		return 0.8 * Bowl + FMath::Lerp(0.15, 1.0, Shaping) * 0.6 * Rim;
	}
	case EPatchTerrainStampShape::Volcano:
	{
		const double Caldera = FMath::Lerp(0.08, 0.35, Shaping);
		const double Cone = FMath::Pow(1.0 - R, 1.4);
		const double Hole = 1.0 - FMath::SmoothStep(0.0, Caldera, R);
		return Cone - Hole * 0.7 * FMath::Pow(1.0 - Caldera, 1.4);
	}
	case EPatchTerrainStampShape::Dunes:
	{
		const FVector2D Q = FVector2D(S, T) * (Freq * 0.5) + Stamp.NoiseOffset;
		const double Phase = 0.5 * (S + 1.0) * Freq + 0.25 * PatchTerrainNoise::FBm(Q, 3, 0.5);
		const double F = FMath::Frac(Phase);
		// Montée douce au vent, descente raide sous le vent
		return F < 0.75 ? FMath::SmoothStep(0.0, 0.75, F) : 1.0 - FMath::SmoothStep(0.75, 1.0, F);
	}
	case EPatchTerrainStampShape::EggBox:
		return FMath::Sin(UE_DOUBLE_PI * Freq * S) * FMath::Sin(UE_DOUBLE_PI * Freq * T);

	case EPatchTerrainStampShape::Saddle:
		return S * S - T * T;

	case EPatchTerrainStampShape::Ripples:
		return FMath::Cos(UE_DOUBLE_TWO_PI * Freq * R) * (1.0 - R);

	case EPatchTerrainStampShape::Pyramid:
		return 1.0 - FMath::Max(AbsS, AbsT);

	case EPatchTerrainStampShape::Cone:
		return 1.0 - R;

	case EPatchTerrainStampShape::Expression:
	{
		if (!Stamp.Expression.IsValid())
		{
			return 0.0;
		}
		FPatchTerrainExpression::FContext Ctx;
		Ctx.X = X;
		Ctx.Y = Y;
		Ctx.Z = Z;
		Ctx.U = 0.5 * (S + 1.0);
		Ctx.V = 0.5 * (T + 1.0);
		Ctx.NoiseOffset = Stamp.NoiseOffset;
		return Stamp.Expression->Evaluate(Ctx);
	}
	}
	return 0.0;
}

double FPatchTerrainSurface::HeightBeforeStamp(double X, double Y, int32 NumStamps) const
{
	// Approximation : on prend la position comme coordonnée du canevas (exact tant que les points ne sont pas décalés en XY)
	FVector P = EvaluateBase(X, Y);
	if (Modifiers.Num() > 0)
	{
		P.Z = ApplyModifiers(P.Z, FMath::Clamp(X, KnotsX[0], KnotsX.Last()), FMath::Clamp(Y, KnotsY[0], KnotsY.Last()));
	}
	return ApplyStamps(P.Z, P.X, P.Y, NumStamps);
}

double FPatchTerrainSurface::ApplyStamps(double Z, double X, double Y, int32 NumStamps) const
{
	const int32 Count = NumStamps == INDEX_NONE ? Stamps.Num() : FMath::Min(NumStamps, Stamps.Num());
	for (int32 StampIndex = 0; StampIndex < Count; ++StampIndex)
	{
		const FPreparedStamp& Stamp = Stamps[StampIndex];
		const FPatchTerrainStampDesc& D = Stamp.Desc;

		// Passage dans le repère du tampon : S, T dans [-1, 1] sur l'emprise
		const double DX = X - D.Center.X;
		const double DY = Y - D.Center.Y;
		const double LX = DX * Stamp.CosYaw + DY * Stamp.SinYaw;
		const double LY = -DX * Stamp.SinYaw + DY * Stamp.CosYaw;
		const double S = LX / D.HalfSize.X;
		const double T = LY / D.HalfSize.Y;
		if (FMath::Abs(S) >= 1.0 || FMath::Abs(T) >= 1.0)
		{
			continue;
		}

		// Contour en superellipse : rond (exposant 2) à carré (exposant 12). La rampe est toujours rectangulaire ;
		// en raccord auto, elle ne s'adoucit que sur les côtés (ses extrémités épousent déjà le terrain).
		const bool bAutoRamp = D.Shape == EPatchTerrainStampShape::Ramp && D.bAutoFitEnds;
		double R;
		if (bAutoRamp)
		{
			R = FMath::Abs(T);
		}
		else if (D.Shape == EPatchTerrainStampShape::Ramp)
		{
			R = FMath::Max(FMath::Abs(S), FMath::Abs(T));
		}
		else
		{
			const double Pw = FMath::Lerp(2.0, 12.0, FMath::Clamp(D.Squareness, 0.0, 1.0));
			R = FMath::Pow(FMath::Pow(FMath::Abs(S), Pw) + FMath::Pow(FMath::Abs(T), Pw), 1.0 / Pw);
		}
		if (R >= 1.0)
		{
			continue;
		}

		// Masque des bords doux : 0 sur le contour, 1 à l'intérieur au-delà de la zone de douceur
		const double Mask = FMath::SmoothStep(0.0, FMath::Max(D.EdgeSoftness, 0.001), 1.0 - R);
		if (Mask <= 0.0)
		{
			continue;
		}

		double H = StampShapeValue(Stamp, S, T, R, X, Y, Z);

		// Détail : marches douces sur la forme normalisée
		if (D.StepCount > 0)
		{
			const double K = H * D.StepCount;
			const double Floor = FMath::FloorToDouble(K);
			const double Soft = FMath::Clamp(D.StepSoftness, 0.01, 1.0);
			const double Shaped = FMath::SmoothStep(0.5 - 0.5 * Soft, 0.5 + 0.5 * Soft, K - Floor);
			H = (Floor + Shaped) / D.StepCount;
		}

		// Détail : bruit ancré au tampon (il se déplace avec lui)
		double NoiseTerm = 0.0;
		if (D.NoiseType != EPatchTerrainStampNoise::None && D.NoiseAmount != 0.0)
		{
			const FVector2D Q = FVector2D(LX, LY) / FMath::Max(D.NoiseScale, 1.0) + Stamp.NoiseOffset;
			const int32 Octaves = FMath::Clamp(D.NoiseOctaves, 1, 10);
			double N = 0.0;
			switch (D.NoiseType)
			{
			case EPatchTerrainStampNoise::Smooth: N = PatchTerrainNoise::FBm(Q, Octaves, 0.5); break;
			case EPatchTerrainStampNoise::Ridged: N = PatchTerrainNoise::Ridged(Q, Octaves, 0.5); break;
			case EPatchTerrainStampNoise::Billow: N = PatchTerrainNoise::Billow(Q, Octaves, 0.5); break;
			case EPatchTerrainStampNoise::Value: N = PatchTerrainNoise::ValueFBm(Q, Octaves, 0.5); break;
			default: break;
			}
			NoiseTerm = D.NoiseAmount * N;
		}

		if (bAutoRamp)
		{
			// Rampe de raccord : relie les hauteurs relevées sous ses extrémités, toujours en mode Aplatir
			const double RampTarget = FMath::Lerp(Stamp.RampZ0, Stamp.RampZ1, H) + NoiseTerm * D.Height;
			Z = FMath::Lerp(Z, RampTarget, Mask);
			continue;
		}
		H += NoiseTerm;

		const double Delta = H * D.Height;
		const double Target = D.Center.Z + Delta;

		switch (D.Blend)
		{
		case EPatchTerrainStampBlend::Add:		Z += Delta * Mask; break;
		case EPatchTerrainStampBlend::Subtract:	Z -= Delta * Mask; break;
		case EPatchTerrainStampBlend::Max:		Z = FMath::Lerp(Z, FMath::Max(Z, Target), Mask); break;
		case EPatchTerrainStampBlend::Min:		Z = FMath::Lerp(Z, FMath::Min(Z, Target), Mask); break;
		case EPatchTerrainStampBlend::Flatten:	Z = FMath::Lerp(Z, Target, Mask); break;
		}
	}
	return Z;
}

// ============================================================================ Lattices

namespace PatchTerrainMath
{
	/** Polynômes de Bernstein de degré N (N + 1 valeurs) en T. */
	static void Bernstein(int32 N, double T, double* Out)
	{
		static const double Binomial[5][5] =
		{
			{ 1, 0, 0, 0, 0 },
			{ 1, 1, 0, 0, 0 },
			{ 1, 2, 1, 0, 0 },
			{ 1, 3, 3, 1, 0 },
			{ 1, 4, 6, 4, 1 },
		};
		for (int32 I = 0; I <= N; ++I)
		{
			Out[I] = Binomial[N][I] * FMath::Pow(T, static_cast<double>(I)) * FMath::Pow(1.0 - T, static_cast<double>(N - I));
		}
	}

	/** Fenêtre qui vaut 0 sur les faces et 1 à l'intérieur au-delà de Soft. */
	static double FaceWindow(double T, double Soft)
	{
		if (Soft <= 0.0)
		{
			return 1.0;
		}
		return FMath::SmoothStep(0.0, Soft, T) * FMath::SmoothStep(0.0, Soft, 1.0 - T);
	}
}

void FPatchTerrainSurface::SetLattices(const TArray<FPatchTerrainLatticeDesc>& InLattices)
{
	Lattices.Reset();
	for (const FPatchTerrainLatticeDesc& Src : InLattices)
	{
		const FIntVector R = Src.Resolution;
		const bool bValidRes = R.X >= 2 && R.X <= 5 && R.Y >= 2 && R.Y <= 5 && R.Z >= 2 && R.Z <= 5;
		const FVector Size = Src.BoxMax - Src.BoxMin;
		if (bValidRes && Src.ControlPoints.Num() == R.X * R.Y * R.Z && Size.GetMin() > 1.0)
		{
			Lattices.Add(Src);
		}
	}
}

FVector FPatchTerrainSurface::ApplyLattices(const FVector& P) const
{
	FVector Out = P;
	for (const FPatchTerrainLatticeDesc& Lat : Lattices)
	{
		const FVector Local = Lat.BoxToTerrain.InverseTransformPosition(Out);
		const FVector N = (Local - Lat.BoxMin) / (Lat.BoxMax - Lat.BoxMin);
		if (N.X <= 0.0 || N.X >= 1.0 || N.Y <= 0.0 || N.Y >= 1.0 || N.Z <= 0.0 || N.Z >= 1.0)
		{
			continue;
		}

		const FIntVector R = Lat.Resolution;
		double BX[5];
		double BY[5];
		double BZ[5];
		PatchTerrainMath::Bernstein(R.X - 1, N.X, BX);
		PatchTerrainMath::Bernstein(R.Y - 1, N.Y, BY);
		PatchTerrainMath::Bernstein(R.Z - 1, N.Z, BZ);

		// FFD de Bézier : avec les points au repos (grille régulière), la déformation est l'identité
		FVector Deformed = FVector::ZeroVector;
		for (int32 K = 0; K < R.Z; ++K)
		{
			for (int32 J = 0; J < R.Y; ++J)
			{
				const double WJK = BY[J] * BZ[K];
				for (int32 I = 0; I < R.X; ++I)
				{
					Deformed += Lat.ControlPoints[(K * R.Y + J) * R.X + I] * (BX[I] * WJK);
				}
			}
		}

		// Fondu près des faces : continuité garantie avec le terrain hors de la boîte
		const double Soft = FMath::Clamp(Lat.Softness, 0.0, 0.5);
		const double W = PatchTerrainMath::FaceWindow(N.X, Soft) * PatchTerrainMath::FaceWindow(N.Y, Soft) * PatchTerrainMath::FaceWindow(N.Z, Soft);

		Out = Lat.BoxToTerrain.TransformPosition(Local + (Deformed - Local) * W);
	}
	return Out;
}

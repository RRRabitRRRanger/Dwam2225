#pragma once

#include "CoreMinimal.h"

/**
 * Petite expression mathématique compilée une fois, puis évaluée très vite (machine à pile),
 * sans dépendance à l'éditeur : fonctionne aussi en build packagé.
 *
 * Variables :
 *   x, y   position dans le canevas (cm)
 *   u, v   position normalisée 0..1 dans la zone du modificateur (le patch)
 *   z      hauteur actuelle (après les modificateurs précédents)
 * Constantes : pi, tau, e
 * Opérateurs : + - * / % ^ (puissance), < > (donnent 0 ou 1), parenthèses
 * Fonctions  : sin cos tan asin acos atan atan2(y,x) abs sqrt exp log pow(a,b)
 *              floor ceil round frac sign min(a,b) max(a,b) mod(a,b)
 *              clamp(x,a,b) lerp(a,b,t) smoothstep(a,b,x) step(a,x)
 *              noise(x,y) fbm(x,y,octaves) ridged(x,y,octaves) value(x,y) vfbm(x,y,octaves)
 *              (bruits dépendant de la seed)
 *
 * Exemple : sin(u*pi) * sin(v*pi) * 800 + fbm(x/3000, y/3000, 4) * 150
 */
class PATCHTERRAIN_API FPatchTerrainExpression
{
public:
	struct FContext
	{
		double X = 0.0;
		double Y = 0.0;
		double Z = 0.0;
		double U = 0.0;
		double V = 0.0;
		/** Décalage appliqué aux coordonnées des bruits (dépend de la seed). */
		FVector2D NoiseOffset = FVector2D::ZeroVector;
	};

	/** Compile la source. En cas d'erreur, retourne false et décrit le problème dans OutError. */
	bool Compile(const FString& Source, FString& OutError);

	bool IsValid() const { return Code.Num() > 0; }

	/** Évalue l'expression (thread-safe). Retourne 0 si le résultat n'est pas un nombre fini. */
	double Evaluate(const FContext& Context) const;

	/** Profondeur de pile maximale supportée à l'évaluation. */
	static constexpr int32 MaxStackDepth = 64;

	enum class EOp : uint8
	{
		Const,
		VarX, VarY, VarZ, VarU, VarV,
		Add, Sub, Mul, Div, Mod, Pow, Neg, Less, Greater,
		Call,
	};

	struct FInstr
	{
		EOp Op = EOp::Const;
		uint8 Func = 0;
		double Value = 0.0;
	};

private:
	TArray<FInstr> Code;
};

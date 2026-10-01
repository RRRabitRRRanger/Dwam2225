#pragma once

#include "CoreMinimal.h"
#include "PatchTerrainTypes.generated.h"

/** Fonction appliquée par un modificateur de terrain. */
UENUM(BlueprintType)
enum class EPatchTerrainModifierType : uint8
{
	/** Ondulation sinusoïdale dans une direction. */
	Waves		UMETA(DisplayName = "Vagues"),
	/** Bruit fractal (fBm) : relief naturel, collines. */
	Noise		UMETA(DisplayName = "Bruit"),
	/** Bruit à crêtes (ridged) : arêtes de montagne. */
	Ridges		UMETA(DisplayName = "Crêtes"),
	/** Bruit à bosses (billow) : relief arrondi, mamelonné. */
	Billow		UMETA(DisplayName = "Bosses"),
	/** Value noise fractal : relief plus pâteux et arrondi que le Perlin. */
	ValueNoise	UMETA(DisplayName = "Bruit valeur"),
	/** Dunes asymétriques (pente douce au vent, raide sous le vent). */
	Dunes		UMETA(DisplayName = "Dunes"),
	/** Quantifie la hauteur courante en paliers (rizières, falaises étagées). */
	Terraces	UMETA(DisplayName = "Terrasses"),
	/** Formule libre (voir FPatchTerrainModifier::Expression). Le résultat est ajouté à la hauteur. */
	Expression	UMETA(DisplayName = "Expression"),
};

/** Forme d'un tampon. Valeurs normalisées : 1 = la hauteur du tampon (scale Z). */
UENUM(BlueprintType)
enum class EPatchTerrainStampShape : uint8
{
	// --- Relief ---
	/** Bosse arrondie. Façonnage : 0 = pointue, 1 = ronde. */
	Hill		UMETA(DisplayName = "Colline"),
	/** Sommet plat, bords doux (réglés par Douceur des bords). À utiliser en mode Aplatir ou Max. */
	Plateau		UMETA(DisplayName = "Plateau"),
	/** Pente le long du X local, de 0 à la hauteur. Façonnage : 0 = droite, 1 = extrémités adoucies. */
	Ramp		UMETA(DisplayName = "Rampe"),
	/** Arête le long du X local. Façonnage : 0 = arrondie, 1 = tranchante. */
	Ridge		UMETA(DisplayName = "Crête"),
	/** Creux le long du X local. Façonnage : 0 = arrondi, 1 = encaissé. */
	Valley		UMETA(DisplayName = "Vallon"),
	/** Cuvette avec rebord. Façonnage : hauteur du rebord. */
	Crater		UMETA(DisplayName = "Cratère"),
	/** Cône avec caldeira. Façonnage : taille de la caldeira. */
	Volcano		UMETA(DisplayName = "Volcan"),
	/** Dunes le long du X local. Fréquence : nombre de dunes. */
	Dunes		UMETA(DisplayName = "Dunes"),

	// --- Mathématiques ---
	/** sin(x)·sin(y). Fréquence : nombre de bosses par côté. */
	EggBox		UMETA(DisplayName = "Boîte à œufs"),
	/** Selle de cheval (paraboloïde hyperbolique) : x² − y². */
	Saddle		UMETA(DisplayName = "Selle"),
	/** Ondes concentriques amorties. Fréquence : nombre d'anneaux. */
	Ripples		UMETA(DisplayName = "Ondulations"),
	Pyramid		UMETA(DisplayName = "Pyramide"),
	Cone		UMETA(DisplayName = "Cône"),

	/** Formule libre (voir Expression). */
	Expression	UMETA(DisplayName = "Expression"),
};

/** Façon dont un tampon se combine avec le terrain en dessous. */
UENUM(BlueprintType)
enum class EPatchTerrainStampBlend : uint8
{
	/** Ajoute la forme (bosses vers le haut). */
	Add			UMETA(DisplayName = "Ajouter"),
	/** Retire la forme (creuse). */
	Subtract	UMETA(DisplayName = "Creuser"),
	/** Garde le plus haut entre le terrain et la forme posée à l'altitude du tampon : fusionne sans additionner. */
	Max			UMETA(DisplayName = "Max"),
	/** Garde le plus bas entre le terrain et la forme posée à l'altitude du tampon. */
	Min			UMETA(DisplayName = "Min"),
	/** Remplace le terrain par la forme posée à l'altitude du tampon : plateaux, routes, rampes taillées. */
	Flatten		UMETA(DisplayName = "Aplatir"),
};

/** Bruit additionnel d'un tampon. */
UENUM(BlueprintType)
enum class EPatchTerrainStampNoise : uint8
{
	None		UMETA(DisplayName = "Aucun"),
	Smooth		UMETA(DisplayName = "Doux (fBm)"),
	Ridged		UMETA(DisplayName = "Crêtes"),
	Billow		UMETA(DisplayName = "Bosses"),
	Value		UMETA(DisplayName = "Valeur"),
};

/**
 * Un étage de la pile de modificateurs.
 * Les modificateurs s'appliquent dans l'ordre, sur la hauteur (Z local) de la surface de base.
 * Toutes les distances sont en unités Unreal (cm), dans l'espace local du terrain.
 */
USTRUCT(BlueprintType)
struct PATCHTERRAIN_API FPatchTerrainModifier
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modificateur")
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modificateur")
	EPatchTerrainModifierType Type = EPatchTerrainModifierType::Noise;

	// --- Zone ---

	/** Si vrai, s'applique partout ; sinon seulement sur le rectangle de cases [FirstCell, LastCell]. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone")
	bool bWholeTerrain = true;

	/** Première case (colonne X, ligne Y), incluse. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone", meta = (EditCondition = "!bWholeTerrain", ClampMin = "0"))
	FIntPoint FirstCell = FIntPoint(0, 0);

	/** Dernière case (colonne X, ligne Y), incluse. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone", meta = (EditCondition = "!bWholeTerrain", ClampMin = "0"))
	FIntPoint LastCell = FIntPoint(0, 0);

	/** Largeur du fondu autour de la zone (cm). 0 = bord net. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone", meta = (EditCondition = "!bWholeTerrain", ClampMin = "0", Units = "cm"))
	float Falloff = 500.f;

	// --- Expression ---

	/**
	 * Formule ajoutée à la hauteur (cm). Variables : x, y (cm dans le canevas), u, v (0..1 dans la zone),
	 * z (hauteur actuelle). Constantes : pi, tau, e. Opérateurs : + - * / % ^ < >.
	 * Fonctions : sin cos tan asin acos atan atan2 abs sqrt exp log pow floor ceil round frac sign
	 * min max mod clamp lerp smoothstep step noise(x,y) fbm(x,y,oct) ridged(x,y,oct) value(x,y) vfbm(x,y,oct).
	 * Ex. dôme dans le patch : sin(u*pi) * sin(v*pi) * 800
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Expression", meta = (EditCondition = "Type == EPatchTerrainModifierType::Expression", EditConditionHides, MultiLine = true))
	FString Expression = TEXT("sin(u*pi) * sin(v*pi) * 800");

	/** Résultat de la compilation de l'expression (mis à jour à chaque reconstruction). */
	UPROPERTY(VisibleAnywhere, Transient, Category = "Expression", meta = (EditCondition = "Type == EPatchTerrainModifierType::Expression", EditConditionHides))
	FString ExpressionStatus;

	// --- Forme ---

	/** Hauteur maximale ajoutée (cm). Ignoré par Terrasses. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forme", meta = (EditCondition = "Type != EPatchTerrainModifierType::Expression", EditConditionHides, Units = "cm"))
	float Amplitude = 300.f;

	/** Longueur d'onde / taille des motifs (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forme", meta = (EditCondition = "Type != EPatchTerrainModifierType::Expression", EditConditionHides, ClampMin = "1", Units = "cm"))
	float Wavelength = 4000.f;

	/** Direction des vagues / dunes, en degrés dans le plan XY. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forme", meta = (EditCondition = "Type != EPatchTerrainModifierType::Expression", EditConditionHides, Units = "deg"))
	float Direction = 0.f;

	/** Nombre d'octaves pour Bruit / Crêtes / Dunes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forme", meta = (EditCondition = "Type != EPatchTerrainModifierType::Expression", EditConditionHides, ClampMin = "1", ClampMax = "10"))
	int32 Octaves = 4;

	/** Atténuation d'amplitude entre deux octaves (0.5 = classique). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forme", meta = (EditCondition = "Type != EPatchTerrainModifierType::Expression", EditConditionHides, ClampMin = "0", ClampMax = "1"))
	float Persistence = 0.5f;

	/** Hauteur d'une marche (Terrasses), en cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forme", meta = (EditCondition = "Type != EPatchTerrainModifierType::Expression", EditConditionHides, ClampMin = "1", Units = "cm"))
	float TerraceStep = 400.f;

	/** 0 = marches très douces, 1 = falaises nettes (Terrasses). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forme", meta = (EditCondition = "Type != EPatchTerrainModifierType::Expression", EditConditionHides, ClampMin = "0", ClampMax = "0.99"))
	float TerraceSharpness = 0.6f;

	/** Décale le motif aléatoire de ce modificateur sans changer la seed globale. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forme")
	int32 SeedOffset = 0;
};

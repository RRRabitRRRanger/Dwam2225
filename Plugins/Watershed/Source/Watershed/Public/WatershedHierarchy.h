#pragma once

#include "CoreMinimal.h"

/** Description de la grille de hauteurs (2,5D). */
struct FWatershedGridDesc
{
	int32 SizeX = 0;
	int32 SizeY = 0;

	/** Côté d'une cellule (cm). */
	double CellSize = 200.0;

	/** Les bords de la grille évacuent l'eau (elle sort de la région). Sinon, bords fermés. */
	bool bEdgesDrain = true;

	int32 Num() const { return SizeX * SizeY; }
};

/**
 * Hiérarchie de dépressions + Fill–Spill–Merge, sur CPU.
 *
 * D'après Barnes, Callaghan & Wickert (2020-2021), « Computing water flow through complex landscapes »,
 * Part 2 (Depression hierarchy) et Part 3 (Fill–Spill–Merge), Earth Surface Dynamics.
 *
 * Construction :
 *  1. chaque creux (groupe de cellules sans voisin plus bas) devient une feuille ;
 *  2. remplissage par priorité (priority flood) depuis les creux et les bords : chaque cellule reçoit
 *     l'étiquette de la feuille où elle s'écoule ; là où deux étiquettes se touchent, on note le col
 *     (hauteur la plus basse pour passer de l'une à l'autre) ;
 *  3. les cols sont traités du plus bas au plus haut (union-find) : deux bassins qui partagent leur
 *     col le plus bas fusionnent en un méta-bassin ; un bassin qui touche l'extérieur y déborde.
 * Le résultat est un arbre binaire : chaque nœud a un col, une capacité, et un frère où va son surplus.
 *
 * Volume d'un bassin au niveau h : somme, sur les cellules de son sous-arbre plus basses que h,
 * de (h - z) × aire. Calculé à partir des hauteurs triées de chaque feuille : la précision des niveaux
 * ne dépend pas d'un découpage en tranches.
 *
 * Eau : un bassin reçoit de l'eau, monte ; plein, il déborde chez son frère ; quand les deux frères
 * sont pleins, ils fusionnent en un seul lac (le parent), qui continue de monter.
 *
 * Classe sans UObject : construite sur un thread de fond, puis échangée sur le thread de jeu.
 */
class WATERSHED_API FWatershedHierarchy
{
public:
	/** Nœud 0 : l'extérieur (bords, trous). L'eau qui y arrive est perdue. */
	static constexpr int32 OutsideNode = 0;

	struct FNode
	{
		/** Méta-bassin né de la fusion avec le frère (INDEX_NONE pour une racine). */
		int32 Parent = INDEX_NONE;
		int32 Left = INDEX_NONE;
		int32 Right = INDEX_NONE;

		/** Où part le surplus : le frère, ou OutsideNode. */
		int32 Overflow = INDEX_NONE;

		/** Feuille du sous-arbre du frère qui reçoit le surplus (côté du col). */
		int32 GeoLink = INDEX_NONE;

		/** Cellule du col (côté de ce bassin). */
		int32 OutletCell = INDEX_NONE;

		/** Hauteur du col (cm). Infinie pour une racine fermée. */
		double OutletZ = TNumericLimits<double>::Max();

		/** Volume d'eau au niveau du col (cm³). Infini pour une racine fermée. */
		double Capacity = TNumericLimits<double>::Max();

		/** Feuilles du sous-arbre : LeafOrder[LeafBegin .. LeafEnd). */
		int32 LeafBegin = 0;
		int32 LeafEnd = 0;

		/** Nombre de cellules du sous-arbre, et hauteurs extrêmes. */
		int32 NumCells = 0;
		double MinZ = 0.0;
		double MaxZ = 0.0;

		/** Déborde vers l'extérieur. */
		bool bToOutside = false;

		// --- État de l'eau
		double Water = 0.0;
		/** Lac actif : le plus haut nœud dont tout le sous-arbre est fusionné. */
		bool bActive = false;
	};

	/**
	 * Construit la hiérarchie. Heights : SizeX × SizeY hauteurs (cm), indice = X + Y × SizeX.
	 * Une hauteur NaN est un trou : l'eau s'en va par là.
	 */
	void Build(const FWatershedGridDesc& InDesc, TArrayView<const float> InHeights);

	bool IsBuilt() const { return Nodes.Num() > 0; }

	/** Vide toute l'eau. */
	void ResetWater();

	/** Ajoute de l'eau (cm³) qui tombe sur une cellule. Retourne le volume perdu vers l'extérieur. */
	double AddWater(int32 Cell, double Volume);

	/** Lac actif qui contient la cellule (INDEX_NONE si la cellule s'écoule vers l'extérieur). */
	int32 GetLakeOfCell(int32 Cell) const;

	/** Niveau de l'eau d'un nœud (cm). Sans eau : le fond du bassin. */
	double GetLevel(int32 Node) const;

	/** Volume (cm³) que contiendrait le sous-arbre d'un nœud si l'eau était au niveau Z. */
	double VolumeAtLevel(int32 Node, double Z) const;

	const FWatershedGridDesc& GetDesc() const { return Desc; }
	const TArray<FNode>& GetNodes() const { return Nodes; }
	float GetHeight(int32 Cell) const { return Heights[Cell]; }
	int32 GetLabel(int32 Cell) const { return Labels[Cell]; }
	int32 GetNumLeaves() const { return NumLeaves; }
	double GetCellArea() const { return Desc.CellSize * Desc.CellSize; }

	/** Volume total perdu vers l'extérieur depuis le dernier ResetWater (cm³). */
	double GetLostVolume() const { return LostVolume; }

private:
	FWatershedGridDesc Desc;
	TArray<float> Heights;
	/** Feuille où s'écoule chaque cellule (OutsideNode pour les bords et les trous). */
	TArray<int32> Labels;
	TArray<FNode> Nodes;
	int32 NumLeaves = 0;

	/** Feuilles dans l'ordre d'un parcours en profondeur : le sous-arbre d'un nœud est contigu. */
	TArray<int32> LeafOrder;

	/** Hauteurs triées des cellules de chaque feuille, et sommes cumulées. */
	TArray<int32> LeafZStart;     // NumLeaves + 1
	TArray<float> SortedZ;        // une entrée par cellule, groupées par feuille
	TArray<double> PrefixZ;       // par feuille : Count + 1 entrées, décalées de l'indice de feuille

	double LostVolume = 0.0;

	/** Cache des niveaux (recalculés quand l'eau change). */
	mutable TArray<double> CachedLevel;
	mutable TBitArray<> LevelValid;

	void ComputeLeafRanges();
	int32 GetActiveAncestor(int32 Node) const;
	void InvalidateLevel(int32 Node) { LevelValid[Node] = false; }
};

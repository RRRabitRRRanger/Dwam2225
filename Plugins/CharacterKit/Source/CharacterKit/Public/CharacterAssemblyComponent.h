#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CharacterPreset.h"
#include "CharacterAssemblyComponent.generated.h"

class USkeletalMeshComponent;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * Assemble un personnage sur le squelette animé de l'acteur : un UStaticMeshComponent par pièce, attaché à son os.
 * La description vient d'un préréglage (asset) ou est propre à ce component (modifiable en jeu, sauvegardable).
 * Les pièces sont recréées à chaque modification, dans l'éditeur comme en jeu (elles ne sont jamais sauvegardées).
 */
UCLASS(ClassGroup = (Character), meta = (BlueprintSpawnableComponent))
class CHARACTERKIT_API UCharacterAssemblyComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** Vrai : la description vient du préréglage. Faux : la description propre au component. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assembly")
	bool bUsePreset = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assembly", meta = (EditCondition = "bUsePreset"))
	TObjectPtr<UCharacterPreset> Preset;

	/** Description propre au component (utilisée quand bUsePreset est faux). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assembly", meta = (EditCondition = "!bUsePreset"))
	FCharacterDescription Description;

	/** Nom du composant squelette qui porte les pièces ; vide = le premier trouvé sur l'acteur. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assembly")
	FName SkeletonComponentName;

	/** Cache le mesh du squelette : seul le mannequin invisible anime les pièces. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assembly")
	bool bHideSkeletonMesh = false;

	/** Matériau des yeux posés sur les ancres de type Eye (nœud Custom avec /Plugin/SDFEyes/Eyelids.ush). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assembly")
	TObjectPtr<UMaterialInterface> EyeMaterial;

	/** Recrée toutes les pièces. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Assembly")
	void Assemble();

	/** Supprime les pièces créées. */
	UFUNCTION(BlueprintCallable, Category = "Assembly")
	void ClearParts();

	/** La description en cours (celle du préréglage, ou la propre). */
	UFUNCTION(BlueprintPure, Category = "Assembly")
	FCharacterDescription GetCurrentDescription() const;

	/** Remplace la description (écran du joueur, chargement d'une sauvegarde) et réassemble. */
	UFUNCTION(BlueprintCallable, Category = "Assembly")
	void SetDescription(const FCharacterDescription& NewDescription);

	/** Remplace le mesh d'un emplacement (le swap des vêtements). Copie le préréglage dans la description propre si besoin. */
	UFUNCTION(BlueprintCallable, Category = "Assembly")
	bool SetPartMesh(FName Slot, UStaticMesh* NewMesh);

	UFUNCTION(BlueprintPure, Category = "Assembly")
	UStaticMeshComponent* GetPartComponent(FName Slot) const;

	/** Le component créé pour une ancre (un œil : USDFEyeComponent ; un point : USceneComponent). */
	UFUNCTION(BlueprintPure, Category = "Assembly")
	USceneComponent* GetAnchorComponent(FName Anchor) const;

	/** Ajoute une ancre à la description propre (copie le préréglage si besoin) et réassemble. */
	UFUNCTION(BlueprintCallable, Category = "Assembly")
	void AddAnchor(const FCharacterAnchor& Anchor);

protected:
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	USkeletalMeshComponent* FindSkeleton() const;
	const FCharacterDescription* GetSource() const;

	/** Passe sur la description propre, en partant du préréglage. */
	void TakeOwnDescription();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> PartComponents;

	UPROPERTY(Transient)
	TArray<FName> PartSlots;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USceneComponent>> AnchorComponents;

	UPROPERTY(Transient)
	TArray<FName> AnchorNames;
};

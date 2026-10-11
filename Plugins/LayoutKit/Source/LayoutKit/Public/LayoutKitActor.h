#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GraphKitTypes.h"
#include "LayoutKitActor.generated.h"

class UDynamicMeshComponent;
class UMaterialInterface;

/**
 * Un bâtiment (ou un objet) composé comme une interface.
 *
 * Le graphe Layout décrit les boîtes (nœuds, charge utile « Layout Kit Box ») et leurs attaches
 * (arêtes parent -> enfant, charge utile « Layout Kit Attach »). Le mesh est reconstruit à chaque
 * modification dans l'éditeur.
 *
 * L'échelle de l'acteur est lue comme une **taille demandée** : les dimensions étirables la suivent,
 * les fixes gardent leur taille, et le mesh n'est jamais étiré.
 */
UCLASS(meta = (DisplayName = "Layout Kit Actor"))
class LAYOUTKIT_API ALayoutKitActor : public AActor
{
	GENERATED_BODY()

public:
	ALayoutKitActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Layout Kit")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Layout Kit")
	TObjectPtr<UDynamicMeshComponent> Mesh;

	/** Les boîtes et leurs attaches. Orienté : chaque arête va du parent vers l'enfant. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout Kit")
	FGraphKitGraph Layout;

	/** La boîte de départ (posée au centre de l'acteur). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout Kit")
	FName RootBox = TEXT("Corps");

	/**
	 * Niveau d'abstraction (LOA) : 0 = le plus détaillé ; en montant, les boîtes se replient (s'effacent ou se fondent
	 * dans leur parent) jusqu'à « une maison Pokémon ». Entre deux niveaux, on les voit se déplier.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout Kit", meta = (DisplayName = "LOA", ClampMin = "0", UIMin = "0", UIMax = "4"))
	float LOA = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout Kit")
	TObjectPtr<UMaterialInterface> Material;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout Kit")
	bool bGenerateCollision = true;

	/** Ce qui ne va pas dans le graphe (affiché ici, et dans le log). */
	UPROPERTY(VisibleAnywhere, Transient, Category = "Layout Kit")
	TArray<FString> Problems;

	/** Change le niveau d'abstraction et reconstruit (pour l'animer en jeu). */
	UFUNCTION(BlueprintCallable, Category = "Layout Kit")
	void SetLOA(float NewLOA);

	/** Reconstruit le mesh. */
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Layout Kit")
	void Rebuild();

	/** Remet l'exemple de départ : un corps aux coins arrondis et une entrée qui dépasse. */
	UFUNCTION(CallInEditor, Category = "Layout Kit")
	void ResetToExample();

	virtual void OnConstruction(const FTransform& Transform) override;

private:
	void InitExample();
};

// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GhostRevengeSystemComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GHOSTREVENGESYSTEMRUNTIME_API UGhostRevengeSystemComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UGhostRevengeSystemComponent();

	/** Returns Player Controller of this component. */
	UFUNCTION(BlueprintPure, Category = "C++")
	class APlayerController* GetPlayerController() const;
	class APlayerController* GetPlayerControllerChecked() const;

	/** Returns owner (main) player character */
	UFUNCTION(BlueprintPure, Category = "C++")
	class APlayerCharacter* GetMainPlayerCharacter() const;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	/** Called when this level actor is reconstructed or added on the Generated Map, on both server and clients. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnAddedToLevel(class UMapComponent* MapComponent);

	/** Called right before owner actor going to remove from the Generated Map. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnPostRemovedFromLevel(class UMapComponent* MapComponent, UObject* DestroyCauser);

	/** Listen game states to switch character skin. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnGameStateChanged(ECurrentGameState CurrentGameState);

	/** Register into world subsystem main character */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void RegisterMainCharacter();
};

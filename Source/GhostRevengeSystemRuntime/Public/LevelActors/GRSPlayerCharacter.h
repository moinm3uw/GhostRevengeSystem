// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#pragma once

#include "CoreMinimal.h"
#include "Components/MapComponent.h"
#include "GameFramework/Character.h"
#include "GRSPlayerCharacter.generated.h"

/**
 * Ghost Players (only for players, no AI) whose goal is to perform revenge as ghost (spawned on side of map).
 * Copy the died player mesh and skin.
 */
UCLASS()
class GHOSTREVENGESYSTEMRUNTIME_API AGRSPlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	/** Sets default values for this character's properties */
	AGRSPlayerCharacter(const FObjectInitializer& ObjectInitializer);

	/** Called every frame */
	virtual void Tick(float DeltaTime) override;

	/** Called to bind functionality to input */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	/** Current player character, set once game state changes into in-game
	 * Is used as a reference character to init ghost revenge character. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Transient, Category = "C++", meta = (BlueprintProtected, DisplayName = "Current Player Character"))
	TObjectPtr<APlayerCharacter> PlayerCharacterInternal;

	/** Current skeletal mesh component, set once game state changes into in-game */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Transient, Category = "C++", meta = (BlueprintProtected, DisplayName = "Current My Skeletal Mesh Component"))
	TObjectPtr<UMapComponent> MapComponentInternal;

	/** The static mesh nameplate (background material of the player name). */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "C++", meta = (BlueprintProtected, DisplayName = "Nameplate Mesh Component"))
	TObjectPtr<class UStaticMeshComponent> NameplateMeshInternal = nullptr;

	/** 3D widget component that displays the player name above the character. */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "C++", meta = (BlueprintProtected, DisplayName = "Player Name 3D Widget Component"))
	TObjectPtr<class UWidgetComponent> PlayerName3DWidgetComponentInternal = nullptr;
	
	/** Mesh of component. */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "C++", meta = (BlueprintProtected, DisplayName = "Mesh Component"))
	TObjectPtr<class UMeshComponent> MeshComponentInternal = nullptr;

	/*********************************************************************************************
	 * Player Mesh
	 ********************************************************************************************* */
public:
	friend class UMyCheatManager;

	/** Returns the Skeletal Mesh of ghost revenge character. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "C++")
	class UMySkeletalMeshComponent* GetMySkeletalMeshComponent() const;
	UMySkeletalMeshComponent& GetMeshChecked() const;

	/** Set and apply default skeletal mesh for this player.
	 * @param bForcePlayerSkin If true, will force the bot to change own skin to look like a player. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "C++")
	void SetDefaultPlayerMeshData(bool bForcePlayerSkin = false);
	
};

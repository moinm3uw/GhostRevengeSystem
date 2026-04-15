// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#pragma once

#include "Subsystems/ModularGameFeaturePluginSubsystem.h"

#include "GRSWorldSubSystem.generated.h"

enum class EGRSCharacterSide : uint8;

/**
 * Implements the world subsystem to access different components in the module
 */
UCLASS(BlueprintType, Blueprintable)
class GHOSTREVENGESYSTEMRUNTIME_API UGRSWorldSubSystem : public UModularGameFeaturePluginSubsystem
{
	GENERATED_BODY()

	/*********************************************************************************************
	 * Subsystem's Lifecycle
	 **********************************************************************************************/

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGRSOnInitialize);

	/** Returns this Subsystem, is checked and will crash if it can't be obtained.*/
	static UGRSWorldSubSystem& Get();
	static UGRSWorldSubSystem& Get(const UObject* WorldContextObject);

protected:
	/** Subscribes to local pawn ready event */
	virtual void OnGameFeatureInitialize_Implementation() override;

	/** Clears all transient data created by this subsystem */
	virtual void OnGameFeatureDeinitialize_Implementation() override;

	/** Called when the local player character is spawned, possessed, and replicated. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void OnLocalPawnReady(const struct FGameplayEventData& Payload);

	/** Checks if all components present and invokes initialization */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void TryInit();

	/** Cleanup used on unloading module to remove properties that should not be available by other objects. */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void PerformCleanUp();

public:
	/** Checks if the system is ready to load */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	bool IsReady();

	/*********************************************************************************************
	 * Side Collisions actors
	 **********************************************************************************************/
protected:
	/** Current Collision Manager Component used to identify if MGF is ready to be loaded */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Transient, AdvancedDisplay, Category = "[GhostRevengeSystem]")
	TObjectPtr<class UGhostRevengeCollisionComponent> CollisionMangerComponent;

	/** Left Side collision */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected, DisplayName = "Left Side Collision"))
	TObjectPtr<class AActor> LeftSideCollision;

	/** Right Side collision */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected, DisplayName = "Right Side Collision"))
	TObjectPtr<class AActor> RightSideCollision;

public:
	/** Register collision manager component used to track if all components loaded and MGF ready to initialize */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void RegisterCollisionManagerComponent(class UGhostRevengeCollisionComponent* NewCollisionManagerComponent);

	/** Add spawned collision actors to be cached */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	void AddCollisionActor(class AActor* Actor);

	/** Returns TRUE if collision are spawned */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	bool IsCollisionsSpawned();

	/** Returns left side spawned collision or nullptr */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	FORCEINLINE class AActor* GetLeftCollisionActor() const { return LeftSideCollision ? LeftSideCollision : nullptr; }

	/** Returns right side spawned collision or nullptr */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	FORCEINLINE class AActor* GetRightCollisionActor() const { return RightSideCollision ? RightSideCollision : nullptr; }

	/** Clears cached collision manager component */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void UnregisterCollisionManagerComponent();

	/** Clear cached collisions */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	void ClearCollisions();

protected:
	/** Contains list of player characters that were eliminated at least once per game(round) and character can't be a ghost anymore */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Transient, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected, DisplayName = "Dead Player Characters"))
	TArray<class ABmrPawn*> RevivedPlayerCharacters;

public:
	/** Checks if the target Player was already revived. Player can be revived only once
	 * @param PlayerToRevive The BmrPawn to revive
	 * @return false if player was revived once in game */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	bool IsRevivable(const ABmrPawn* PlayerToRevive);

	/** Set a player character as it was revived once */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	void SetRevivedPlayer(ABmrPawn* PlayerToRevive);

	/** Reset revived players so they can be ghosts again */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	void ResetRevivedPlayers();

	/*********************************************************************************************
	 * Ghost Characters
	 **********************************************************************************************/
protected:
	/** Current Character Manager Component */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Transient, AdvancedDisplay, Category = "[GhostRevengeSystem]")
	TObjectPtr<class UGRSGhostCharacterManagerComponent> CharacterManagerComponent;

	/** Ghost character spawned on left side of the map */
	UPROPERTY(VisibleDefaultsOnly, Category = "[GhostRevengeSystem]")
	TObjectPtr<class AGRSPlayerCharacter> GhostCharacterLeftSide;

	/** Ghost character spawned on right side of the map */
	UPROPERTY(VisibleDefaultsOnly, Category = "[GhostRevengeSystem]")
	TObjectPtr<class AGRSPlayerCharacter> GhostCharacterRightSide;

public:
	/** Register character manager component. */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void RegisterCharacterManagerComponent(class UGRSGhostCharacterManagerComponent* NewCharacterManagerComponent);

	/** Register character manager component. */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	FORCEINLINE class UGRSGhostCharacterManagerComponent* GetGRSCharacterManagerComponent() const { return CharacterManagerComponent; }

	/** Register ghost character to obtain it's side NONE if all sides occupied  */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	EGRSCharacterSide RegisterGhostCharacter(class AGRSPlayerCharacter* GhostPlayerCharacter);

	/** Returns currently available ghost character or nullptr if there is no available ghosts. */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	class AGRSPlayerCharacter* GetAvailableGhostCharacter();

	/*********************************************************************************************
	 * Pawn Component
	 **********************************************************************************************/
protected:
	/** Pawn Components attached to BmrPawn to track Pawn's state change */
	UPROPERTY(VisibleDefaultsOnly, Category = "[GhostRevengeSystem]")
	TArray<TObjectPtr<class UGrsPawnComponent>> PawnComponents;

public:
	/** Register a new Pawn component to track the pawn state */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void RegisterPawnComponent(class UGrsPawnComponent* NewPawnComponent);

	/** Returns all available Pawn */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	TArray<class UGrsPawnComponent*> GetPawnComponents() const;

	/** Clears the registered pawn component once it deleted  */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void UnRegisterPawnComponent(class UGrsPawnComponent* PawnComponentToUnregister);

	/** Clears cached character manager component. */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void UnregisterCharacterManagerComponent();

	/** Clear cached ghost character by reference */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	void UnregisterGhostCharacter(class AGRSPlayerCharacter* GhostPlayerCharacter);

	/** Clear cached ghost character references */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	void ClearGhostCharacters();

	/*********************************************************************************************
	 * PlayerState Component
	 **********************************************************************************************/
protected:
	/** Pawn Components attached to BmrPawn to track Pawn's state change */
	UPROPERTY(VisibleDefaultsOnly, Category = "[GhostRevengeSystem]")
	TArray<TObjectPtr<class UGrsPlayerStateComponent>> PlayerStateComponents;

public:
	/** Register a new player state component */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void RegisterPlayerStateComponent(class UGrsPlayerStateComponent* NewPlayerStateComponent);

	/** Unregister a player state component */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void UnRegisterPlayerStateComponent(class UGrsPlayerStateComponent* PlayerStateComponent);

	/** Find a player state component by player ID or nullptr
	 * @param TargetPlayerID to obtain his player state component  */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	UGrsPlayerStateComponent* GetPlayerStateComponent(int32 TargetPlayerID);

	/*********************************************************************************************
	 * Treasury (temp)
	 **********************************************************************************************/
protected:
	/** Listen game states to switch character skin. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void OnGameStateChanged(const struct FGameplayEventData& Payload);
	
	/** Listen end game states to show/hide HUD temporarry */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void OnEndGameStateChanged(EBmrEndGameState EndGameState);
};

// Copyright (c)  Valerii Rotermel & Yevhenii Selivanov

#pragma once

// UE
#include "AbilitySystemInterface.h"
#include "ActiveGameplayEffectHandle.h"
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"

#include "GrsPlayerStateComponent.generated.h"

/**
 * The component is attached to BmrPlayerState primarily to take care of the GAS abilities: revive, bomb spawn ability.
 *
 * Grants abilities: review and bomb spawn when game started ( game state changed to InGame)
 * Remove abilities: review and bomb spawn when game is about to start (game state changed to GameStarting) or MGF is unloaded (Unregistered)
 * When a ghost player eliminates a player/bot, component applies revive ability to return from a ghost (GrsPawn) to a regular player (BmrPlayer)
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GHOSTREVENGESYSTEMRUNTIME_API UGrsPlayerStateComponent
	: public UActorComponent
	  , public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	/** Sets default values for this component's properties */
	UGrsPlayerStateComponent();

	/** Returns the player state from attached BmrPlayerState component */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	class ABmrPlayerState* GetCurrentPlayerState() const;
	class ABmrPlayerState& GetCurrentPlayerStateChecked() const;

protected:
	/** Called when the game starts */
	virtual void BeginPlay() override;

	/** Called as part of MGF(GFP) lifecycle when unload happens */
	virtual void OnUnregister() override;

	/** Starting point once whole module is ready(loaded) to be initialized */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void OnInitialize(const struct FGameplayEventData& Payload);

	/** Listen game states to grant revive ability for player character  */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void OnGameStateChanged(const struct FGameplayEventData& Payload);

	/** Is increased when this player kills an opponent */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void OnOpponentsKilledNumChanged(int32 OpponentsKilledNum);

	/** Tries to revive main player character when a ghost eliminates an enemy on level including elimination of bots */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void TryReviveCharacter();

	/** Returns the Ability System Component from the Player State.
	 * In blueprints, call 'Get Ability System Component' as interface function. */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	/*********************************************************************************************
	 * Revive ability
	 **********************************************************************************************/
protected:
	/** Cached reference to a previous GrsPawn that was possessing this PlayerState. Used to indicate that an elimination done by this PlayerState was from previously possessed GrsPawn.
	 * At apply a revive ability current possessed pawn could be different from GrsPawn. */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Transient, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected, DisplayName = "Possessed GrsPawn"))
	TObjectPtr<class AGrsPawn> PreviousGrsPawn = nullptr; 
	
public:
	/** Assign previous GrsPawn reference to track an elimination done by GrsPawn  */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	void AssignPreviousGrsPawn(class AGrsPawn* NewGrsPawn);
	
	/** Apply a revive ability that will restore regular player character
	 * @param PlayerCharacter a target player character to revive required to obtain location
	 */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	void RevivePlayerCharacter(class ABmrPawn* PlayerCharacter);

	/** Grant to a player revive GAS effect */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void GrantPlayerReviveEffect();

	/** To Remove Revive applied gameplay effect */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	void RemoveAppliedReviveGameplayEffect();

	/*********************************************************************************************
	 * Bomb spawning ability that automatically explodes after a certain time
	 **********************************************************************************************/
protected:
	/** Cached handle of applied explosion (bomb spawning) effect */
	FActiveGameplayEffectHandle AppliedBombSpawnEffectHandle;

public:
	/** Returns handle of current applied ability effect  */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	FORCEINLINE FActiveGameplayEffectHandle GetAppliedBombSpawningEffectHandle() const { return AppliedBombSpawnEffectHandle; }

	/** To apply explosion (bomb spawning) gameplay effect */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	void ApplyBombSpawningGameplayEffect();

	/** To Remove applied explosion (bomb spawning) gameplay effect */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	void RemoveBombSpawningGameplayEffect();
};
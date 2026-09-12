// Copyright (c)  Valerii Rotermel & Yevhenii Selivanov

#pragma once

// UE
#include "AbilitySystemInterface.h"
#include "ActiveGameplayEffectHandle.h"
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"

#include "GrsPlayerStateComponent.generated.h"

class ABmrPlayerState;
/**
 * The component is attached to BmrPlayerState primarily to take care of the GAS abilities: revive, bomb spawn ability.
 *
 * Grants abilities: review and bomb spawn when game started ( game state changed to InGame)
 * Remove abilities: review and bomb spawn when game is about to start (game state changed to GameStarting) or GFP is unloaded (Unregistered)
 * When a ghost player eliminates a player/bot, component applies revive ability to return from a ghost (GrsPawn) to a regular player (BmrPlayer)
 *
 * Owns the replicated 'was already revived' state of its player, so a player can be revived only once per match.
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
	UFUNCTION(BlueprintPure, Category = "[GhostRevengeSystem]")
	ABmrPlayerState* GetCurrentPlayerState() const;
	ABmrPlayerState& GetCurrentPlayerStateChecked() const;

protected:
	/** Returns properties that are replicated for the lifetime of the actor channel */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Called when the game starts */
	virtual void BeginPlay() override;

	/** Called as part of GFP lifecycle when unload happens */
	virtual void OnUnregister() override;

	/** Starting point once whole module is ready(loaded) to be initialized */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
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
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	void GrantPlayerReviveEffect();

	/** To Remove Revive applied gameplay effect */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	void RemoveAppliedReviveGameplayEffect();

	/*********************************************************************************************
	 * Revive (once per match)
	 **********************************************************************************************/
protected:
	/** Is set once this player was revived back from a ghost to the regular player character.
	 * Is replicated, so clients know as well that this player already used their only revive of the current match.
	 * Is reset on each match start (game state changed to InGame) and on GFP unload. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Replicated, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected, DisplayName = "Is Revived"))
	bool bIsRevived = false;

public:
	/** Returns TRUE if this player was not revived yet in the current match, so they are still allowed to become a ghost */
	UFUNCTION(BlueprintPure, Category = "[GhostRevengeSystem]")
	bool IsRevivable() const;

	/** Marks this player as revived, so they can't become a ghost again until the match restarts. Is applied on server only as the state is replicated to clients. */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	void SetRevived();

	/** Resets the revive state, so this player can become a ghost again */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	void ResetRevived();

	/*********************************************************************************************
	 * Bomb spawning ability that automatically explodes after a certain time
	 **********************************************************************************************/
protected:
	/** Cached handle of applied explosion (bomb spawning) effect */
	FActiveGameplayEffectHandle AppliedBombSpawnEffectHandle;

public:
	/** Returns handle of current applied ability effect  */
	FORCEINLINE const FActiveGameplayEffectHandle& GetAppliedBombSpawningEffectHandle() const { return AppliedBombSpawnEffectHandle; }

	/** To apply explosion (bomb spawning) gameplay effect */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	void ApplyBombSpawningGameplayEffect();

	/** To Remove applied explosion (bomb spawning) gameplay effect */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	void RemoveBombSpawningGameplayEffect();
};
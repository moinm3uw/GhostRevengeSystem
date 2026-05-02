// Copyright (c) Yevhenii Selivanov

#pragma once

#include "AbilitySystemInterface.h"
#include "ActiveGameplayEffectHandle.h"
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"

#include "GrsPlayerStateComponent.generated.h"

/**
 * The component is attached to BmrPlayerState primarily to take of the GAS abilities
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GHOSTREVENGESYSTEMRUNTIME_API UGrsPlayerStateComponent : public UActorComponent,
                                                               public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	/** Sets default values for this component's properties */
	UGrsPlayerStateComponent();

	/** Returns the player state from attached BmrPlayerState component */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	class ABmrPlayerState* GetCurrentPlayerState() const;
	class ABmrPlayerState* GetCurrentPlayerStateChecked() const;

	/** Checks if player state has authority */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	bool HasAuthority();

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

	/** Returns the Ability System Component from the Player State.
	 * In blueprints, call 'Get Ability System Component' as interface function. */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	/*********************************************************************************************
	 * Revive ability
	 **********************************************************************************************/
public:
	/** Apply review ability that will restore regular player character
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
// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#pragma once

// PoolManager
#include "Data/PoolObjectHandle.h"

// UE
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"

#include "GrsProjectilePoolComponent.generated.h"

/**
 *  Attached to the BmrGameState to prepare bomb projectiles in the pool once GRS is ready (match is started), so throws take ready ones without spawning.
 *  Works on server only: projectiles are replicated actors, clients receive them from the server.
 *  Projectiles are taken from the pool on each throw by UGrsThrowBombAbility and return themselves back on landing.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GHOSTREVENGESYSTEMRUNTIME_API UGrsProjectilePoolComponent : public UActorComponent
{
	GENERATED_BODY()

	/*********************************************************************************************
	 * Lifecycle
	 **********************************************************************************************/

public:
	/** Sets default values for this component's properties */
	UGrsProjectilePoolComponent();

protected:
	/** Called when the game starts */
	virtual void BeginPlay() override;

	/** Clears all transient data created by this component. */
	virtual void OnUnregister() override;

	/*********************************************************************************************
	 * Projectiles Pool
	 **********************************************************************************************/
protected:
	/** Array of pool actors handlers of projectiles that are being prepared */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Transient, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected, DisplayName = "Pool Projectiles Actors Handlers"))
	TArray<FPoolObjectHandle> ProjectilePoolActorHandlersInternal;

	/** Is set once projectiles are prepared in the pool, the pool survives between matches, so it's prepared only once */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	bool bIsProjectilePoolPrepared = false;

	/** Starting point once whole module is ready(loaded) to be initialized, prepares projectiles in the pool on server */
	UFUNCTION(BlueprintNativeEvent, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void OnInitialize(const struct FGameplayEventData& Payload);

	/** Puts prepared projectiles back to the pool, so throws take ready ones without spawning (Object pooling patter)
	 * @param CreatedObjects - Handles of objects from Pool Manager
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "[GhostRevengeSystem]")
	void OnTakeProjectilesFromPoolCompleted(const TArray<struct FPoolObjectData>& CreatedObjects);
};

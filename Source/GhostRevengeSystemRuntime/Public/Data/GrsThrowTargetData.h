// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#pragma once

// UE
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Engine/NetSerialization.h"

#include "GrsThrowTargetData.generated.h"

/**
 * Launch data of the trajectory the ghost saw in the charge preview, is sent from the ghost's client with the throw event,
 * so the thrown bomb flies along the same arc on every machine.
 * Is quantized to keep the throw cheap for network.
 */
USTRUCT()
struct GHOSTREVENGESYSTEMRUNTIME_API FGrsThrowTargetData : public FGameplayAbilityTargetData
{
	GENERATED_BODY()

	/** World location the preview trajectory starts from */
	UPROPERTY()
	FVector_NetQuantize Start = FVector::ZeroVector;

	/** Velocity the preview trajectory was predicted with */
	UPROPERTY()
	FVector_NetQuantize10 LaunchVelocity = FVector::ZeroVector;

	/** Time the preview trajectory takes till its end, where the aiming area is shown */
	UPROPERTY()
	float FlightTime = 0.f;

	/** Returns the script struct of this target data, is required by GAS to replicate derived target data */
	virtual UScriptStruct* GetScriptStruct() const override { return StaticStruct(); }

	/** Serializes this target data for network with quantized vectors */
	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);
};

template <>
struct TStructOpsTypeTraits<FGrsThrowTargetData> : public TStructOpsTypeTraitsBase2<FGrsThrowTargetData>
{
	enum
	{
		WithNetSerializer = true // Is required for FGameplayAbilityTargetDataHandle net serialization to work
	};
};

// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

// Grs
#include "Data/GrsThrowTargetData.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GrsThrowTargetData)

// Serializes this target data for network with quantized vectors
bool FGrsThrowTargetData::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	Start.NetSerialize(Ar, Map, bOutSuccess);
	LaunchVelocity.NetSerialize(Ar, Map, bOutSuccess);
	Ar << FlightTime;

	bOutSuccess = true;
	return true;
}

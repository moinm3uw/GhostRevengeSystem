// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

// Grs
#include "Data/GRSDataAsset.h"

//  DataAssetsLoader
#include "DalSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GRSDataAsset)

const UGRSDataAsset& UGRSDataAsset::Get()
{
	return UDalSubsystem::GetDataAssetChecked<ThisClass>();
}

// Returns how long the thrown bomb flies along given arc with the projectile flight speed, or 0 if there is no arc
float UGRSDataAsset::GetProjectileFlightTime(const FPredictProjectilePathResult& PredictResult) const
{
	return PredictResult.PathData.IsEmpty() ? 0.f : PredictResult.PathData.Last().Time / ProjectileFlightSpeed;
}

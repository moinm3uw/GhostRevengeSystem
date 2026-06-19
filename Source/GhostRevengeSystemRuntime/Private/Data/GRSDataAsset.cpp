// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#include "Data/GRSDataAsset.h"

//  DataAssetsLoader
#include "DalSubsystem.h"

// @PR JanSeliv [Coding Standards] - uncomment, reflection cpp requires active UE_INLINE_GENERATED_CPP_BY_NAME after includes, no commented-out code
// #include UE_INLINE_GENERATED_CPP_BY_NAME(GRSDataAsset)

const UGRSDataAsset& UGRSDataAsset::Get()
{
	return UDalSubsystem::GetDataAssetChecked<ThisClass>();
}

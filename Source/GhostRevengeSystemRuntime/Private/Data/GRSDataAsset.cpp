// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#include "Data/GRSDataAsset.h"

//  DataAssetsLoader
#include "DalSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GRSDataAsset)

const UGRSDataAsset& UGRSDataAsset::Get()
{
	return UDalSubsystem::GetDataAssetChecked<ThisClass>();
}

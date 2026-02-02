// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#include "Data/GRSDataAsset.h"

#include "SubSystems/GRSWorldSubSystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GRSDataAsset)

const UGRSDataAsset& UGRSDataAsset::Get()
{
	const UGRSDataAsset* DataAsset = UGRSWorldSubSystem::Get().GetGRSDataAsset();
	ensureMsgf(DataAsset, TEXT("ASSERT: [%i] %hs:\n'DataAsset' is null!"), __LINE__, __FUNCTION__);
	return *DataAsset;
}

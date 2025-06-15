// Copyright (c) Yevhenii Selivanov


#include "GhostRevengeUtils.h"

#include "SubSystems/GRSWorldSubSystem.h"

AGRSPlayerCharacter* UGhostRevengeUtils::GetGhostPlayerCharacter(const UObject* OptionalWorldContext)
{
	return UGRSWorldSubSystem::Get().GetGhostPlayerCharacter();
}

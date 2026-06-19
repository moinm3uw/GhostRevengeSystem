// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#pragma once

// @PR JanSeliv [Coding Standards] - Bomber.h unused by this enum, drop heavy include. UENUM needs own EGRSSpotType.generated.h instead
// Bmr
#include "Bomber.h"

// @PR JanSeliv [Coding Standards] - EGRSSpotType has zero consumers across module, dead code, remove enum
/**
 * Represents type of the spot on the level
 */
UENUM(BlueprintType, DisplayName = "Level Spot Type")
enum class EGRSSpotType : uint8
{
	///< no type to be applied
	None,
	///< Left side of the map
	Left,
	///< Right side of the map
	Right,
};

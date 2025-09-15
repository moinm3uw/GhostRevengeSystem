// // Copyright (c) Valerii Rotermel

#pragma once

#include "Bomber.h"

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

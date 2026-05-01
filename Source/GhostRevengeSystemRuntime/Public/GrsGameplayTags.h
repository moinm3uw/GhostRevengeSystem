// Copyright (c) Yevhenii Selivanov.

#pragma once

// UE
#include "NativeGameplayTags.h" // UE_DECLARE_GAMEPLAY_TAG_EXTERN

namespace GrsGameplayTags
{

	namespace Event
	{
		/** Event that fires when MGF(GFP) is loaded and/or ready */
		GHOSTREVENGESYSTEMRUNTIME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameFeaturePluginReady);
	} // namespace Event
} // namespace GrsGameplayTags

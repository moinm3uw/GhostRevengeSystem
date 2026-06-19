// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#include "SubSystems/GRSWorldSubSystem.h"

// GRS
// @PR JanSeliv [Coding Standards] - .cpp uses UGrsCollisionComponent and UGrsCharacterManagerComponent (header forward-declares only), include "Components/GrsCollisionComponent.h" and "Components/GrsCharacterManagerComponent.h" like GrsPawnComponent.h, never rely on transitive/unity
#include "Components/GrsPawnComponent.h"
#include "GrsGameplayTags.h"
#include "LevelActors/GrsPawn.h"

// Bmr
#include "Actors/BmrPawn.h"
#include "GameFramework/BmrGameState.h"
#include "GameFramework/BmrPlayerState.h"
#include "Structures/BmrGameStateTag.h"
#include "Structures/BmrGameplayTags.h"
#include "UI/Widgets/BmrHUDWidget.h"
#include "UtilityLibraries/BmrBlueprintFunctionLibrary.h"

// MyEditorUtils
#include "MyUtilsLibraries/UtilsLibrary.h"
#include "Subsystems/GlobalMessageSubsystem.h"

// UE
#include "Abilities/GameplayAbilityTypes.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "GhostRevengeSystemRuntimeModule.h"
// @PR JanSeliv [Coding Standards] - unused include, no UGameplayStatics symbol referenced in file, remove it
#include "Kismet/GameplayStatics.h"

// @PR JanSeliv [Coding Standards] - reflection .cpp must have active UE_INLINE_GENERATED_CPP_BY_NAME after includes, uncomment it, drop commented-out form
// #include UE_INLINE_GENERATED_CPP_BY_NAME(GRSWorldSubSystem)

/*********************************************************************************************
 * Subsystem's Lifecycle
 **********************************************************************************************/

// Returns this Subsystem, is checked and will crash if it can't be obtained
UGRSWorldSubSystem& UGRSWorldSubSystem::Get()
{
	const UWorld* World = UUtilsLibrary::GetPlayWorld();
	// @PR JanSeliv [Coding Standards] - use %hs with __FUNCTION__, not %s with *FString(). Applies to ThisSubsystem checkf below
	checkf(World, TEXT("%s: 'World' is null"), *FString(__FUNCTION__));
	UGRSWorldSubSystem* ThisSubsystem = World->GetSubsystem<ThisClass>();
	checkf(ThisSubsystem, TEXT("%s: 'GRSWorldSubSystem' is null"), *FString(__FUNCTION__));
	return *ThisSubsystem;
}

// Subscribes to local pawn ready event
void UGRSWorldSubSystem::OnGameFeatureInitialize_Implementation()
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: "), __LINE__, __FUNCTION__);
	UGlobalMessageSubsystem::CallOrStartListeningForGlobalMessage(BmrGameplayTags::Event::Player_LocalPawnReady, this, &ThisClass::OnLocalPawnReady);
}

// Called when the local player character is spawned, possessed, and replicated
void UGRSWorldSubSystem::OnLocalPawnReady_Implementation(const FGameplayEventData& Payload)
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: "), __LINE__, __FUNCTION__);

	const APawn* Pawn = Cast<APawn>(Payload.Instigator.Get());
	ABmrPlayerState* PlayerState = Pawn ? Pawn->GetPlayerState<ABmrPlayerState>() : nullptr;
	checkf(PlayerState, TEXT("ERROR: [%i] %hs:\n'PlayerState' is null!"), __LINE__, __FUNCTION__);
	// @PR JanSeliv [Coding Standards] - AddUniqueDynamic with no matching RemoveDynamic in PerformCleanUp, every Add listener needs paired Remove on cleanup
	PlayerState->OnEndGameStateChanged.AddUniqueDynamic(this, &ThisClass::OnEndGameStateChanged);

	UGlobalMessageSubsystem::CallOrStartListeningForGlobalMessage(BmrGameplayTags::Event::GameState_Changed, this, &ThisClass::OnGameStateChanged);
}

// Checks if all components present and invokes initialization
void UGRSWorldSubSystem::TryInit()
{
	// --- check if managers have characters and collisions spawned if not - broadcast, yes -> return
	if (IsReady())
	{
		FGameplayEventData EventData;
		EventData.EventTag = GrsGameplayTags::Event::GameFeaturePluginReady;
		UGlobalMessageSubsystem::BroadcastGlobalMessage(EventData);
	}
}

// Checks if the system is ready to load
bool UGRSWorldSubSystem::IsReady()
{
	// @PR JanSeliv [Coding Standards] - extract magic 4 to shared constexpr max players, same literal hardcoded at PawnComponents.Num() < 4 in RegisterPawnComponent
	// todo: obtain max player param from Bmr core
	/* @PR JanSeliv [Potential Bug] - readiness gates on PawnComponents.Num() == MaxPlayers with MaxPlayers literal 4 and RegisterPawnComponent hardcodes ensure(Num() < 4), so any non-4 match (fewer players, spectators, late-join) never broadcasts GameFeaturePluginReady and nothing initializes.
	 * Replace literal 4 with UBmrBlueprintFunctionLibrary::GetAlivePlayersNum(EBmrPlayerType::Any) (counts BmrPawn players human and bots, no spectators), single TryInit trigger */
	int32 MaxPlayers = 4;

	const ABmrGameState& GameState = ABmrGameState::Get();
	// @PR JanSeliv [Coding Standards] - bool must start with b and CamelCase, rename isReady to bIsReady
	bool isReady = CharacterManagerComponent
	               && CollisionMangerComponent
	               && PawnComponents.Num() == MaxPlayers
	               && GameState.HasMatchingGameplayTag(FBmrGameStateTag::InGame);
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: %s "), __LINE__, __FUNCTION__, isReady ? TEXT("READY") : TEXT("NOT READY"));
	return isReady;
}

// Clears all transient data created by this subsystem
void UGRSWorldSubSystem::OnGameFeatureDeinitialize_Implementation()
{
	PerformCleanUp();
}

// Cleanup used on unloading module to remove properties that should not be available by other objects.
void UGRSWorldSubSystem::PerformCleanUp()
{
	UGlobalMessageSubsystem::StopListeningForAllGlobalMessages(this);

	// Clear cached GameFeaturePluginReady so late-binding listeners receive fresh data on GRS load
	UGlobalMessageSubsystem::ClearCachedMessages(GrsGameplayTags::Event::GameFeaturePluginReady);

	UnregisterCharacterManagerComponent();
	UnregisterCollisionManagerComponent();
	ClearGhostCharacters();
	ClearCollisions();

	UBmrHUDWidget* BmrHUD = UBmrBlueprintFunctionLibrary::GetHUDWidget(this);
	if (BmrHUD)
	{
		BmrHUD->SetVisibility(ESlateVisibility::Visible);
	}
}

/*********************************************************************************************
 * Side Collisions actors
 **********************************************************************************************/

// Register collision manager component used to track if all components loaded and GFP ready to initialize
void UGRSWorldSubSystem::RegisterCollisionManagerComponent(UGrsCollisionComponent* NewCollisionManagerComponent)
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: "), __LINE__, __FUNCTION__);
	if (!ensureMsgf(NewCollisionManagerComponent != CollisionMangerComponent, TEXT("ASSERT: [%i] %hs:\n'CollisionMangerComponent' is being overriden twice!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	// @PR JanSeliv [Coding Standards] - `!= CollisionMangerComponent` already guaranteed by ensureMsgf above, drop redundant clause, keep null-check `if (NewCollisionManagerComponent)`. Applies across file: RegisterCharacterManagerComponent
	if (NewCollisionManagerComponent && NewCollisionManagerComponent != CollisionMangerComponent)
	{
		CollisionMangerComponent = NewCollisionManagerComponent;
	}

	TryInit(); // try to initialize
}

// Add spawned collision actors to be cached
void UGRSWorldSubSystem::AddCollisionActor(AActor* Actor)
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: "), __LINE__, __FUNCTION__);

	if (!Actor)
	{
		return;
	}

	if (!LeftSideCollision)
	{
		LeftSideCollision = Actor;
	}
	else if (!RightSideCollision)
	{
		RightSideCollision = Actor;
	}
}

// Returns TRUE if collision are spawned
bool UGRSWorldSubSystem::IsCollisionsSpawned()
{
	// @PR JanSeliv [Coding Standards] - if only assigns bool literal, collapse to const bool bIsSpawned = LeftSideCollision && RightSideCollision. Applies across file: IsRevivable below
	bool bIsSpawned = false;

	if (LeftSideCollision && RightSideCollision)
	{
		bIsSpawned = true;
	}

	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: %s "), __LINE__, __FUNCTION__, bIsSpawned ? TEXT("TRUE") : TEXT("FALSE"));
	return bIsSpawned;
}

// Clears cached collision manager component
void UGRSWorldSubSystem::UnregisterCollisionManagerComponent()
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: "), __LINE__, __FUNCTION__);
	CollisionMangerComponent = nullptr;
}

// Clear cached collisions
void UGRSWorldSubSystem::ClearCollisions()
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: "), __LINE__, __FUNCTION__);
	if (LeftSideCollision)
	{
		LeftSideCollision->Destroy();
	}

	if (RightSideCollision)
	{
		RightSideCollision->Destroy();
	}

	LeftSideCollision = nullptr;
	RightSideCollision = nullptr;
}

// Checks if the target Player was already revived. Player can be revived only once
bool UGRSWorldSubSystem::IsRevivable(const ABmrPawn* PlayerToRevive)
{
	bool bIsRevivable = true;

	if (!PlayerToRevive || RevivedPlayerCharacters.Contains(PlayerToRevive))
	{
		bIsRevivable = false;
	}

	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: %s "), __LINE__, __FUNCTION__, bIsRevivable ? TEXT("Revivable") : TEXT("NOT Revivable"));
	return bIsRevivable;
}

// Set a player character as it was revived once
void UGRSWorldSubSystem::SetRevivedPlayer(ABmrPawn* PlayerToRevive)
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: "), __LINE__, __FUNCTION__);

	if (!PlayerToRevive)
	{
		return;
	}
	RevivedPlayerCharacters.AddUnique(PlayerToRevive);
}

// Reset revived players so they can be ghosts again
void UGRSWorldSubSystem::ResetRevivedPlayers()
{
	RevivedPlayerCharacters.Empty();
}

/*********************************************************************************************
 * Ghost Characters
 **********************************************************************************************/

// Register character manager component
void UGRSWorldSubSystem::RegisterCharacterManagerComponent(UGrsCharacterManagerComponent* NewCharacterManagerComponent)
{
	if (!ensureMsgf(NewCharacterManagerComponent != CharacterManagerComponent, TEXT("ASSERT: [%i] %hs:\n'CharacterManagerComponent' is being overriden twice!"), __LINE__, __FUNCTION__))
	{
		// @PR JanSeliv [Coding Standards] - redundant UE_LOG, ensureMsgf already surfaces same message, drop it like RegisterCollisionManagerComponent
		UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs:\n'CharacterManagerComponent' is being overriden twice!"), __LINE__, __FUNCTION__);
		return;
	}

	if (NewCharacterManagerComponent && NewCharacterManagerComponent != CharacterManagerComponent)
	{
		CharacterManagerComponent = NewCharacterManagerComponent;
	}

	TryInit();
}

/*********************************************************************************************
 * Pawn Component
 **********************************************************************************************/

// Register ghost character
EGRSCharacterSide UGRSWorldSubSystem::RegisterGhostCharacter(AGrsPawn* GhostPlayerCharacter)
{
	// @PR JanSeliv [Coding Standards] - missing terminating `;` after UE_LOG, every other call site ends with `;`
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: "), __LINE__, __FUNCTION__)
	checkf(GhostPlayerCharacter, TEXT("ERROR: [%i] %hs:\n'GhostPlayerCharacter' is null!"), __LINE__, __FUNCTION__);

	if (!GhostCharacterLeftSide)
	{
		GhostCharacterLeftSide = GhostPlayerCharacter;
		return EGRSCharacterSide::Left;
	}

	if (!GhostCharacterRightSide)
	{
		GhostCharacterRightSide = GhostPlayerCharacter;
		return EGRSCharacterSide::Right;
	}

	return EGRSCharacterSide::None;
}

// Register a new Pawn component to track the pawn state
// @PR JanSeliv [Coding Standards] - drop `class` elaborated specifier in .cpp, GrsPawnComponent.h already included, use plain UGrsPawnComponent*. Applies across file: UnRegisterPawnComponent below
void UGRSWorldSubSystem::RegisterPawnComponent(class UGrsPawnComponent* NewPawnComponent)
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs:\n'"), __LINE__, __FUNCTION__);
	if (!NewPawnComponent)
	{
		return;
	}

	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: %s %i"), __LINE__, __FUNCTION__, NewPawnComponent->GetOwner()->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"), PawnComponents.Num());

	if (!ensureMsgf(PawnComponents.Num() < 4, TEXT("ASSERT: [%i] %hs:\n'PawnComponents' is more than expected!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	PawnComponents.AddUnique(NewPawnComponent);
	TryInit();
}

// Clears the registered pawn component once it deleted
void UGRSWorldSubSystem::UnRegisterPawnComponent(class UGrsPawnComponent* PawnComponentToUnregister)
{
	// @PR JanSeliv [Coding Standards] - Contains then Remove double lookup, Remove already no-op on absent element, drop Contains and null-guard only
	if (!PawnComponentToUnregister || PawnComponents.IsEmpty() || !PawnComponents.Contains(PawnComponentToUnregister))
	{
		return;
	}

	PawnComponents.Remove(PawnComponentToUnregister);
}

// Clears cached character manager component
void UGRSWorldSubSystem::UnregisterCharacterManagerComponent()
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: "), __LINE__, __FUNCTION__);
	CharacterManagerComponent = nullptr;
}

// Clear cached ghost character by reference
void UGRSWorldSubSystem::UnregisterGhostCharacter(AGrsPawn* GhostPlayerCharacter)
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: "), __LINE__, __FUNCTION__);

	if (!GhostPlayerCharacter)
	{
		return;
	}

	if (GhostCharacterLeftSide == GhostPlayerCharacter)
	{
		GhostCharacterLeftSide = nullptr;

		return;
	}

	if (GhostCharacterRightSide == GhostPlayerCharacter)
	{
		GhostCharacterRightSide = nullptr;
	}
}

// Clear cached ghost character references
void UGRSWorldSubSystem::ClearGhostCharacters()
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: "), __LINE__, __FUNCTION__);

	/* @PR JanSeliv [Architecture] - ghost AGrsPawns taken from pool by GrsPawnComponent but Destroy() directly here while pawn also returns to pool in PerformCleanUp, two disposal owners for one pooled actor (stale handle, pool churn). ClearCollisions has same bug.
	 * Subsystem only nulls cached refs, never Destroy() pooled actor which it does not own, Destroy() must happen in owner who initially spawned it. */
	if (GhostCharacterLeftSide)
	{
		GhostCharacterLeftSide->Destroy();
		GhostCharacterLeftSide = nullptr;
	}

	if (GhostCharacterRightSide)
	{
		GhostCharacterRightSide->Destroy();
		GhostCharacterRightSide = nullptr;
	}
}

//  Changes the Bmr HUD visibility
void UGRSWorldSubSystem::ChangeHUDEndResultVisibility(bool bVisibility)
{
	UBmrHUDWidget* BmrHUD = UBmrBlueprintFunctionLibrary::GetHUDWidget(this);
	if (!ensureMsgf(BmrHUD, TEXT("ASSERT: [%i] %hs:\n'BmrHUD' is not valid!"), __LINE__, __FUNCTION__))
	{
		return;
	}
	UTextBlock* ResultTextBlock = nullptr;
	// @PR JanSeliv [Coding Standards] - compile-time name, extract to static const FName, drop redundant FName() wrap
	FName ResultTextBlockName = FName(TEXT("RESULT"));

	/* @PR JanSeliv [Architecture] - Wrap entire hack as separate function, marked as @TODO for JanSeliv. */
	TArray<UWidget*> AllWidgets;
	BmrHUD->WidgetTree->GetAllWidgets(AllWidgets);

	// @PR JanSeliv [Coding Standards] - Widget only read, make const-pointee `const UWidget*` like neighbor loop in GrsPlayerControllerComponent
	for (UWidget* Widget : AllWidgets)
	{
		if (UTextBlock* TextBlock = Cast<UTextBlock>(Widget))
		{
			if (TextBlock->GetName() == ResultTextBlockName)
			{
				ResultTextBlock = TextBlock;
			}
		}
	}

	if (!ensureMsgf(ResultTextBlock, TEXT("ASSERT: [%i] %hs:\n'ResultTextBlock' with name %s is not found in the BmrHUD !"), __LINE__, __FUNCTION__, *ResultTextBlockName.ToString()))
	{
		return;
	}

	// @PR JanSeliv [Coding Standards] - local named same as type ESlateVisibility shadows enum, rename to NewVisibility
	ESlateVisibility ESlateVisibility = bVisibility ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
	ResultTextBlock->SetVisibility(ESlateVisibility);
}

// Listen end game states to show/hide HUD temporarry
void UGRSWorldSubSystem::OnEndGameStateChanged_Implementation(EBmrEndGameState EndGameState)
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: "), __LINE__, __FUNCTION__);

	if (EndGameState == EBmrEndGameState::Lose || EndGameState == EBmrEndGameState::HonorLoss)
	{
		bool bShowHUDEndResult = false;
		ChangeHUDEndResultVisibility(bShowHUDEndResult);
	}
}

/*********************************************************************************************
 * Treasury (temp)
 **********************************************************************************************/

// Listen game states to switch character skin.
void UGRSWorldSubSystem::OnGameStateChanged_Implementation(const FGameplayEventData& Payload)
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: "), __LINE__, __FUNCTION__);

	// @PR JanSeliv [Coding Standards] - HasTag(InGame) retrieved twice, cache to local bool and use if\else
	if (Payload.InstigatorTags.HasTag(FBmrGameStateTag::InGame))
	{
		TryInit();
		ResetRevivedPlayers();
	}

	if (!Payload.InstigatorTags.HasTag(FBmrGameStateTag::InGame))
	{
		bool bShowHUDEndResult = true;
		ChangeHUDEndResultVisibility(bShowHUDEndResult);
	}
}

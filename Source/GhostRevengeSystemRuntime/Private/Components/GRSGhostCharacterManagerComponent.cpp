// Copyright (c) Yevhenii Selivanov

#include "Components/GRSGhostCharacterManagerComponent.h"

#include "Bomber.h"
#include "Components/MySkeletalMeshComponent.h"
#include "Controllers/MyPlayerController.h"
#include "Data/GRSDataAsset.h"
#include "Engine/World.h"
#include "GameFramework/MyGameStateBase.h"
#include "GeneratedMap.h"
#include "Kismet/GameplayStatics.h"
#include "LevelActors/GRSPlayerCharacter.h"
#include "PoolManagerSubsystem.h"
#include "SubSystems/GRSWorldSubSystem.h"
#include "Subsystems/GlobalEventsSubsystem.h"
#include "UtilityLibraries/CellsUtilsLibrary.h"
#include "UtilityLibraries/MyBlueprintFunctionLibrary.h"

// Sets default values for this component's properties
UGRSGhostCharacterManagerComponent::UGRSGhostCharacterManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	SetIsReplicatedByDefault(true);
}

// Called when the game starts
void UGRSGhostCharacterManagerComponent::BeginPlay()
{
	Super::BeginPlay();
	UGRSWorldSubSystem::Get().RegisterCharacterManagerComponent(this);

	if (!GetOwner()->HasAuthority())
	{
		return;
	}

	UGRSWorldSubSystem::Get().OnInitialize.AddUniqueDynamic(this, &ThisClass::OnInitialize);
}

// The component is considered as loaded only when the subsystem is loaded
void UGRSGhostCharacterManagerComponent::OnInitialize()
{
	BIND_ON_GAME_STATE_CHANGED(this, ThisClass::OnGameStateChanged);

	// spawn 2 characters right away
	AddGhostCharacter();
}

// Listen game states to remove ghost character from level
void UGRSGhostCharacterManagerComponent::OnGameStateChanged_Implementation(ECurrentGameState CurrentGameState)
{
	switch (CurrentGameState)
	{
		case ECurrentGameState::GameStarting:
		{
			if (GetOwner()->HasAuthority())
			{
				RegisterForPlayerDeath();
			}
			break;
		}
		default: break;
	}
}

void UGRSGhostCharacterManagerComponent::RegisterForPlayerDeath()
{
	if (!GetOwner()->HasAuthority())
	{
		return;
	}

	TArray<AActor*> PlayerCharactersInternal;

	// -- subscribe to PlayerCharacters death event in order to see if a ghost player killed somebody
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerCharacter::StaticClass(), PlayerCharactersInternal);

	for (AActor* Actor : PlayerCharactersInternal)
	{
		APlayerCharacter* MyActor = Cast<APlayerCharacter>(Actor);
		if (MyActor)
		{
			if (MyActor->IsBotControlled())
			{
				continue;
			}

			UMapComponent* MapComponent = UMapComponent::GetMapComponent(MyActor);
			if (MapComponent)
			{
				MapComponent->OnPreRemovedFromLevel.AddUniqueDynamic(this, &ThisClass::OnPreRemovedFromLevel);
			}
		}
	}
}

// Called right before owner actor going to remove from the Generated Map, on both server and clients
void UGRSGhostCharacterManagerComponent::OnPreRemovedFromLevel_Implementation(class UMapComponent* MapComponent, class UObject* DestroyCauser)
{
	APlayerCharacter* PlayerCharacter = MapComponent->GetOwner<APlayerCharacter>();
	if (!ensureMsgf(PlayerCharacter, TEXT("ASSERT: [%i] %hs:\n'PlayerCharacter' is not valid!"), __LINE__, __FUNCTION__)
	    || PlayerCharacter->IsBotControlled())
	{
		return;
	}

	UGRSWorldSubSystem::Get().ActivateGhostCharacter(PlayerCharacter);
}

// Add ghost character to the current active game (on level map)
void UGRSGhostCharacterManagerComponent::AddGhostCharacter()
{
	// --- Return to Pool Manager the list of handles which is not needed (if there are any)
	if (!PoolActorHandlersInternal.IsEmpty())
	{
		UPoolManagerSubsystem::Get().ReturnToPoolArray(PoolActorHandlersInternal);
		PoolActorHandlersInternal.Empty();
	}

	// --- Prepare spawn request
	const TWeakObjectPtr<ThisClass> WeakThis = this;
	const FOnSpawnAllCallback OnTakeActorsFromPoolCompleted = [WeakThis](const TArray<FPoolObjectData>& CreatedObjects)
	{
		if (UGRSGhostCharacterManagerComponent* This = WeakThis.Get())
		{
			This->OnTakeActorsFromPoolCompleted(CreatedObjects);
		}
	};

	// --- Spawn actor
	UPoolManagerSubsystem::Get().TakeFromPoolArray(PoolActorHandlersInternal, AGRSPlayerCharacter::StaticClass(), 2, OnTakeActorsFromPoolCompleted, ESpawnRequestPriority::High);
}

// Grabs a Ghost Revenge Player Character from the pool manager (Object pooling patter)
void UGRSGhostCharacterManagerComponent::OnTakeActorsFromPoolCompleted(const TArray<FPoolObjectData>& CreatedObjects)
{
	// --- something wrong if there are less than 1 object found
	if (CreatedObjects.Num() < 1)
	{
		return;
	}

	// --- Setup spawned characters
	for (const FPoolObjectData& CreatedObject : CreatedObjects)
	{
		AGRSPlayerCharacter& GhostCharacter = CreatedObject.GetChecked<AGRSPlayerCharacter>();
		UE_LOG(LogTemp, Warning, TEXT("Spawned ghost character --- %s - %s"), *GhostCharacter.GetName(), GhostCharacter.HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"));

		// we can path a current local player since it needed only for the skin init
		GhostCharacter.OnGhostEliminatesPlayer.AddUniqueDynamic(this, &ThisClass::OnGhostEliminatesPlayer);
		// GhostCharacter.OnGhostRemovedFromLevel.AddUniqueDynamic(this, &ThisClass::OnGhostRemovedFromLevel);
	}
}

// Called when the ghost player kills another player and will be swaped with him
void UGRSGhostCharacterManagerComponent::OnGhostEliminatesPlayer(FVector AtLocation, class AGRSPlayerCharacter* GhostCharacter)
{
	AddPlayerCharacter(AtLocation, GhostCharacter);
}

// Called when the ghost character should be removed from level to unpossess controller
void UGRSGhostCharacterManagerComponent::OnGhostRemovedFromLevel(AGRSPlayerCharacter* GhostCharacter)
{
	AddPlayerCharacter(FVector::Zero(), GhostCharacter);
}

// Spawn and possess a regular player character to the level at location
void UGRSGhostCharacterManagerComponent::AddPlayerCharacter(FVector AtLocation, class AGRSPlayerCharacter* GhostCharacter)
{
	if (!GhostCharacter)
	{
		return;
	}

	AController* PlayerController = GhostCharacter->GetController();
	if (!ensureMsgf(PlayerController, TEXT("ASSERT: [%i] %hs:\n'PlayerController' is not valid!"), __LINE__, __FUNCTION__)
	    || !PlayerController->HasAuthority())
	{
		return;
	}

	FVector SpawnLocation = AtLocation;

	FCell CurrentCell = SpawnLocation;
	const FCell SnappedCell = UCellsUtilsLibrary::GetNearestFreeCell(CurrentCell);

	const TFunction<void(UMapComponent&)> OnPlayerSpawned = [WeakThis = TWeakObjectPtr(this), PlayerController, SpawnLocation](const UMapComponent& MapComponent)
	{
		UGRSGhostCharacterManagerComponent* This = WeakThis.Get();
		if (!This || !PlayerController || !PlayerController->HasAuthority())
		{
			return;
		}

		APlayerCharacter* PlayerCharacter = MapComponent.GetOwner<APlayerCharacter>();
		if (!ensureMsgf(PlayerCharacter, TEXT("ASSERT: [%i] %hs:\n'PlayerCharacter' is not valid!"), __LINE__, __FUNCTION__))
		{
			return;
		}

		PlayerCharacter->GetMeshComponentChecked().SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
		if (PlayerController->GetPawn())
		{
			// At first, unpossess previous controller
			PlayerController->UnPossess();
		}

		PlayerController->Possess(PlayerCharacter);
	};
	AGeneratedMap::Get().SpawnActorByType(EActorType::Player, SnappedCell, OnPlayerSpawned);
}

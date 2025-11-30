// Copyright (c) Yevhenii Selivanov

#include "Components/GRSGhostCharacterManagerComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Actors/BmrGeneratedMap.h"
#include "Bomber.h"
#include "Components/BmrSkeletalMeshComponent.h"
#include "Controllers/BmrPlayerController.h"
#include "Data/GRSDataAsset.h"
#include "Engine/World.h"
#include "GameFramework/BmrGameState.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "LevelActors/GRSPlayerCharacter.h"
#include "PoolManagerSubsystem.h"
#include "SubSystems/GRSWorldSubSystem.h"
#include "Subsystems/BmrGlobalEventsSubsystem.h"
#include "UtilityLibraries/BmrBlueprintFunctionLibrary.h"
#include "UtilityLibraries/BmrCellUtilsLibrary.h"

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

	if (!GetOwner()->HasAuthority())
	{
		return;
	}

	UGRSWorldSubSystem& WorldSubsystem = UGRSWorldSubSystem::Get();
	WorldSubsystem.OnInitialize.AddUniqueDynamic(this, &ThisClass::OnInitialize);
	WorldSubsystem.RegisterCharacterManagerComponent(this);
}

// The component is considered as loaded only when the subsystem is loaded
void UGRSGhostCharacterManagerComponent::OnInitialize()
{
	UE_LOG(LogTemp, Log, TEXT("UGRSGhostCharacterManagerComponent OnInitialize  --- %s"), *this->GetName());

	// spawn 2 characters right away
	AddGhostCharacter();

	if (GetOwner()->HasAuthority())
	{
		RegisterForPlayerDeath();
	}

	// --- bind to  clear ghost data
	BIND_ON_GAME_STATE_CHANGED(this, ThisClass::OnGameStateChanged);
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
		UE_LOG(LogTemp, Log, TEXT("Spawned ghost character --- %s - %s"), *GhostCharacter.GetName(), GhostCharacter.HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"));

		// we can path a current local player since it needed only for the skin init
		GhostCharacter.OnGhostEliminatesPlayer.AddUniqueDynamic(this, &ThisClass::OnGhostEliminatesPlayer);
		GhostCharacter.OnGhostRemovedFromLevel.AddUniqueDynamic(this, &ThisClass::OnGhostRemovedFromLevel);
	}
}

// Clears all transient data created by this component.
void UGRSGhostCharacterManagerComponent::OnUnregister()
{
	Super::OnUnregister();

	if (PoolActorHandlersInternal.Num() > 0)
	{
		UPoolManagerSubsystem::Get().ReturnToPoolArray(PoolActorHandlersInternal);
		PoolActorHandlersInternal.Empty();
		UPoolManagerSubsystem::Get().EmptyPool(AGRSPlayerCharacter::StaticClass());
	}

	if (DeadPlayerCharacters.Num() > 0)
	{
		DeadPlayerCharacters.Empty();
	}

	// --- clean delegates
	if (BoundMapComponents.Num() > 0)
	{
		for (int32 Index = 0; Index < BoundMapComponents.Num(); Index++)
		{
			BoundMapComponents[0]->OnPreRemovedFromLevel.RemoveDynamic(this, &ThisClass::PlayerCharacterOnPreRemovedFromLevel);
		}

		BoundMapComponents.Empty();
	}
}

// Listen game states to remove ghost character from level
void UGRSGhostCharacterManagerComponent::OnGameStateChanged_Implementation(EBmrCurrentGameState CurrentGameState)
{
	if (CurrentGameState != EBmrCurrentGameState::InGame)
	{
		// --- clean delegates
		if (BoundMapComponents.Num() > 0)
		{
			for (int32 Index = 0; Index < BoundMapComponents.Num(); Index++)
			{
				BoundMapComponents[0]->OnPreRemovedFromLevel.RemoveDynamic(this, &ThisClass::PlayerCharacterOnPreRemovedFromLevel);
			}
		}

		DeadPlayerCharacters.Empty();
	}

	switch (CurrentGameState)
	{
		case EBmrCurrentGameState::GameStarting:

			if (GetOwner()->HasAuthority())
			{
				RegisterForPlayerDeath();
			}
			break;
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
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABmrPawn::StaticClass(), PlayerCharactersInternal);

	for (AActor* Actor : PlayerCharactersInternal)
	{
		const ABmrPawn* MyActor = Cast<ABmrPawn>(Actor);
		if (MyActor)
		{
			if (MyActor->IsBotControlled())
			{
				continue;
			}

			UBmrMapComponent* MapComponent = UBmrMapComponent::GetMapComponent(MyActor);
			if (MapComponent)
			{
				MapComponent->OnPreRemovedFromLevel.AddUniqueDynamic(this, &ThisClass::PlayerCharacterOnPreRemovedFromLevel);

				BoundMapComponents.AddUnique(MapComponent);

				// Actor has ASC: apply effect through GAS
				const ABmrPawn* PlayerCharacter = MapComponent->GetOwner<ABmrPawn>();
				if (PlayerCharacter)
				{
					UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(PlayerCharacter);
					TSubclassOf<UGameplayEffect> PlayerReviveEffect = UGRSDataAsset::Get().GetPlayerReviveEffect();
					if (ensureMsgf(PlayerReviveEffect, TEXT("ASSERT: [%i] %hs:\n'PlayerDeathEffect' is not set!"), __LINE__, __FUNCTION__))
					{
						ASC->ApplyGameplayEffectToSelf(PlayerReviveEffect.GetDefaultObject(), /*Level*/ 1.f, ASC->MakeEffectContext());
					}
				}
			}
		}
	}
}

// Called right before owner actor going to remove from the Generated Map, on both server and clients
void UGRSGhostCharacterManagerComponent::PlayerCharacterOnPreRemovedFromLevel_Implementation(class UBmrMapComponent* MapComponent, class UObject* DestroyCauser)
{
	ABmrPawn* PlayerCharacter = MapComponent->GetOwner<ABmrPawn>();
	if (!ensureMsgf(PlayerCharacter, TEXT("ASSERT: [%i] %hs:\n'PlayerCharacter' is not valid!"), __LINE__, __FUNCTION__)
	    || PlayerCharacter->IsBotControlled()
	    || !DestroyCauser)
	{
		return;
	}

	// --- check if already dead character was present
	for (const TPair<ABmrPawn*, AGRSPlayerCharacter*>& Pair : DeadPlayerCharacters)
	{
		if (Pair.Key == PlayerCharacter)
		{
			if (!Pair.Value)
			{
				return;
			}
		}
	}

	// --- handle 3rd player character death to perform swap
	AGRSPlayerCharacter* GhostCauser = Cast<AGRSPlayerCharacter>(DestroyCauser);
	if (GhostCauser && DeadPlayerCharacters.Num() > 1)
	{
		for (TPair<ABmrPawn*, AGRSPlayerCharacter*>& Pair : DeadPlayerCharacters)
		{
			if (Pair.Value == GhostCauser)
			{
				Pair.Value = nullptr;
				GhostCauser->ActivateCharacter(PlayerCharacter);
				UGRSWorldSubSystem::Get().SetLastActivatedGhostCharacter(GhostCauser);
				DeadPlayerCharacters.Add(PlayerCharacter, GhostCauser);
				return;
			}
		}
	}

	AGRSPlayerCharacter* GhostToActive = UGRSWorldSubSystem::Get().GetAvailableGhostCharacter();
	if (GhostToActive)
	{
		GhostToActive->ActivateCharacter(PlayerCharacter);
		UGRSWorldSubSystem::Get().SetLastActivatedGhostCharacter(GhostToActive);
		DeadPlayerCharacters.Add(PlayerCharacter, GhostToActive);
	}
}

// Called when the ghost player kills another player and will be swaped with him
void UGRSGhostCharacterManagerComponent::OnGhostEliminatesPlayer(FVector AtLocation, class AGRSPlayerCharacter* GhostCharacter)
{
	// RevivePlayerCharacter(GhostCharacter);
}

// Called when the ghost character should be removed from level to unpossess controller
void UGRSGhostCharacterManagerComponent::OnGhostRemovedFromLevel(AController* CurrentController, AGRSPlayerCharacter* GhostCharacter)
{
	if (!CurrentController || !GhostCharacter)
	{
		return;
	}

	RevivePlayerCharacter(CurrentController, GhostCharacter);
}

// Spawn and possess a regular player character to the level at location
void UGRSGhostCharacterManagerComponent::RevivePlayerCharacter(AController* PlayerController, AGRSPlayerCharacter* GhostCharacter)
{
	if (!ensureMsgf(PlayerController, TEXT("ASSERT: [%i] %hs:\n'PlayerController' is not valid!"), __LINE__, __FUNCTION__)
	    || !PlayerController->HasAuthority())
	{
		return;
	}

	if (!GhostCharacter)
	{
		return;
	}
	ABmrPawn* PlayerCharacter = nullptr;

	for (TPair<ABmrPawn*, AGRSPlayerCharacter*>& Pair : DeadPlayerCharacters)
	{
		if (Pair.Value == GhostCharacter)
		{
			PlayerCharacter = Pair.Key;
			Pair.Value = nullptr;
		}
	}

	if (!PlayerCharacter)
	{
		return;
	}

	PlayerCharacter->GetMeshComponentChecked().SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	if (PlayerController->GetPawn())
	{
		// At first, unpossess previous controller
		PlayerController->UnPossess();
	}

	// --- Always possess to player character when ghost character is no longer in control
	PlayerController->Possess(PlayerCharacter);
	UE_LOG(LogTemp, Log, TEXT("[%i] %hs: --- PlayerController is %s"), __LINE__, __FUNCTION__, PlayerController ? TEXT("TRUE") : TEXT("FALSE"));
	UE_LOG(LogTemp, Log, TEXT("[%i] %hs: --- PlayerCharacter is %s"), __LINE__, __FUNCTION__, PlayerCharacter ? TEXT("TRUE") : TEXT("FALSE"));
	UE_LOG(LogTemp, Log, TEXT("[%i] %hs: --- PlayerCharacter: %s"), __LINE__, __FUNCTION__, *GetNameSafe(PlayerCharacter));

	// Activate revive ability if player was NOT revived previously

	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(PlayerCharacter);
	FGameplayEventData EventData;
	EventData.EventMagnitude = UBmrCellUtilsLibrary::GetIndexByCellOnLevel(PlayerCharacter->GetActorLocation());
	ASC->HandleGameplayEvent(UGRSDataAsset::Get().GetReviePlayerCharacterTriggerTag(), &EventData);
}

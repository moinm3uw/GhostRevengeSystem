// Copyright (c) Valerii Rotermel & Yevhenii Selivanov


#include "Components/GRSComponent.h"

#include "Components/MapComponent.h"
#include "Components/MySkeletalMeshComponent.h"
#include "Data/GRSDataAsset.h"
#include "GameFramework/MyPlayerState.h"
#include "LevelActors/PlayerCharacter.h"
#include "Subsystems/GlobalEventsSubsystem.h"
#include "SubSystems/GRSWorldSubSystem.h"
#include "Subsystems/WidgetsSubsystem.h"
#include "UI/Widgets/HUDWidget.h"
#include "UtilityLibraries/MyBlueprintFunctionLibrary.h"

// Sets default values for this component's properties
UGRSComponent::UGRSComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

// Returns the Skeletal Mesh of the Bomber character
UMySkeletalMeshComponent* UGRSComponent::GetMySkeletalMeshComponent() const
{
	return GetOwner()->FindComponentByClass<UMySkeletalMeshComponent>();
}

// Returns the Skeletal Mesh of the Bomber character
UMySkeletalMeshComponent& UGRSComponent::GetMeshChecked() const
{
	UMySkeletalMeshComponent* Mesh = GetMySkeletalMeshComponent();
	ensureMsgf(Mesh, TEXT("ASSERT: [%i] %hs:\n'Mesh' is nullptr, can not get mesh for spot.!"), __LINE__, __FUNCTION__);
	return *Mesh;
}

// Returns current spot name
FName UGRSComponent::GetSpotName_Implementation()
{
	return SpotNameInternal;
}

// Called when a component is registered, after Scene is set, but before CreateRenderState_Concurrent or OnCreatePhysicsState are called
void UGRSComponent::OnRegister()
{
	Super::OnRegister();

	UGRSWorldSubSystem::Get().OnInitialize.AddUniqueDynamic(this, &ThisClass::OnInitialize);
}

// Clears all transient data created by this component
void UGRSComponent::OnUnregister()
{
	Super::OnUnregister();
}

// Called when the ghost revenge system is ready loaded (when game transitions to ingame state)
void UGRSComponent::OnInitialize_Implementation()
{
	SpotNameInternal = UGRSWorldSubSystem::Get().GetSpotName();
	UGRSWorldSubSystem::Get().RegisterSpotComponent(this);
	BIND_ON_LOCAL_CHARACTER_READY(this, UGRSComponent::OnLocalCharacterReady);
	UE_LOG(LogTemp, Warning, TEXT("UGRSComponent OnRegister"));
	UE_LOG(LogTemp, Warning, TEXT("UGRSDataAsset string %s"), *UGRSDataAsset::Get().GetTestString());
}

// Called when the local player character is spawned, possessed, and replicated
void UGRSComponent::OnLocalCharacterReady_Implementation(class APlayerCharacter* PlayerCharacter, int32 CharacterID)
{
	// Binds the local player state ready event to the handler
	BIND_ON_LOCAL_PLAYER_STATE_READY(this, ThisClass::OnLocalPlayerStateReady);
}

// Subscribes to the end game state change notification on the player state.
void UGRSComponent::OnLocalPlayerStateReady_Implementation(class AMyPlayerState* PlayerState, int32 CharacterID)
{
	checkf(PlayerState, TEXT("ERROR: [%i] %hs:\n'PlayerState' is null!"), __LINE__, __FUNCTION__);
	PlayerStateInternal = PlayerState;

	
	APlayerCharacter* PlayerCharacter = PlayerStateInternal->GetPlayerCharacter();
	PlayerCharacterInternal = DuplicateObject<APlayerCharacter>(PlayerCharacter, PlayerCharacter->GetOuter(), TEXT("GRSPlayerCharacter"));
	PlayerCharacterInternal->SetActorLocation(FVector(-800, -800, 100));
	
	MySkeletalMeshComponentInternal = PlayerCharacterInternal->GetMeshChecked();

	/*
	if (UMapComponent* MapComponent = UMapComponent::GetMapComponent(PlayerCharacterInternal))
	{
		MapComponent->SetReplicatedMeshData(MySkeletalMeshComponentInternal->GetMeshData());
	}
	*/

	PlayerState->OnEndGameStateChanged.AddUniqueDynamic(this, &ThisClass::OnEndGameStateChanged);
}

// Called when the end game state was changed to recalculate progression according to endgame (win, loss etc.) 
void UGRSComponent::OnEndGameStateChanged_Implementation(EEndGameState EndGameState)
{
	class UHUDWidget* HUD = nullptr;
	APlayerCharacter* SpawnedActor = nullptr;

	// Spawn parameters
	FActorSpawnParameters SpawnParams;
	
	FVector SpawnLocation = UGRSDataAsset::Get().GetSpawnLocation();
	switch (EndGameState)
	{
	case EEndGameState::Lose:
		
	
		SpawnedActor = GetWorld()->SpawnActor<APlayerCharacter>(PlayerCharacterInternal->GetClass(),SpawnLocation, FRotator::ZeroRotator,SpawnParams);
		/*
		//HUD = UWidgetsSubsystem::Get().GetWidgetByTag();
		if (!ensureMsgf(HUD, TEXT("ASSERT: [%i] %hs:\n'HUD' is not valid!"), __LINE__, __FUNCTION__))
		{
			break;
		}
		HUD->SetVisibility(ESlateVisibility::Collapsed);
		PlayerStateInternal->SetCharacterDead(false);
		PlayerStateInternal->SetOpponentKilledNum(0);
		PlayerStateInternal->SetEndGameState(EEndGameState::None);
		*/
		break;

	default: break;
	}
}

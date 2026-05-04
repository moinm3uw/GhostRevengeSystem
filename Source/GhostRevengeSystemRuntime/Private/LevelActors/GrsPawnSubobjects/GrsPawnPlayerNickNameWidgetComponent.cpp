#include "LevelActors/GrsPawnSubobjects/GrsPawnPlayerNickNameWidgetComponent.h"

#include "Components/BmrPlayerNameWidgetComponent.h"
#include "Components/GrsPlayerStateComponent.h"
#include "GameFramework/BmrPlayerState.h"
#include "LevelActors/GrsPawn.h"
#include "Utils/GrsPawnHelper.h"

// Initialize 3d widget component for the player name
void FGrsPawnPlayerNickNameWidgetComponent::SetupWidget(class AGrsPawn* GrsPawn)
{
	check(GrsPawn);

	PlayerName3DWidgetComponent = GrsPawn->CreateDefaultSubobject<UBmrPlayerNameWidgetComponent>(TEXT("PlayerName3DWidgetComponent"));
	PlayerName3DWidgetComponent->SetupAttachment(GrsPawn->GetRootComponent());
}

// Initialize player name widget (on top of character)
void FGrsPawnPlayerNickNameWidgetComponent::InitializePlayerNameWidget(AGrsPawn* GrsPawn)
{
	check(GrsPawn);

	ABmrPlayerState* MyPlayerState = Cast<ABmrPlayerState>(UGrsPawnHelper::GetPlayerStateForPlayerID(GrsPawn));
	if (!ensureMsgf(MyPlayerState, TEXT("ASSERT: [%i] %hs:\n'MyPlayerState' is not valid!"), __LINE__, __FUNCTION__)
	    || !ensureMsgf(PlayerName3DWidgetComponent, TEXT("ASSERT: [%i] %hs:\n'PlayerName3DWidgetComponent' is not valid!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	PlayerName3DWidgetComponent->Init(MyPlayerState);
}
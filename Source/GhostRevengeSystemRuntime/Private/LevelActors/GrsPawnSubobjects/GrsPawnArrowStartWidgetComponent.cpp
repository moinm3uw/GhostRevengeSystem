#include "LevelActors/GrsPawnSubobjects/GrsPawnArrowStartWidgetComponent.h"

#include "Components/BmrPlayerArrowStartComponent.h"
#include "LevelActors/GrsPawn.h"

//  Initialize 3D player arrow widget component that appears on top of character when player start to control it
void FGrsPawnArrowStartWidgetComponent::InitArrowStartWidgetComponent(class AGrsPawn* GrsPawn)
{
	check(GrsPawn);

	PlayerArrowStartComponent = GrsPawn->CreateDefaultSubobject<UBmrPlayerArrowStartComponent>(TEXT("PlayerArrowStartWidgetComponent"));
	PlayerArrowStartComponent->SetupAttachment(GrsPawn->GetRootComponent());
}

// Set visibility of the arrow on top of player character
void FGrsPawnArrowStartWidgetComponent::SetArrowEnabled(bool bVisibility)
{
	PlayerArrowStartComponent->SetArrowEnabled(bVisibility);
}
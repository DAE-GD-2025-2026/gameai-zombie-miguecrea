

#include "BlackBoard.h"


UBlackBoard::UBlackBoard()
{

	PrimaryComponentTick.bCanEverTick = true;
}


void UBlackBoard::BeginPlay()
{
	Super::BeginPlay();
}


void UBlackBoard::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

}


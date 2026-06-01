

#include "BlackBoardLozanoMiguel.h"


UBlackBoardLozanoMiguel::UBlackBoardLozanoMiguel()
{
	PrimaryComponentTick.bCanEverTick = true;
}


void UBlackBoardLozanoMiguel::BeginPlay()
{
	Super::BeginPlay();
}


void UBlackBoardLozanoMiguel::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

}


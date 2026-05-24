#include "FleeState.h"


UFleeState::UFleeState()
	: UStateBase()
{
}

void UFleeState::OnInit()
{
}

void UFleeState::OnEnter_Implementation(AActor* Owner)
{
	UE_LOG(LogTemp, Warning, TEXT("UFlee State OnEnter "));
}

void UFleeState::OnTick_Implementation(float DeltaTime, AActor* Owner)
{
	
}

void UFleeState::OnExit_Implementation(AActor* Owner)
{
}

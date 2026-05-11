#include "StateBase.h"


UStateBase::UStateBase()
{
	m_StateName = FName("UStateBase");
}


USeekState::USeekState():
UStateBase()
{
	FName df = FName("UStateBase");
	
	UE_LOG(LogTemp,Warning,TEXT("USeekState::USeekState(): %s"), *df.ToString());
}

void USeekState::OnEnter_Implementation(AActor* Owner)
{
	
}

void USeekState::OnTick(float DeltaTime, AActor* Owner)
{
}

void USeekState::OnExit(AActor* Owner)
{
}

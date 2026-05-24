#include "StateBase.h"

#include "FSMComponent.h"


UStateBase::UStateBase()
{
	m_StateName = GetClass()->GetFName();
}

void UStateBase::Init(UFSMComponent* InFSM)
{
	FSM = InFSM;
	OnInit();
}

AActor* UStateBase::GetOwnerActor() const
{
	return FSM.IsValid() ? FSM->GetOwner() : nullptr;
}

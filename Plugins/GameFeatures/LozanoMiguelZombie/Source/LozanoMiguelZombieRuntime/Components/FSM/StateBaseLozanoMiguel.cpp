#include "StateBaseLozanoMiguel.h"

#include "FSMComponentLozanoMiguel.h"


UStateBaseLozanoMiguel::UStateBaseLozanoMiguel()
{
	m_StateName = GetClass()->GetFName();
}

void UStateBaseLozanoMiguel::Init(UFSMComponentLozanoMiguel* InFSM)
{
	FSM = InFSM;
	OnInit();
}

AActor* UStateBaseLozanoMiguel::GetOwnerActor() const
{
	return FSM.IsValid() ? FSM->GetOwner() : nullptr;
}

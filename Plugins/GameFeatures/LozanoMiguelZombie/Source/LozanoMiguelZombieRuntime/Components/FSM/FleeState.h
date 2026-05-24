#pragma once

#include "CoreMinimal.h"
#include "StateBase.h"
#include "FleeState.generated.h"

UCLASS()
class LOZANOMIGUELZOMBIERUNTIME_API UFleeState : public UStateBase
{
	GENERATED_BODY()
public:
	UFleeState();
protected:
	virtual void OnInit() override;
	virtual void OnEnter_Implementation(AActor * Owner) override;
	virtual void OnTick_Implementation(float DeltaTime, AActor * Owner) override;
	virtual void OnExit_Implementation(AActor * Owner) override;
};

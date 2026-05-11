#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "StateBase.generated.h"

UCLASS(Abstract, Blueprintable, EditInlineNew)
class LOZANOMIGUELZOMBIERUNTIME_API UStateBase : public UObject
{
	GENERATED_BODY()
protected:
	
	FName m_StateName;
	
public:
	
	UStateBase();
	//
	UFUNCTION(BlueprintNativeEvent, Category="FSM")
	void OnEnter(AActor* Owner);
	virtual void OnEnter_Implementation(AActor * Owner) {}

	UFUNCTION(BlueprintNativeEvent, Category="FSM")
	void OnTick(float DeltaTime, AActor* Owner);
	virtual void OnTick_Implementation(float DeltaTime, AActor* Owner) {}

	UFUNCTION(BlueprintNativeEvent, Category="FSM")
	void OnExit(AActor* Owner);
	virtual void OnExit_Implementation(AActor * Owner) {}
};

class LOZANOMIGUELZOMBIERUNTIME_API   USeekState  : public UStateBase
{
public:
	
	USeekState();
	virtual void OnEnter_Implementation(AActor * Owner) override;
	void OnTick(float DeltaTime, AActor* Owner);
	void OnExit(AActor* Owner);
	
	
	
};

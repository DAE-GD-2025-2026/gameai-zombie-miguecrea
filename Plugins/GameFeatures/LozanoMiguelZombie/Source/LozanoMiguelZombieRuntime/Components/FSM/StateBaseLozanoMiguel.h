#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameFramework/Actor.h"
#include "StateBaseLozanoMiguel.generated.h"

class UFSMComponentLozanoMiguel;

UCLASS(Abstract, Blueprintable, EditInlineNew)
class LOZANOMIGUELZOMBIERUNTIME_API UStateBaseLozanoMiguel : public UObject
{
	GENERATED_BODY()
public:
	UStateBaseLozanoMiguel();

	void Init(UFSMComponentLozanoMiguel * InFSM);

	virtual void OnInit() {}

	FName GetStateName() const { return m_StateName; }

	UFUNCTION(BlueprintNativeEvent, Category="FSM")
	void OnEnter(AActor* Owner);
	virtual void OnEnter_Implementation(AActor * Owner) {}

	UFUNCTION(BlueprintNativeEvent, Category="FSM")
	void OnTick(float DeltaTime, AActor* Owner);
	virtual void OnTick_Implementation(float DeltaTime, AActor* Owner) {}

	UFUNCTION(BlueprintNativeEvent, Category="FSM")
	void OnExit(AActor* Owner);
	virtual void OnExit_Implementation(AActor * Owner) {}

protected:
	FName m_StateName;

	TWeakObjectPtr<UFSMComponentLozanoMiguel> FSM;

	AActor * GetOwnerActor() const;

	template<class T>
	T * GetSibling() const
	{
		AActor * O = GetOwnerActor();
		return O ? O->FindComponentByClass<T>() : nullptr;
	}
};

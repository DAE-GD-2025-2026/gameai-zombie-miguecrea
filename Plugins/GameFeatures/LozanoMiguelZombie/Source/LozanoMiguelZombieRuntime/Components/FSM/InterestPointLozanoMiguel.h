#pragma once

#include "CoreMinimal.h"
#include "ReasonToMoveLozanoMiguel.h"
#include "InterestPointLozanoMiguel.generated.h"

USTRUCT(BlueprintType)
struct FInterestPointLozanoMiguel
{
	GENERATED_BODY()

	UPROPERTY()
	TWeakObjectPtr<AActor> Actor;

	UPROPERTY()
	bool m_Visited = false;

	bool operator==(const FInterestPointLozanoMiguel& Other) const
	{
		return Actor == Other.Actor;
	}
};

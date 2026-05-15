#pragma once

#include "CoreMinimal.h"
#include "InterestPoint.generated.h"

USTRUCT(BlueprintType)
struct FInterestPoint
{
	GENERATED_BODY()

	UPROPERTY()
	TWeakObjectPtr<AActor> Actor;

	UPROPERTY()
	bool m_Visited = false;

	bool operator==(const FInterestPoint& Other) const
	{
		return Actor == Other.Actor;
	}
};
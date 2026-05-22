#pragma once

#include "CoreMinimal.h"
#include "ReasonToMove.generated.h"

// Tag describing WHY the FSM issued a Move. Read by HandleArrived to pick
// the right post-arrival behavior (set BB keys to enter Loot, advance
// patrol, etc.). Kept in its own header so non-state code (e.g., debug
// tooling, future BB writers) can reference it without dragging in all of
// StateBase.h.
UENUM(BlueprintType)
enum class EReasonToMove : uint8
{
	VisitHouse,
	Loot,
	Explore,
	UNDEFINED
};

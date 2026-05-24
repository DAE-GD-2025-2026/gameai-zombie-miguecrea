#pragma once

#include "CoreMinimal.h"


// `inline const` (C++17) means each key has exactly one definition across
// the whole module even though this header is included many places.

namespace BBKeys
{
	// ---- Movement / destinations ------------------------------------------
	inline const FName CurrentDestination(TEXT("CurrentDestination"));

	
	
	// ---- Perception flags (set by UStudentPerceptor) ----------------------
	inline const FName bArrivedAtInterestPoint (TEXT("bArrivedAtInterestPoint"));
	inline const FName bLootDone      (TEXT("bLootDone"));
	inline const FName bItem      (TEXT("bItem"));
	inline const FName bThreatNearby  (TEXT("bThreatNearby"));
	inline const FName bThreatGone    (TEXT("bThreatGone"));
	inline const FName bShouldSuicide    (TEXT("bShouldSuicide"));
	
	
	
	
	inline const FName ThreatLocation (TEXT("ThreatLocation"));
	inline const FName DistanceToTarget(TEXT("DistanceToTarget"));

	// ---- Memory (set by UMemoryComponent) ---------------------------------
	inline const FName ClosestItem    (TEXT("ClosestItem"));
	inline const FName ClosestHouse   (TEXT("ClosestHouse"));

	// ---- State-machine transition triggers --------------------------------
	inline const FName bAtTarget      (TEXT("bAtTarget"));
}

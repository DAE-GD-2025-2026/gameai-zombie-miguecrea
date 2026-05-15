#pragma once

#include "CoreMinimal.h"

// All blackboard key names live here. Reference these constants everywhere
// instead of inline TEXT("...") literals. One typo, one definition.
//
// Add a new key:  add a line below, include this header where you need it.
// Rename a key:   change the string here and the Find/Replace on the symbol
//                 takes care of every callsite. No more silent string drift.
//
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
	inline const FName ThreatLocation (TEXT("ThreatLocation"));
	inline const FName DistanceToTarget(TEXT("DistanceToTarget"));

	// ---- Memory (set by UMemoryComponent) ---------------------------------
	inline const FName ClosestItem    (TEXT("ClosestItem"));
	inline const FName ClosestHouse   (TEXT("ClosestHouse"));

	// ---- State-machine transition triggers --------------------------------
	inline const FName bAtTarget      (TEXT("bAtTarget"));
	inline const FName bThreatGone    (TEXT("bThreatGone"));
}

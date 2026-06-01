#pragma once

#include "CoreMinimal.h"


// `inline const` (C++17) means each key has exactly one definition across
// the whole module even though this header is included many places.

namespace BBKeysLozanoMiguel
{
	// ---- Movement / destinations ------------------------------------------
	inline const FName CurrentDestination(TEXT("CurrentDestination"));

	
	
	// ---- Perception flags (set by UStudentPerceptorLozanoMiguel) ----------
	inline const FName bArrivedAtInterestPoint (TEXT("bArrivedAtInterestPoint"));
	inline const FName bLootDone      (TEXT("bLootDone"));
	inline const FName bItem      (TEXT("bItem"));
	inline const FName bThreatNearby  (TEXT("bThreatNearby"));
	inline const FName bThreatGone    (TEXT("bThreatGone"));
	inline const FName bShouldSuicide    (TEXT("bShouldSuicide"));

	// Perceptor polls all APurgeZone actors each tick and sets this true if
	// any zone's danger radius (Diameter/2 + buffer) overlaps the survivor.
	// The FSM's →Flee global transition reads this. Higher priority than
	// →Combat so a survivor never picks a fight inside a purge zone.
	inline const FName bPurgeZoneNearby  (TEXT("bPurgeZoneNearby"));
	
	
	
	
	inline const FName ThreatLocation (TEXT("ThreatLocation"));
	inline const FName DistanceToTarget(TEXT("DistanceToTarget"));

	// ---- Memory (set by UMemoryComponent) ---------------------------------
	inline const FName ClosestItem    (TEXT("ClosestItem"));
	inline const FName ClosestHouse   (TEXT("ClosestHouse"));

	// ---- State-machine transition triggers --------------------------------
	inline const FName bAtTarget      (TEXT("bAtTarget"));
}

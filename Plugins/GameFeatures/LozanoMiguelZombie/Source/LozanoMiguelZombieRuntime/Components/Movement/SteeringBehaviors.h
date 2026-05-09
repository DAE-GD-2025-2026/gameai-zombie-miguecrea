// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SteeringHelpers.h"
#include "Kismet/KismetMathLibrary.h"

class ASurvivorPawn;
class LOZANOMIGUELZOMBIERUNTIME_API SteeringBehaviors
{
public:
	
	SteeringBehaviors() = default;
	virtual ~SteeringBehaviors() = default;

	// Override to implement your own behavior
	virtual SteeringBehaviors CalculateSteering(float DeltaT,ASurvivorPawn * SurvivorPawn) = 0;

	void SetTarget(const FTargetData & NewTarget) { m_Target = NewTarget; }
	
	FVector2D AtoB(FVector2D self,FVector2D target);
	template<class T, std::enable_if_t<std::is_base_of_v<SteeringBehaviors, T>>* = nullptr>
	T* As()
	{ return static_cast<T*>(this);}

	bool ArrivedToTarget(ASurvivorPawn  *  SurvivorPawn);

protected:
	
	FTargetData m_Target;
};


class LOZANOMIGUELZOMBIERUNTIME_API Seek : public SteeringBehaviors
{
public:
	Seek() = default;
	virtual ~Seek() = default;
protected:
};






// class Wander : public ISteeringBehavior
// {
// public:
// 	Wander();
// 	virtual ~Wander() = default;
// 	virtual SteeringOutput CalculateSteering(float DeltaT, ASteeringAgent & Agent)  override;
//
// 	float m_Radius = 200.f;
// 	float m_Angle = 0.0f;
//
// 	float m_TotalTimeElapsed = 0.0f;
// 	float m_AngleUpdateTime = 0.4f;
// 	float m_AngleInRadians = 0.0f;
//
// protected:
// };
//
// class Flee : public ISteeringBehavior
// {
// public:
// 	Flee() = default;
// 	virtual ~Flee() = default;
// 	virtual SteeringOutput CalculateSteering(float DeltaT, ASteeringAgent & Agent)  override;
// protected:
// };
//
// class Pursuit : public ISteeringBehavior
// {
// public:
// 	Pursuit() = default;
// 	virtual ~Pursuit() = default;
// 	virtual SteeringOutput CalculateSteering(float DeltaT, ASteeringAgent& Agent)  override;
// protected:
// };
//
//
// class Evade : public ISteeringBehavior
// {
// public:
// 	Evade() = default;
// 	virtual ~Evade() = default;
// 	virtual SteeringOutput CalculateSteering(float DeltaT, ASteeringAgent& Agent)  override;
// 	float m_EvadeRadius = 400.f;
// protected:
// };
//
//
// class Arrive : public ISteeringBehavior
// {
// public:
// 	Arrive() = default;
// 	virtual ~Arrive() = default;
// 	virtual SteeringOutput CalculateSteering(float DeltaT, ASteeringAgent& Agent)  override;
// 	
// 	void SetTargetRadius(float newstopRadius) { m_StopRadius = newstopRadius; }
// 	float  m_StopRadius{100.f};
// 	float m_SlowDownRadius{350.f};
//
// protected:
// };

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Damage.h"
#include "Perception/AISense_Damage.h"
#include "StudentPerceptor.generated.h"


class ASurvivorPawn;

class UAIPerceptionComponent;
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LOZANOMIGUELZOMBIERUNTIME_API UStudentPerceptor : public UActorComponent
{
	GENERATED_BODY()
	
	
private:
	UAIPerceptionComponent * m_PerceptionComponent;

	ASurvivorPawn * m_SurvivorPawn = nullptr;
public:
	UStudentPerceptor();
	virtual void BeginPlay() override;
	void TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction) override;

///////
	UFUNCTION()
	virtual void OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);
	
	UFUNCTION()
	void OnTargetForgotten(AActor* Actor);
	
	void DeferredInit();
	
	TArray<AActor*> GetSeenActorsInMemory();
	TArray<AActor*> GetActorsOnFOV();
	void ForgetActorsFromMemory(AActor * Actor);
	
	


	
};

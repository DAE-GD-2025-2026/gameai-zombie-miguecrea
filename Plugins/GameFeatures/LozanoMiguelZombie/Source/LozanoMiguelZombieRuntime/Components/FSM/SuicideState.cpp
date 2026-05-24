#include "SuicideState.h"

#include "FSMComponent.h"
#include "BehaviorTree/BlackboardComponent.h"

#include "../Movement/SteeringComponent.h"
#include "../BlackBoard/BBKeys.h"
#include "../MACROS/DebugMacro.h"

#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"


USuicideState::USuicideState()
	: UStateBase()
{
}

void USuicideState::OnInit()
{
	SteeringComponent = GetSibling<USteeringComponent>();

	static const TCHAR* ExplosionPath =
		TEXT("/LozanoMiguelZombie/Sounds/ExplosionSound.ExplosionSound");
	static const TCHAR* TickBombPath  =
		TEXT("/LozanoMiguelZombie/Sounds/freesound_community-ticking-bomb-90319.freesound_community-ticking-bomb-90319");

	m_ExplosionSound = LoadObject<USoundBase>(nullptr, ExplosionPath);
	m_TickBombSound  = LoadObject<USoundBase>(nullptr, TickBombPath);

	if (!m_ExplosionSound)
	{
		UE_LOG(LogTemp, Error, TEXT("[Suicide] Failed to load explosion sound at '%s'."), ExplosionPath);
	}
	if (!m_TickBombSound)
	{
		UE_LOG(LogTemp, Error, TEXT("[Suicide] Failed to load tick-bomb sound at '%s'."), TickBombPath);
	}
}

void USuicideState::OnEnter_Implementation(AActor* Owner)
{
	FSM->Blackboard->SetValueAsBool(BBKeys::bThreatNearby, false);
	FSM->Blackboard->SetValueAsBool(BBKeys::bThreatGone,   false);
	UE_LOG(LogTemp, Warning, TEXT("Suicide State OnEnter "));

	if (SteeringComponent.IsValid())
	{
		SteeringComponent->StopMoving();
	}

	if (!Owner) return;
	LocationToExplode = Owner->GetActorLocation();

	if (m_TickBombSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			Owner, m_TickBombSound, Owner->GetActorLocation());
	}

	FTimerHandle ExplodeTimer;
	GetWorld()->GetTimerManager().SetTimer(
		ExplodeTimer, this, &USuicideState::Explode, 3.f, false);
}

void USuicideState::OnTick_Implementation(float DeltaTime, AActor* Owner)
{
	if (m_UpdateRadius)
	{
		Timer += DeltaTime;
		constexpr float PulseDuration = 0.6f;
		const float T = FMath::Fmod(Timer, PulseDuration) / PulseDuration;
		m_ExplosionRadius = m_Radius * T;
	}
	DRAW_CIRCLE(GetWorld(), Owner->GetActorLocation(), m_ExplosionRadius, FColor::Red, 3.f);
}

void USuicideState::Explode()
{
	if (m_ExplosionSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			GetWorld(), m_ExplosionSound, LocationToExplode);
	}

	FTimerHandle KillPlayerTimer;
	GetWorld()->GetTimerManager().SetTimer(
		KillPlayerTimer, this, &USuicideState::KillPlayer, 0.4f, false);
}

void USuicideState::KillPlayer()
{
	UGameplayStatics::ApplyRadialDamage(
		GetWorld(),
		99999.f,
		LocationToExplode,
		m_Radius,
		UDamageType::StaticClass(),
		TArray<AActor*>(),
		nullptr,
		nullptr,
		true);
}

void USuicideState::OnExit_Implementation(AActor* Owner)
{
}

#include "SuicideStateLozanoMiguel.h"

#include "FSMComponentLozanoMiguel.h"
#include "BehaviorTree/BlackboardComponent.h"

#include "../Movement/SteeringComponentLozanoMiguel.h"
#include "../BlackBoard/BBKeysLozanoMiguel.h"
#include "../MACROS/DebugMacro.h"

#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"


USuicideStateLozanoMiguel::USuicideStateLozanoMiguel()
	: UStateBaseLozanoMiguel()
{
}

void USuicideStateLozanoMiguel::OnInit()
{
	SteeringComponent = GetSibling<USteeringComponentLozanoMiguel>();

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

void USuicideStateLozanoMiguel::OnEnter_Implementation(AActor* Owner)
{
	FSM->Blackboard->SetValueAsBool(BBKeysLozanoMiguel::bThreatNearby, false);
	FSM->Blackboard->SetValueAsBool(BBKeysLozanoMiguel::bThreatGone,   false);
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
		ExplodeTimer, this, &USuicideStateLozanoMiguel::Explode, 3.f, false);
}

void USuicideStateLozanoMiguel::OnTick_Implementation(float DeltaTime, AActor* Owner)
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

void USuicideStateLozanoMiguel::Explode()
{
	if (m_ExplosionSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			GetWorld(), m_ExplosionSound, LocationToExplode);
	}

	FTimerHandle KillPlayerTimer;
	GetWorld()->GetTimerManager().SetTimer(
		KillPlayerTimer, this, &USuicideStateLozanoMiguel::KillPlayer, 0.4f, false);
}

void USuicideStateLozanoMiguel::KillPlayer()
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

void USuicideStateLozanoMiguel::OnExit_Implementation(AActor* Owner)
{
}

#include "CombatState.h"

#include "FSMComponent.h"
#include "InterestPoint.h"
#include "BehaviorTree/BlackboardComponent.h"

#include "../Movement/SteeringComponent.h"
#include "../BlackBoard/BBKeys.h"
#include "../StudentPerceptor/StudentPerceptor.h"
#include "Common/InventoryComponent.h"
#include "Items/BaseItem.h"
#include "Items/ItemType.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"
#include "Zombies/BaseZombie.h"
#include "DrawDebugHelpers.h"
#include "CollisionQueryParams.h"


UCombatState::UCombatState()
	: UStateBase()
{
}

void UCombatState::OnInit()
{
	SteeringComponent = GetSibling<USteeringComponent>();
	Memory            = GetSibling<UStudentPerceptor>();
	Inventory         = GetSibling<UInventoryComponent>();

	static const TCHAR* PistolPath   = TEXT("/LozanoMiguelZombie/Sounds/Pistol.Pistol");
	static const TCHAR* ShotgunPath  = TEXT("/LozanoMiguelZombie/Sounds/Shotgun.Shotgun");
	static const TCHAR* PanicPath    = TEXT("/LozanoMiguelZombie/Sounds/OhShitScream.OhShitScream");
	static const TCHAR* EmptyGunPath = TEXT("/LozanoMiguelZombie/Sounds/EmptyGun.EmptyGun");

	m_PistolSound   = LoadObject<USoundBase>(nullptr, PistolPath);
	m_ShotgunSound  = LoadObject<USoundBase>(nullptr, ShotgunPath);
	m_PanicSound    = LoadObject<USoundBase>(nullptr, PanicPath);
	m_EmptyGunSound = LoadObject<USoundBase>(nullptr, EmptyGunPath);

	if (!m_PistolSound)   UE_LOG(LogTemp, Error, TEXT("[Combat] Failed to load pistol sound at '%s'."),   PistolPath);
	if (!m_ShotgunSound)  UE_LOG(LogTemp, Error, TEXT("[Combat] Failed to load shotgun sound at '%s'."),  ShotgunPath);
	if (!m_PanicSound)    UE_LOG(LogTemp, Error, TEXT("[Combat] Failed to load panic sound at '%s'."),    PanicPath);
	if (!m_EmptyGunSound) UE_LOG(LogTemp, Error, TEXT("[Combat] Failed to load empty-gun sound at '%s'."), EmptyGunPath);
}

void UCombatState::OnEnter_Implementation(AActor* Owner)
{
	UE_LOG(LogTemp, Warning, TEXT("UCombat State OnEnter"));

	if (FSM.IsValid() && FSM->Blackboard.IsValid())
	{
		FSM->Blackboard->SetValueAsBool(BBKeys::bThreatGone, false);
	}
	if (SteeringComponent.IsValid()) SteeringComponent->StopMoving();
	if (SteeringComponent.IsValid()) SteeringComponent->SetRotate(false);

	m_CurrentTarget.Reset();
}

bool UCombatState::HandleShooting(AActor* Owner, ABaseZombie* Target, bool IsFacingTarget)
{
	if (IsFacingTarget && m_CanShoot)
	{
		const TArray<int32> WeaponSlots = FindWeaponSlots();
		if (WeaponSlots.IsEmpty())
		{
			// Fall back to a remembered weapon if any.
			m_ClosestWeaponInMemory.Reset();
			if (FInterestPoint * WeaponIP = Memory->GetClosestWeaponInMemory())
			{
				m_ClosestWeaponInMemory = Cast<ABaseItem>(WeaponIP->Actor.Get());
			}

			if (m_ClosestWeaponInMemory.IsValid())
			{
				m_OnDangerSearchingOldWeapon = true;
				UE_LOG(LogTemp, Warning, TEXT("[Combat] Weapons are not available Searching for And Old One"));
			}
			else
			{
				FSM->Blackboard->SetValueAsBool(BBKeys::bShouldSuicide, true);
				UE_LOG(LogTemp, Warning, TEXT("[Combat] Weapons are not available Suiciding."));
			}
			return true;
		}

		for (int32 Slot : WeaponSlots)
		{
			ABaseItem* WeaponItem = Inventory->GetInventory()[Slot];
			if (!WeaponItem)
			{
				UE_LOG(LogTemp, Error, TEXT("Not Valid Item"))
				continue;
			}
			SteeringComponent->Move(Owner->GetActorLocation() + Target->GetActorForwardVector() * 400.f);

			bool HasValue = WeaponItem->GetValue() > 0;
			m_CanShoot = false;
			if (!HasValue)
			{
				UE_LOG(LogTemp, Error, TEXT("NO AMMO"))
				Inventory->RemoveItem(Slot);
				PlayEmptyWeaponSound();
				GetWorld()->GetTimerManager().SetTimer(
					m_OhShitSounds, this, &UCombatState::PlayOhShitSound, 0.3f, false);
				GetWorld()->GetTimerManager().SetTimer(
					m_ResetShootingFlag, this, &UCombatState::ResetShootFlag, 0.6f, false);
			}
			else
			{
				SteeringComponent->Move(Owner->GetActorLocation() + Target->GetActorForwardVector() * 400.f);
				UE_LOG(LogTemp, Warning, TEXT(" SHOOT"))
				Inventory->UseItem(Slot);
				PlayWeaponSound(Owner, WeaponItem);
				DrawWeaponTrace(Owner, WeaponItem);
				GetWorld()->GetTimerManager().SetTimer(
					m_ResetShootingFlag, this, &UCombatState::ResetShootFlag, 0.3f, false);
				int value = WeaponItem->GetValue();
				UE_LOG(LogTemp, Warning, TEXT(" VALUE IS : %d"), value)
			}
			break;
		}
	}
	return false;
}

void UCombatState::OnTick_Implementation(float DeltaTime, AActor* Owner)
{
	if (!Owner || !Memory.IsValid()) return;

	// Still On Combat — chasing a remembered weapon.
	if (m_OnDangerSearchingOldWeapon)
	{
		if (m_ClosestWeaponInMemory.IsValid())
		{
			SteeringComponent->Move(m_ClosestWeaponInMemory->GetActorLocation());
		}
		return;
	}

	auto Zombies = Memory->GetVisibleZombies();
	if (Zombies.IsEmpty())
	{
		FTimerManager& TM = GetWorld()->GetTimerManager();
		if (!TM.IsTimerActive(m_NoMoreZombiesAroundTimer))
		{
			TM.SetTimer(m_NoMoreZombiesAroundTimer, this,
				&UCombatState::GoBackToWander, 1.f, false);
		}
	}
	else
	{
		GetWorld()->GetTimerManager().ClearTimer(m_NoMoreZombiesAroundTimer);
	}

	ABaseZombie* Target = PickClosestZombie(Zombies, Owner);
	if (!Target) return;
	m_CurrentTarget = Target;
	bool IsFacingTarget = FaceTarget(DeltaTime, Owner, Target);

	if (HandleShooting(Owner, Target, IsFacingTarget)) return;
}

void UCombatState::PlayOhShitSound()
{
	AActor* Owner = GetOwnerActor();
	if (!Owner || !m_PanicSound) return;
	UGameplayStatics::PlaySoundAtLocation(Owner, m_PanicSound, Owner->GetActorLocation());
}

void UCombatState::PlayEmptyWeaponSound()
{
	AActor* Owner = GetOwnerActor();
	if (!Owner || !m_EmptyGunSound) return;
	UGameplayStatics::PlaySoundAtLocation(Owner, m_EmptyGunSound, Owner->GetActorLocation());
}

void UCombatState::ResetShootFlag()
{
	m_CanShoot = true;
}

void UCombatState::GoBackToWander()
{
	FSM->Blackboard->SetValueAsBool(BBKeys::bThreatGone, true);
}

// ---- Helpers ---------------------------------------------------------------

ABaseZombie* UCombatState::PickClosestZombie(
	const TArray<ABaseZombie*>& Zombies, AActor* Owner) const
{
	if (Zombies.IsEmpty() || !Owner) return nullptr;
	const FVector MyLoc = Owner->GetActorLocation();

	ABaseZombie* Closest = nullptr;
	float ClosestDistSq = TNumericLimits<float>::Max();
	for (ABaseZombie* Z : Zombies)
	{
		if (!Z) continue;
		const float DistSq = FVector::DistSquared(MyLoc, Z->GetActorLocation());
		if (DistSq < ClosestDistSq)
		{
			ClosestDistSq = DistSq;
			Closest = Z;
		}
	}
	return Closest;
}

TArray<int32> UCombatState::FindWeaponSlots() const
{
	TArray<int32> Slots;
	if (!Inventory.IsValid()) return Slots;

	const TArray<ABaseItem*>& All = Inventory->GetInventory();
	for (int32 i = 0; i < All.Num(); ++i)
	{
		if (!All[i]) continue;
		const EItemType T = All[i]->GetItemType();
		if (T == EItemType::Pistol || T == EItemType::Shotgun)
		{
			Slots.Add(i);
		}
	}
	return Slots;
}

bool UCombatState::FaceTarget(float Dt, AActor* Owner, AActor* Threat) const
{
	if (!Owner || !Threat) return false;
	const FVector ToTarget =
		(Threat->GetActorLocation() - Owner->GetActorLocation()).GetSafeNormal2D();
	if (ToTarget.IsNearlyZero()) return false;

	FRotator TargetRot = ToTarget.Rotation();
	TargetRot.Pitch = 0.f;
	TargetRot.Roll  = 0.f;

	const FRotator NewRot = FMath::RInterpTo(Owner->GetActorRotation(), TargetRot, Dt, FaceTargetSpeed);
	Owner->SetActorRotation(NewRot);

	const float DeltaYaw =
		FMath::Abs(FRotator::NormalizeAxis(Owner->GetActorRotation().Yaw - TargetRot.Yaw));
	return DeltaYaw <= 2.0f;
}

void UCombatState::OnExit_Implementation(AActor* Owner)
{
	ResetShootFlag();
	m_CurrentTarget.Reset();
	m_Timer     = 0.f;
	m_FireTimer = 0.f;
}


void UCombatState::DrawWeaponTrace(AActor* Owner, ABaseItem* WeaponItem) const
{
	if (!Owner || !WeaponItem) return;
	UWorld* W = Owner->GetWorld();
	if (!W) return;

	const FVector Start   = Owner->GetActorLocation();
	const FVector Forward = Owner->GetActorForwardVector();

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Owner);

	auto TraceAndDraw = [&](const FVector& Direction)
	{
		const FVector End = Start + Direction * 10000.f;

		FHitResult Hit;
		const bool bHit = W->LineTraceSingleByChannel(Hit, Start, End, ECC_Pawn, Params);

		const FVector LineEnd = bHit ? Hit.ImpactPoint : End;
		const FColor  LineCol = bHit ? FColor::Red    : FColor::Yellow;

		DrawDebugLine(W, Start, LineEnd, LineCol,
			false, 1.0f, 0, 2.f);

		if (bHit)
		{
			DrawDebugSphere(W, Hit.ImpactPoint, 20.f, 8, FColor::Red,
				false, 1.0f, 0, 1.f);
		}
	};

	switch (WeaponItem->GetItemType())
	{
	case EItemType::Pistol:
		TraceAndDraw(Forward);
		break;
	case EItemType::Shotgun:
	{
		constexpr int32 ShotsPerAmmo  = 3;
		constexpr float MaxSprayDelta = 10.f;
		for (int32 i = 0; i < ShotsPerAmmo; ++i)
		{
			const float YawDeg = FMath::RandRange(-MaxSprayDelta, MaxSprayDelta);
			const FVector Dir = Forward.ToOrientationRotator().Add(0.f, YawDeg, 0.f).Vector();
			TraceAndDraw(Dir);
		}
		break;
	}
	default:
		break;
	}
}

void UCombatState::PlayWeaponSound(AActor* Owner, ABaseItem* WeaponItem) const
{
	if (!Owner || !WeaponItem) return;

	USoundBase* SoundToPlay = nullptr;
	switch (WeaponItem->GetItemType())
	{
	case EItemType::Pistol:  SoundToPlay = m_PistolSound;  break;
	case EItemType::Shotgun: SoundToPlay = m_ShotgunSound; break;
	default: return;
	}

	if (!SoundToPlay) return;

	UGameplayStatics::PlaySoundAtLocation(
		Owner, SoundToPlay, Owner->GetActorLocation(), 1.0f, 1.0f);
}

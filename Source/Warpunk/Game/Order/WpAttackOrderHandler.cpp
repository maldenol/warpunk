#include "WpAttackOrderHandler.h"

#include "Warpunk/Game/Entity/WpEntity.h"
#include "Warpunk/Game/Weapon/WpWeapon.h"
#include "Warpunk/Game/Weapon/WpWeaponComponent.h"

#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"

FWpAttackOrderHandler::FWpAttackOrderHandler()
{
	SetTickCooldown(0.25f);
}

void FWpAttackOrderHandler::Initialize(AWpEntity* const InEntity)
{
	FWpOrderHandler::Initialize(InEntity);

	AIController = GetEntity()->GetController<AAIController>();
	ensureAlways(AIController);

	WeaponComponent = GetEntity()->GetWeaponComponent();
	ensureAlways(WeaponComponent->IsActive());

	for (uint32 Index = 0; Index < WP_ENTITY_MAX_WEAPON; ++Index)
	{
		if (const FWpWeapon* const Weapon = WeaponComponent->GetWeapon(Index))
		{
			if (!ShortestRangeWeapon || (Weapon->GetRange() < ShortestRangeWeapon->GetRange()))
			{
				ShortestRangeWeapon = Weapon;
			}
		}
		else
		{
			break;
		}
	}
}

void FWpAttackOrderHandler::Reactivate()
{
	FWpOrderHandler::Reactivate();

	if (!GetOrder().TargetEntity)
	{
		Finish();
		return;
	}

	for (uint32 Index = 0; Index < WP_ENTITY_MAX_WEAPON; ++Index)
	{
		if (FWpWeapon* const Weapon = WeaponComponent->GetWeapon(Index))
		{
			Weapon->SetPriorityTargetEntity(GetOrder().TargetEntity);
		}
		else
		{
			break;
		}
	}
}

void FWpAttackOrderHandler::Tick(const float CurrentTime)
{
	FWpOrderHandler::Tick(CurrentTime);

	if (!GetOrder().TargetEntity)
	{
		Finish();
		return;
	}

	const bool bEveryWeaponIsInReach = ShortestRangeWeapon->CanReach(GetOrder().TargetEntity->GetActorLocation());
	const bool bIsMoving = AIController->GetMoveStatus() != EPathFollowingStatus::Type::Idle;
	if (!bEveryWeaponIsInReach && !bIsMoving)
	{
		AIController->MoveToActor(GetOrder().TargetEntity, ShortestRangeWeapon->GetRange(), false, true, false, nullptr, false);
	}
}

void FWpAttackOrderHandler::Deactivate()
{
	if (AIController)
	{
		AIController->StopMovement();
	}

	FWpOrderHandler::Deactivate();
}

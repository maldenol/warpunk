#include "WpAttackOrderHandler.h"

#include "Warpunk/Game/Entity/WpEntityInterface.h"
#include "Warpunk/Game/Weapon/WpWeapon.h"
#include "Warpunk/Game/Weapon/WpWeaponComponent.h"

#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"

FWpAttackOrderHandler::FWpAttackOrderHandler()
{
	SetTickCooldown(0.25f);
}

void FWpAttackOrderHandler::Initialize(IWpEntityInterface* const InEntity)
{
	FWpOrderHandler::Initialize(InEntity);

	AIController = Cast<APawn>(GetEntity())->GetController<AAIController>();
	ensureAlways(AIController);

	WeaponComponent = GetEntity()->GetWeaponComponent();

	for (uint32 Index = 0; Index < WP_ENTITY_MAX_WEAPON; ++Index)
	{
		if (const FWpWeapon* const Weapon = WeaponComponent->GetWeapon(Index))
		{
			if (!ShortestRangeWeapon || Weapon->GetRange() < ShortestRangeWeapon->GetRange())
			{
				ShortestRangeWeapon = Weapon;
			}
		}
		else
		{
			break;
		}
	}

	const bool bHasAtLeastOneWeapon = ShortestRangeWeapon != nullptr;
	ensureAlways(bHasAtLeastOneWeapon);
}

void FWpAttackOrderHandler::Reactivate()
{
	FWpOrderHandler::Reactivate();

	for (uint32 Index = 0; Index < WP_ENTITY_MAX_WEAPON; ++Index)
	{
		if (FWpWeapon* const Weapon = WeaponComponent->GetWeapon(Index))
		{
			Weapon->SetPriorityTargetEntity(GetOrder().GetTargetEntity());
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

	if (!IsValid(GetOrder().TargetEntityActor))
	{
		Finish();
		return;
	}

	const bool bEveryWeaponIsInReach = ShortestRangeWeapon->CanReach(GetOrder().TargetEntityActor->GetActorLocation());
	const bool bIsMoving = AIController->GetMoveStatus() != EPathFollowingStatus::Type::Idle;
	if (!bEveryWeaponIsInReach && !bIsMoving)
	{
		AIController->MoveToActor(GetOrder().TargetEntityActor, ShortestRangeWeapon->GetRange(), false, true, false, nullptr, false);
	}
}

void FWpAttackOrderHandler::Deactivate()
{
	if (IsValid(AIController))
	{
		AIController->StopMovement();
	}

	FWpOrderHandler::Deactivate();
}

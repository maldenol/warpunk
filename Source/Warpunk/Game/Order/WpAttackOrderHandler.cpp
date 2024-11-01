#include "WpAttackOrderHandler.h"

#include "Warpunk/Game/EntityComponent/WpEntityComponent.h"
#include "Warpunk/Game/Weapon/WpWeapon.h"
#include "Warpunk/Game/Weapon/WpWeaponArray.h"

#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"

void FWpAttackOrderHandler::SetOwnerEntityComponent(UWpEntityComponent* const InOwnerEntityComponent)
{
	FWpOrderHandler::SetOwnerEntityComponent(InOwnerEntityComponent);

	AIController = GetOwnerEntityComponent()->GetOwner<APawn>()->GetController<AAIController>();
	ensureAlways(AIController);

	WeaponArray = &WP_AS_WEAPON_ARRAY(GetOwnerEntityComponent());

	for (uint32 Index = 0; Index < WP_ENTITY_MAX_WEAPON; ++Index)
	{
		if (const FWpWeapon* const Weapon = WeaponArray->GetWeapon(Index))
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
		if (FWpWeapon* const Weapon = WeaponArray->GetWeapon(Index))
		{
			Weapon->SetPriorityTarget(GetOrder().TargetEntityComponent);
		}
		else
		{
			break;
		}
	}
}

void FWpAttackOrderHandler::Tick()
{
	FWpOrderHandler::Tick();

	if (!IsValid(GetOrder().TargetEntityComponent))
	{
		Finish();
		return;
	}

	const bool bEveryWeaponIsInReach = ShortestRangeWeapon->CanReach(GetOrder().TargetEntityComponent->GetOwner()->GetActorLocation());
	const bool bIsMoving = AIController->GetMoveStatus() != EPathFollowingStatus::Type::Idle;
	if (!bEveryWeaponIsInReach && !bIsMoving)
	{
		AIController->MoveToActor(GetOrder().TargetEntityComponent->GetOwner(), ShortestRangeWeapon->GetRange(), false, true, false, nullptr, false);
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

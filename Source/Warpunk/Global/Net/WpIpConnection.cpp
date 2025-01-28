#include "WpIpConnection.h"

#include "Warpunk/Game/Player/WpGamePlayerController.h"
#include "Warpunk/Game/Player/WpGamePlayerState.h"
#include "Warpunk/Game/Pool/WpPooledActorInterface.h"
#include "Warpunk/Game/Pool/WpPooledActorVisibilityManager.h"

void UWpIpConnection::FlushDormancy(AActor* const Actor)
{
	if (PlayerController->GetClass()->IsChildOf<AWpGamePlayerController>())
	{
		if (Actor->Implements<UWpPooledActorInterface>())
		{
			const AWpGamePlayerState* const PlayerState = PlayerController->GetPlayerState<AWpGamePlayerState>();
			const uint32 PlayerTeamId = PlayerState->GetTeamId();
			if (!AWpPooledActorVisibilityManager::IsVisibleForTeam(PlayerTeamId, Actor))
			{
				return;
			}
		}
	}

	Super::FlushDormancy(Actor);
}

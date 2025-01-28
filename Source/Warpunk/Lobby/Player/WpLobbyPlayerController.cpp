#include "WpLobbyPlayerController.h"

#include "Warpunk/Global/GameInstance/WpGameInstance.h"
#include "Warpunk/Global/LoadingScreen/WpLoadingScreen.h"
#include "Warpunk/Lobby/Player/WpLobbyPlayerState.h"

AWpLobbyPlayerController::AWpLobbyPlayerController()
{
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
}

void AWpLobbyPlayerController::PreClientTravel(const FString& PendingURL, ETravelType TravelType, bool bIsSeamlessTravel)
{
	Super::PreClientTravel(PendingURL, TravelType, bIsSeamlessTravel);

	GetGameInstance<UWpGameInstance>()->GetLoadingScreen()->EnableLoadingScreen(true);
}

void AWpLobbyPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	PlayerStateReplicatedEvent.Broadcast();
}

void AWpLobbyPlayerController::ServerSetBelligerentId_Implementation(const uint32 BelligerentId)
{
	AWpLobbyPlayerState* const InnerPlayerState = GetPlayerState<AWpLobbyPlayerState>();
	InnerPlayerState->SetBelligerentId(BelligerentId);
}

bool AWpLobbyPlayerController::ServerSetBelligerentId_Validate(const uint32 BelligerentId)
{
	return BelligerentId != 0;
}

void AWpLobbyPlayerController::ServerSetTeamId_Implementation(const uint32 TeamId)
{
	AWpLobbyPlayerState* const InnerPlayerState = GetPlayerState<AWpLobbyPlayerState>();
	InnerPlayerState->SetTeamId(TeamId);
}

bool AWpLobbyPlayerController::ServerSetTeamId_Validate(const uint32 TeamId)
{
	return TeamId != 0;
}

void AWpLobbyPlayerController::ServerSetColor_Implementation(const FColor& Color)
{
	AWpLobbyPlayerState* const InnerPlayerState = GetPlayerState<AWpLobbyPlayerState>();
	InnerPlayerState->SetColor(Color);
}

bool AWpLobbyPlayerController::ServerSetColor_Validate(const FColor& Color)
{
	return Color.A == MAX_uint8;
}

void AWpLobbyPlayerController::ServerSetReady_Implementation(const bool bIsReady)
{
	AWpLobbyPlayerState* const InnerPlayerState = GetPlayerState<AWpLobbyPlayerState>();
	InnerPlayerState->SetReady(bIsReady);
}

AWpLobbyPlayerController::FPlayerStateReplicatedEvent& AWpLobbyPlayerController::OnPlayerStateReplicated()
{
	return PlayerStateReplicatedEvent;
}

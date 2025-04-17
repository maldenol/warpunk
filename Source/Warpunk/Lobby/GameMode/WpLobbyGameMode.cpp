#include "WpLobbyGameMode.h"

#include "Warpunk/Global/GameInstance/WpGameInstance.h"
#include "Warpunk/Lobby/GameMode/WpLobbyGameState.h"
#include "Warpunk/Lobby/Player/WpLobbyPlayerController.h"
#include "Warpunk/Lobby/Player/WpLobbyPlayerState.h"
#include "Warpunk/Lobby/UI/WpLobbyHUD.h"

#ifdef UE_EDITOR
const float AWpLobbyGameMode::TimeUntilGameStart = 3.0f;
#else
const float AWpLobbyGameMode::TimeUntilGameStart = 10.0f;
#endif // UE_EDITOR

AWpLobbyGameMode::AWpLobbyGameMode()
{
	PrimaryActorTick.bCanEverTick = false;

	bUseSeamlessTravel = true;
}

void AWpLobbyGameMode::BeginPlay()
{
	Super::BeginPlay();

	ensureAlways(GameStateClass->IsChildOf<AWpLobbyGameState>());
	ensureAlways(PlayerControllerClass->IsChildOf<AWpLobbyPlayerController>());
	ensureAlways(PlayerStateClass->IsChildOf<AWpLobbyPlayerState>());
	ensureAlways(HUDClass->IsChildOf<AWpLobbyHUD>());
}

void AWpLobbyGameMode::OnPostLogin(AController* NewPlayer)
{
	Super::OnPostLogin(NewPlayer);

	AWpLobbyGameState* const InnerGameState = GetGameState<AWpLobbyGameState>();
	InnerGameState->NotifyPlayerLoggedIn(NewPlayer->PlayerState);

	NotifyPlayerStateUpdated();
}

void AWpLobbyGameMode::Logout(AController* Exiting)
{
	AWpLobbyGameState* const InnerGameState = GetGameState<AWpLobbyGameState>();
	InnerGameState->NotifyPlayerLoggedOut(Exiting->PlayerState);

	NotifyPlayerStateUpdated();

	Super::Logout(Exiting);
}

void AWpLobbyGameMode::NotifyPlayerStateUpdated()
{
	for (APlayerState* const PlayerState : GameState->PlayerArray)
	{
		AWpLobbyPlayerState* const InnerPlayerState = Cast<AWpLobbyPlayerState>(PlayerState);
		InnerPlayerState->SetReady(false);
	}
}

void AWpLobbyGameMode::NotifyPlayerReadinessChanged()
{
	bool bAllPlayersAreReady = true;
	for (const APlayerState* const PlayerState : GameState->PlayerArray)
	{
		const AWpLobbyPlayerState* const InnerPlayerState = Cast<AWpLobbyPlayerState>(PlayerState);
		if (!InnerPlayerState->IsReady())
		{
			bAllPlayersAreReady = false;
			break;
		}
	}

	FTimerManager& TimerManager = GetWorld()->GetTimerManager();
	AWpLobbyGameState* const InnerGameState = GetGameState<AWpLobbyGameState>();
	if (bAllPlayersAreReady)
	{
		TimerManager.ClearTimer(StartGameTimerHandle);

		FTimerDelegate StartGameTimerDelegate;
		StartGameTimerDelegate.BindUObject(this, &AWpLobbyGameMode::StartGame);
		TimerManager.SetTimer(StartGameTimerHandle, StartGameTimerDelegate, TimeUntilGameStart, false);

		InnerGameState->SetTimeUntilGameStart(TimeUntilGameStart);
	}
	else
	{
		TimerManager.ClearTimer(StartGameTimerHandle);

		InnerGameState->SetTimeUntilGameStart(-1.0f);
	}
}

void AWpLobbyGameMode::StartGame()
{
	TArray<int32> PlayerIds;
	PlayerIds.Reserve(GameState->PlayerArray.Num());
	for (const APlayerState* const PlayerState : GameState->PlayerArray)
	{
		PlayerIds.Add(PlayerState->GetPlayerId());
	}
	GetGameInstance<UWpGameInstance>()->SetPlayerIds(PlayerIds);

	static const FString Url = TEXT("/Game/Maps/GameMap");
	GetWorld()->ServerTravel(Url);
}

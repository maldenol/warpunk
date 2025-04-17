#include "WpLobbyGameState.h"

#include "Warpunk/Game/GameMode/WpGameGameState.h"

#include "Net/UnrealNetwork.h"

AWpLobbyGameState::AWpLobbyGameState()
{
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
}

void AWpLobbyGameState::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (HasAuthority() && (TimeUntilGameStart > 0.0f))
	{
		SetTimeUntilGameStart(TimeUntilGameStart - DeltaSeconds);
	}
}

void AWpLobbyGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWpLobbyGameState, bTeamsAllowed);
	DOREPLIFETIME(AWpLobbyGameState, MaxPlayerNumber);
	DOREPLIFETIME(AWpLobbyGameState, MaxSpectatorNumber);

	DOREPLIFETIME(AWpLobbyGameState, TimeUntilGameStart);
}

void AWpLobbyGameState::PostRepNotifies()
{
	if (PlayerArray != PreviousPlayerArray)
	{
		for (const APlayerState* PlayerState : PreviousPlayerArray)
		{
			if (!PlayerArray.Contains(PlayerState))
			{
				PlayerLoggedOutEvent.Broadcast(PlayerState);
			}
		}

		for (APlayerState* PlayerState : PlayerArray)
		{
			if (!PreviousPlayerArray.Contains(PlayerState))
			{
				PlayerLoggedInEvent.Broadcast(PlayerState);
			}
		}

		PreviousPlayerArray = PlayerArray;
	}
}

void AWpLobbyGameState::SetTeamsAllowed(const bool bInTeamsAllowed)
{
	bTeamsAllowed = bInTeamsAllowed;
}

bool AWpLobbyGameState::AreTeamsAllowed() const
{
	return bTeamsAllowed;
}

void AWpLobbyGameState::SetMaxPlayerNumber(const uint32 InMaxPlayerNumber)
{
	MaxPlayerNumber = InMaxPlayerNumber;
}

uint32 AWpLobbyGameState::GetMaxPlayerNumber() const
{
	return MaxPlayerNumber;
}

void AWpLobbyGameState::SetMaxSpectatorNumber(const uint32 InMaxSpectatorNumber)
{
	MaxSpectatorNumber = InMaxSpectatorNumber;
}

uint32 AWpLobbyGameState::GetMaxSpectatorNumber() const
{
	return MaxSpectatorNumber;
}

void AWpLobbyGameState::NotifyPlayerLoggedIn(APlayerState* const PlayerState)
{
	PlayerLoggedInEvent.Broadcast(PlayerState);
}

void AWpLobbyGameState::NotifyPlayerLoggedOut(const APlayerState* const PlayerState)
{
	PlayerLoggedOutEvent.Broadcast(PlayerState);
}

void AWpLobbyGameState::SetTimeUntilGameStart(const float InTimeUntilGameStart)
{
	TimeUntilGameStart = InTimeUntilGameStart;
	OnRep_TimeUntilGameStart();
}

AWpLobbyGameState::FLobbySettingsChangedEvent& AWpLobbyGameState::OnLobbySettingsUpdated()
{
	return LobbySettingsChangedEvent;
}

AWpLobbyGameState::FPlayerLoggedInEvent& AWpLobbyGameState::OnPlayerLoggedIn()
{
	return PlayerLoggedInEvent;
}

AWpLobbyGameState::FPlayerLoggedOutEvent& AWpLobbyGameState::OnPlayerLoggedOut()
{
	return PlayerLoggedOutEvent;
}

AWpLobbyGameState::FTimeUntilGameStartChangedEvent& AWpLobbyGameState::OnTimeUntilGameStartUpdated()
{
	return TimeUntilGameStartChangedEvent;
}

void AWpLobbyGameState::OnRep_General()
{
	LobbySettingsChangedEvent.Broadcast();
}

void AWpLobbyGameState::OnRep_TimeUntilGameStart()
{
	TimeUntilGameStartChangedEvent.Broadcast(TimeUntilGameStart);
}

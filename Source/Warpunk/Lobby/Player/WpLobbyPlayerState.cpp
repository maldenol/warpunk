#include "WpLobbyPlayerState.h"

#include "Warpunk/Lobby/GameMode/WpLobbyGameMode.h"
#include "Warpunk/Game/Player/WpGamePlayerState.h"

#include "Net/UnrealNetwork.h"

AWpLobbyPlayerState::AWpLobbyPlayerState()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
}

void AWpLobbyPlayerState::CopyProperties(APlayerState* const PlayerState)
{
	Super::CopyProperties(PlayerState);

	if (AWpGamePlayerState* const GamePlayerState = Cast<AWpGamePlayerState>(PlayerState))
	{
		GamePlayerState->SetName(Name);
		GamePlayerState->SetBelligerentId(BelligerentId);
		GamePlayerState->SetTeamId(TeamId);
		GamePlayerState->SetColor(Color);
	}
	else
	{
		ensureAlways(false);
	}
}

void AWpLobbyPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWpLobbyPlayerState, Name);
	DOREPLIFETIME(AWpLobbyPlayerState, BelligerentId);
	DOREPLIFETIME(AWpLobbyPlayerState, TeamId);
	DOREPLIFETIME(AWpLobbyPlayerState, Color);
	DOREPLIFETIME(AWpLobbyPlayerState, bIsReady);
}

void AWpLobbyPlayerState::SetName(const FString& InName)
{
	Name = InName;
	OnRep_General();
}

const FString& AWpLobbyPlayerState::GetName() const
{
	return Name;
}

void AWpLobbyPlayerState::SetBelligerentId(const uint32 InBelligerentId)
{
	BelligerentId = InBelligerentId;
	OnRep_General();
}

uint32 AWpLobbyPlayerState::GetBelligerentId() const
{
	return BelligerentId;
}

void AWpLobbyPlayerState::SetTeamId(const uint32 InTeamId)
{
	TeamId = InTeamId;
	OnRep_General();
}

uint32 AWpLobbyPlayerState::GetTeamId() const
{
	return TeamId;
}

void AWpLobbyPlayerState::SetColor(const FColor& InColor)
{
	Color = InColor;
	OnRep_General();
}

const FColor& AWpLobbyPlayerState::GetColor() const
{
	return Color;
}

void AWpLobbyPlayerState::SetReady(const bool bInIsReady)
{
	bIsReady = bInIsReady;
	OnRep_bIsReady();
}

bool AWpLobbyPlayerState::IsReady() const
{
	return bIsReady;
}

AWpLobbyPlayerState::FPlayerSettingsChangedEvent& AWpLobbyPlayerState::OnPlayerSettingsUpdated()
{
	return PlayerSettingsChangedEvent;
}

void AWpLobbyPlayerState::OnRep_General()
{
	PlayerSettingsChangedEvent.Broadcast();

	if (AWpLobbyGameMode* const GameMode = GetWorld()->GetAuthGameMode<AWpLobbyGameMode>())
	{
		GameMode->NotifyPlayerStateUpdated();
	}
}

void AWpLobbyPlayerState::OnRep_bIsReady()
{
	PlayerSettingsChangedEvent.Broadcast();

	if (AWpLobbyGameMode* const GameMode = GetWorld()->GetAuthGameMode<AWpLobbyGameMode>())
	{
		GameMode->NotifyPlayerReadinessChanged();
	}
}

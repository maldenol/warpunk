#pragma once

#include "CoreMinimal.h"

#include "GameFramework/GameStateBase.h"

#include "WpLobbyGameState.generated.h"

UCLASS(MinimalAPI, NotBlueprintable, NotBlueprintType)
class AWpLobbyGameState final : public AGameStateBase
{
	GENERATED_BODY()

public:
	DECLARE_EVENT(AWpLobbyGameState, FLobbySettingsChangedEvent);

	DECLARE_EVENT_OneParam(AWpLobbyGameState, FPlayerLoggedInEvent, APlayerState*);

	DECLARE_EVENT_OneParam(AWpLobbyGameState, FPlayerLoggedOutEvent, const APlayerState*);

	DECLARE_EVENT_OneParam(AWpLobbyGameState, FTimeUntilGameStartChangedEvent, float);

	AWpLobbyGameState();

	virtual void Tick(const float DeltaSeconds) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void PostRepNotifies() override;

	void SetTeamsAllowed(const bool bInTeamsAllowed);
	bool AreTeamsAllowed() const;
	void SetMaxPlayerNumber(const uint32 InMaxPlayerNumber);
	uint32 GetMaxPlayerNumber() const;
	void SetMaxSpectatorNumber(const uint32 InMaxSpectatorNumber);
	uint32 GetMaxSpectatorNumber() const;

	void NotifyPlayerLoggedIn(APlayerState* const PlayerState);
	void NotifyPlayerLoggedOut(const APlayerState* const PlayerState);

	void SetTimeUntilGameStart(const float InTimeUntilGameStart);

	FLobbySettingsChangedEvent& OnLobbySettingsUpdated();

	FPlayerLoggedInEvent& OnPlayerLoggedIn();
	FPlayerLoggedOutEvent& OnPlayerLoggedOut();

	FTimeUntilGameStartChangedEvent& OnTimeUntilGameStartUpdated();

private:
	UFUNCTION()
	void OnRep_General();
	UFUNCTION()
	void OnRep_TimeUntilGameStart();

	UPROPERTY(ReplicatedUsing=OnRep_General)
	bool bTeamsAllowed = false;
	UPROPERTY(ReplicatedUsing=OnRep_General)
	uint32 MaxPlayerNumber = 10;
	UPROPERTY(ReplicatedUsing=OnRep_General)
	uint32 MaxSpectatorNumber = 5;

	TArray<TObjectPtr<APlayerState>> PreviousPlayerArray = {};

	UPROPERTY(ReplicatedUsing=OnRep_TimeUntilGameStart)
	float TimeUntilGameStart = -1.0f;

	FLobbySettingsChangedEvent LobbySettingsChangedEvent = {};

	FPlayerLoggedInEvent PlayerLoggedInEvent = {};
	FPlayerLoggedOutEvent PlayerLoggedOutEvent = {};

	FTimeUntilGameStartChangedEvent TimeUntilGameStartChangedEvent = {};
};

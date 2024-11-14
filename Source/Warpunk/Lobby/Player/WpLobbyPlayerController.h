#pragma once

#include "CoreMinimal.h"

#include "GameFramework/PlayerController.h"

#include "WpLobbyPlayerController.generated.h"

UCLASS(MinimalAPI, NotBlueprintable, NotBlueprintType)
class AWpLobbyPlayerController final : public APlayerController
{
	GENERATED_BODY()

public:
	DECLARE_EVENT(AWpLobbyPlayerController, FPlayerStateReplicatedEvent);

	AWpLobbyPlayerController();

	virtual void OnRep_PlayerState() override;

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSetBelligerentId(const uint32 BelligerentId);
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSetTeamId(const uint32 TeamId);
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSetColor(const FColor& Color);
	UFUNCTION(Server, Reliable)
	void ServerSetReady(const bool bIsReady);

	FPlayerStateReplicatedEvent& OnPlayerStateReplicated();

private:
	FPlayerStateReplicatedEvent PlayerStateReplicatedEvent = {};
};

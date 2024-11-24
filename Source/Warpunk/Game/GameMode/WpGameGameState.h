#pragma once

#include "CoreMinimal.h"

#include "GameFramework/GameStateBase.h"

#include "WpGameGameState.generated.h"

USTRUCT(NotBlueprintable, NotBlueprintType)
struct FWpBelligerentTeam
{
	GENERATED_BODY()

	UPROPERTY(VisibleInstanceOnly)
	uint32 BelligerentId = 0;
	UPROPERTY(VisibleInstanceOnly)
	uint32 TeamId = 0;

	bool operator==(const FWpBelligerentTeam& Other) const;
	bool operator==(const uint32 OtherBelligerentId) const;
};

UCLASS(MinimalAPI, Abstract, Blueprintable, NotBlueprintType)
class AWpGameGameState final : public AGameStateBase
{
	GENERATED_BODY()

public:
	DECLARE_EVENT(AWpGameGameState, FBelligerentTeamsReplicatedEvent);

	AWpGameGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void AddToTeam(const uint32 BelligerentId, const uint32 TeamId);
	bool BelongToSameTeam(const uint32 BelligerentId1, const uint32 BelligerentId2) const;

	FBelligerentTeamsReplicatedEvent& OnBelligerentTeamsReplicated();

	const UDataTable* GetEntityConfigs() const;

	UPROPERTY(EditDefaultsOnly)
	uint32 CommanderConfigId = 0;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UUserWidget> EnduranceWidgetClass = nullptr;
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UUserWidget> SelectionWidgetClass = nullptr;

private:
	UFUNCTION()
	void OnRep_BelligerentTeams();

	UPROPERTY(VisibleInstanceOnly, ReplicatedUsing=OnRep_BelligerentTeams)
	TArray<FWpBelligerentTeam> BelligerentTeams = {};

	FBelligerentTeamsReplicatedEvent BelligerentTeamsReplicatedEvent = {};

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UDataTable> EntityConfigs = nullptr;
};

#pragma once

#include "CoreMinimal.h"

#include "GameFramework/PlayerState.h"

#include "WpLobbyPlayerState.generated.h"

UCLASS(MinimalAPI, NotBlueprintable, NotBlueprintType)
class AWpLobbyPlayerState final : public APlayerState
{
	GENERATED_BODY()

public:
	DECLARE_EVENT(AWpLobbyPlayerState, FPlayerSettingsChangedEvent);

	AWpLobbyPlayerState();

	virtual void CopyProperties(APlayerState* const PlayerState) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void SetName(const FString& InName);
	const FString& GetName() const;
	void SetBelligerentId(const uint32 InBelligerentId);
	uint32 GetBelligerentId() const;
	void SetTeamId(const uint32 InTeamId);
	uint32 GetTeamId() const;
	void SetColor(const FColor& InColor);
	const FColor& GetColor() const;
	void SetReady(const bool bInIsReady);
	bool IsReady() const;

	FPlayerSettingsChangedEvent& OnPlayerSettingsUpdated();

private:
	UFUNCTION()
	void OnRep_General();
	UFUNCTION()
	void OnRep_bIsReady();

	UPROPERTY(ReplicatedUsing=OnRep_General)
	FString Name = TEXT("");
	UPROPERTY(ReplicatedUsing=OnRep_General)
	uint32 BelligerentId = 0;
	UPROPERTY(ReplicatedUsing=OnRep_General)
	uint32 TeamId = 0;
	UPROPERTY(ReplicatedUsing=OnRep_General)
	FColor Color = FColor::MakeRandomColor();
	UPROPERTY(ReplicatedUsing=OnRep_bIsReady)
	bool bIsReady = false;

	FPlayerSettingsChangedEvent PlayerSettingsChangedEvent = {};
};

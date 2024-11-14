#pragma once

#include "CoreMinimal.h"

#include "GameFramework/PlayerState.h"

#include "WpGamePlayerState.generated.h"

UCLASS(MinimalAPI, NotBlueprintable, NotBlueprintType)
class AWpGamePlayerState final : public APlayerState
{
	GENERATED_BODY()

public:
	AWpGamePlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void SetName(const FString& InName);
	const FString& GetName() const;
	void SetBelligerentId(const uint32 InBelligerentId);
	uint32 GetBelligerentId() const;
	void SetTeamId(const uint32 InTeamId);
	uint32 GetTeamId() const;
	void SetColor(const FColor& InColor);
	const FColor& GetColor() const;

private:
	UPROPERTY(Replicated)
	FString Name = TEXT("");
	UPROPERTY(Replicated)
	uint32 BelligerentId = 0;
	UPROPERTY(Replicated)
	uint32 TeamId = 0;
	UPROPERTY(Replicated)
	FColor Color = FColor::Transparent;
};

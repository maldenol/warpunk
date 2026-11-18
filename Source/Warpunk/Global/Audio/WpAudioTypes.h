#pragma once

#include "CoreMinimal.h"

#include "WpAudioTypes.generated.h"

UENUM(NotBlueprintable, NotBlueprintType)
enum class EWpSfx : uint8
{
	Projectile,
	Impact,
	HitScan,
	Damage,
	Destruction,
	DestructionBuilding,
	Production,
	ProductionComplete,
	Move,
	Attack,
	Stop,
	Assist,
	Reclaim,
	SelfDestruct,
};

enum class EWpUiSound : uint8
{
	Hover,
	Press,
	Select,
	Deselect,
};

enum class EWpSoundtrack : uint8
{
	None,
	Menu,
	Lobby,
	Game,
};

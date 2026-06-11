#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "NetGameInstance.generated.h"

UENUM(BlueprintType)
enum class EPlayerTeam : uint8
{
    TEAM_Unknown,
    TEAM_Blue,
    TEAM_Red,
};

UENUM(BlueprintType)
enum class EGameResults : uint8
{
    RESULT_Undefined,
    RESULT_Lost,
    RESULT_Won,
};

USTRUCT(BlueprintType)
struct FSPlayerInfo
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Nickname;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString CustomizationData;

    UPROPERTY(BlueprintReadOnly)
    EPlayerTeam TeamID;
};

UCLASS()
class UNetGameInstance : public UGameInstance
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable)
    void Host(FString MapName, FSPlayerInfo Info);

    UFUNCTION(BlueprintCallable)
    void Join(FString Address, FSPlayerInfo Info);

    UPROPERTY(BlueprintReadWrite)
    FSPlayerInfo PlayerInfo;
};
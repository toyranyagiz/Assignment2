#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "NetGameInstance.h"
#include "NetPlayerState.generated.h"

UCLASS()
class ANetPlayerState : public APlayerState
{
    GENERATED_BODY()

public:
    ANetPlayerState();

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_PlayerInfo)
    FSPlayerInfo Data;

    UPROPERTY(BlueprintReadOnly, Replicated)
    int PlayerIndex;

    EPlayerTeam TeamID;
    EGameResults Result;

    UPROPERTY(BlueprintReadOnly, Replicated)
    float ClosestApproachToBlue;

private:
    UFUNCTION()
    void OnRep_PlayerInfo();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
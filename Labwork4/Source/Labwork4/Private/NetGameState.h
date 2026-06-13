#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "NetGameState.generated.h"

class ANetPlayerState;

UCLASS()
class ANetGameState : public AGameStateBase
{
    GENERATED_BODY()

public:
    ANetGameState();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintCallable)
    ANetPlayerState* GetPlayerStateByIndex(int PlayerIndex);

    UFUNCTION(BlueprintImplementableEvent)
    void OnVictory();

    UFUNCTION(BlueprintImplementableEvent)
    void OnRestart();

    UFUNCTION(NetMulticast, Reliable)
    void TriggerRestart();

    UFUNCTION()
    void OnRep_Winner();

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Winner)
    int WinningPlayer;

    UPROPERTY(BlueprintReadOnly, Replicated)
    int32 TimeRemaining;
};
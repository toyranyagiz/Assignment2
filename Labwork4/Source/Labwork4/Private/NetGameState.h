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

    UFUNCTION(BlueprintImplementableEvent)
    void OnVictory();

    UFUNCTION(BlueprintImplementableEvent)
    void OnRestart();

    UFUNCTION(NetMulticast, Reliable)
    void TriggerRestart();

    UFUNCTION(BlueprintCallable)
    ANetPlayerState* GetPlayerStateByIndex(int PlayerIndex);

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Winner)
    int WinningPlayer;

    UPROPERTY(BlueprintReadOnly, Replicated)
    int32 TimeRemaining;

    UFUNCTION()
    void OnRep_Winner();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
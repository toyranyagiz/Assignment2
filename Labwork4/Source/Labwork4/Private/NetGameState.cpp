#include "NetGameState.h"
#include "NetPlayerState.h"
#include "NetBaseCharacter.h"
#include "Net/UnrealNetwork.h"

ANetGameState::ANetGameState()
{
    WinningPlayer = -1;
    TimeRemaining = 0;
}

void ANetGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ANetGameState, WinningPlayer);
    DOREPLIFETIME(ANetGameState, TimeRemaining);
}

void ANetGameState::OnRep_Winner()
{
    const bool bHasWinner = WinningPlayer >= 0;

    if (!bHasWinner)
    {
        return;
    }

    OnVictory();
}

void ANetGameState::TriggerRestart_Implementation()
{
    OnRestart();
}

ANetPlayerState* ANetGameState::GetPlayerStateByIndex(int PlayerIndex)
{
    for (APlayerState* PS : PlayerArray)
    {
        ANetPlayerState* State = Cast<ANetPlayerState>(PS);

        if (!State)
        {
            continue;
        }

        if (State->PlayerIndex == PlayerIndex)
        {
            return State;
        }
    }

    return nullptr;
}
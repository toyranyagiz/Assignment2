#include "NetGameMode.h"
#include "NetBaseCharacter.h"
#include "NetGameState.h"
#include "NetPlayerState.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "Components/CapsuleComponent.h"

ANetGameMode::ANetGameMode()
{
    DefaultPawnClass = ANetBaseCharacter::StaticClass();
    PlayerStateClass = ANetPlayerState::StaticClass();
    GameStateClass = ANetGameState::StaticClass();

    NextBluePlayer = nullptr;
    ConnectedPlayers = 0;
    MatchTime = 30.0f;
}

AActor* ANetGameMode::GetPlayerStart(FString Name, int Index)
{
    FName PSName;

    if (Index < 0)
    {
        PSName = *Name;
    }
    else
    {
        PSName = *FString::Printf(TEXT("%s%d"), *Name, Index % 4);
    }

    for (TActorIterator<APlayerStart> It(GWorld); It; ++It)
    {
        APlayerStart* PS = Cast<APlayerStart>(*It);

        if (!PS)
        {
            continue;
        }

        if (PS->PlayerStartTag == PSName)
        {
            return *It;
        }
    }

    return nullptr;
}

AActor* ANetGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
    AActor* Start = AssignTeamAndPlayerStart(Player);

    if (Start)
    {
        return Start;
    }

    return Super::ChoosePlayerStart_Implementation(Player);
}

AActor* ANetGameMode::AssignTeamAndPlayerStart(AController* Player)
{
    AActor* Start = nullptr;
    ANetPlayerState* State = Player->GetPlayerState<ANetPlayerState>();

    if (!State)
    {
        return Start;
    }

    if (TotalGames == 0)
    {
        if (TotalPlayerCount == 0)
        {
            State->TeamID = EPlayerTeam::TEAM_Blue;
        }
        else
        {
            State->TeamID = EPlayerTeam::TEAM_Red;
        }
    }
    else
    {
        if (State == NextBluePlayer)
        {
            State->TeamID = EPlayerTeam::TEAM_Blue;
        }
        else
        {
            State->TeamID = EPlayerTeam::TEAM_Red;
        }
    }

    State->ClosestApproachToBlue = 550.0f;

    if (State->TeamID == EPlayerTeam::TEAM_Blue)
    {
        Start = GetPlayerStart("Blue", -1);
    }
    else
    {
        Start = GetPlayerStart("Red", PlayerStartIndex++);
    }

    State->PlayerIndex = TotalPlayerCount++;
    AllPlayers.Add(Cast<APlayerController>(Player));

    ConnectedPlayers++;

    ANetGameState* GState = GetGameState<ANetGameState>();

    if (ConnectedPlayers == 1)
    {
        if (GState)
        {
            GState->TimeRemaining = FMath::RoundToInt(MatchTime);
        }
    }
    else if (ConnectedPlayers >= 2)
    {
        GetWorld()->GetTimerManager().SetTimer(
            SwapTimerHandle,
            this,
            &ANetGameMode::Timer,
            1.0f,
            true
        );
    }

    return Start;
}

void ANetGameMode::Timer()
{
    ANetGameState* GState = GetGameState<ANetGameState>();

    if (!GState)
    {
        GetWorld()->GetTimerManager().ClearTimer(SwapTimerHandle);
        return;
    }

    if (GState->WinningPlayer >= 0)
    {
        GetWorld()->GetTimerManager().ClearTimer(SwapTimerHandle);
        return;
    }

    if (GState->TimeRemaining <= 0)
    {
        GState->TimeRemaining = FMath::RoundToInt(MatchTime);
        return;
    }

    GState->TimeRemaining--;

    if (GState->TimeRemaining > 0)
    {
        return;
    }

    GetWorld()->GetTimerManager().ClearTimer(SwapTimerHandle);
    SwapPlayers();
}

void ANetGameMode::AvatarsOverlapped(ANetAvatar* AvatarA, ANetAvatar* AvatarB)
{
    if (!AvatarA || !AvatarB)
    {
        return;
    }

    ANetGameState* GState = GetGameState<ANetGameState>();

    if (!GState)
    {
        return;
    }

    if (GState->WinningPlayer >= 0)
    {
        return;
    }

    ANetPlayerState* StateA = AvatarA->GetPlayerState<ANetPlayerState>();
    ANetPlayerState* StateB = AvatarB->GetPlayerState<ANetPlayerState>();

    if (!StateA || !StateB)
    {
        return;
    }

    if (StateA->TeamID == StateB->TeamID)
    {
        return;
    }

    GetWorld()->GetTimerManager().ClearTimer(SwapTimerHandle);

    ANetPlayerState* WinnerState = nullptr;

    if (StateA->TeamID == EPlayerTeam::TEAM_Red)
    {
        WinnerState = StateA;
    }
    else
    {
        WinnerState = StateB;
    }

    if (WinnerState)
    {
        GState->WinningPlayer = WinnerState->PlayerIndex;
        NextBluePlayer = WinnerState;
    }

    AvatarA->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
    AvatarB->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

    GState->OnVictory();
    TotalGames++;

    FTimerHandle EndGameTimerHandle;

    GWorld->GetTimerManager().SetTimer(
        EndGameTimerHandle,
        this,
        &ANetGameMode::EndGame,
        2.5f,
        false
    );
}

void ANetGameMode::EndGame()
{
    PlayerStartIndex = 0;
    ConnectedPlayers = 0;

    ANetGameState* GState = GetGameState<ANetGameState>();

    if (GState)
    {
        GState->WinningPlayer = -1;
        GState->TimeRemaining = 0;
    }

    for (APlayerController* Player : AllPlayers)
    {
        if (!Player)
        {
            continue;
        }

        APawn* Pawn = Player->GetPawn();

        Player->UnPossess();

        if (Pawn)
        {
            Pawn->Destroy();
        }

        Player->StartSpot.Reset();
        RestartPlayer(Player);
    }

    if (GState)
    {
        GState->TriggerRestart();
    }
}

void ANetGameMode::SwapPlayers()
{
    ANetGameState* GState = GetGameState<ANetGameState>();

    if (!GState)
    {
        return;
    }

    if (GState->WinningPlayer >= 0)
    {
        return;
    }

    TArray<ANetPlayerState*> RedPlayers;
    int BluePlayerIndex = -1;

    for (TActorIterator<ANetAvatar> It(GWorld); It; ++It)
    {
        ANetPlayerState* PS = It->GetPlayerState<ANetPlayerState>();

        if (!PS)
        {
            continue;
        }

        if (PS->TeamID == EPlayerTeam::TEAM_Blue)
        {
            BluePlayerIndex = PS->PlayerIndex;
            continue;
        }

        if (PS->TeamID == EPlayerTeam::TEAM_Red)
        {
            RedPlayers.Add(PS);
        }
    }

    if (BluePlayerIndex != -1)
    {
        GState->WinningPlayer = BluePlayerIndex;
    }

    if (RedPlayers.Num() > 0)
    {
        const int RandomIndex = FMath::RandRange(0, RedPlayers.Num() - 1);
        NextBluePlayer = RedPlayers[RandomIndex];
    }

    GState->OnVictory();
    TotalGames++;

    FTimerHandle EndGameTimerHandle;

    GWorld->GetTimerManager().SetTimer(
        EndGameTimerHandle,
        this,
        &ANetGameMode::EndGame,
        2.5f,
        false
    );
}

void ANetGameMode::ReportDash(ANetAvatar* DashingAvatar, FVector DashStart, FVector DashEnd)
{
    if (!DashingAvatar)
    {
        return;
    }

    ANetPlayerState* DasherState = DashingAvatar->GetPlayerState<ANetPlayerState>();

    if (!DasherState)
    {
        return;
    }

    if (DasherState->TeamID != EPlayerTeam::TEAM_Red)
    {
        return;
    }

    FVector BlueLoc = FVector::ZeroVector;
    bool bFoundBlue = false;

    for (TActorIterator<ANetAvatar> It(GWorld); It; ++It)
    {
        ANetPlayerState* PS = It->GetPlayerState<ANetPlayerState>();

        if (!PS)
        {
            continue;
        }

        if (PS->TeamID != EPlayerTeam::TEAM_Blue)
        {
            continue;
        }

        BlueLoc = It->GetActorLocation();
        bFoundBlue = true;
        break;
    }

    if (!bFoundBlue)
    {
        return;
    }

    FVector ClosestPoint = FMath::ClosestPointOnSegment(BlueLoc, DashStart, DashEnd);
    float Dist = FVector::Dist(BlueLoc, ClosestPoint);

    if (Dist < DasherState->ClosestApproachToBlue)
    {
        DasherState->ClosestApproachToBlue = Dist;
    }
}
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
    FName PSName = Index < 0 ? *Name : *FString::Printf(TEXT("%s%d"), *Name, Index % 4);

    for (TActorIterator<APlayerStart> It(GWorld); It; ++It)
    {
        if (APlayerStart* PS = Cast<APlayerStart>(*It))
        {
            if (PS->PlayerStartTag == PSName) return *It;
        }
    }
    return nullptr;
}

AActor* ANetGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
    AActor* Start = AssignTeamAndPlayerStart(Player);
    return Start ? Start : Super::ChoosePlayerStart_Implementation(Player);
}

AActor* ANetGameMode::AssignTeamAndPlayerStart(AController* Player)
{
    AActor* Start = nullptr;
    ANetPlayerState* State = Player->GetPlayerState<ANetPlayerState>();

    if (!State) return Start;

    if (TotalGames == 0)
        State->TeamID = TotalPlayerCount == 0 ? EPlayerTeam::TEAM_Blue : EPlayerTeam::TEAM_Red;
    else
        State->TeamID = (State == NextBluePlayer) ? EPlayerTeam::TEAM_Blue : EPlayerTeam::TEAM_Red;

    State->ClosestApproachToBlue = 550.0f;
    Start = State->TeamID == EPlayerTeam::TEAM_Blue ? GetPlayerStart("Blue", -1) : GetPlayerStart("Red", PlayerStartIndex++);

    State->PlayerIndex = TotalPlayerCount++;
    AllPlayers.Add(Cast<APlayerController>(Player));

    ConnectedPlayers++;

    if (ConnectedPlayers == 1)
    {
        ANetGameState* GState = GetGameState<ANetGameState>();
        if (GState) GState->TimeRemaining = FMath::RoundToInt(MatchTime);
    }
    else if (ConnectedPlayers >= 2)
    {
        GetWorld()->GetTimerManager().SetTimer(SwapTimerHandle, this, &ANetGameMode::Timer, 1.0f, true);
    }

    return Start;
}

void ANetGameMode::Timer()
{
    ANetGameState* GState = GetGameState<ANetGameState>();

    if (!GState || GState->WinningPlayer >= 0)
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

    if (GState->TimeRemaining <= 0)
    {
        GetWorld()->GetTimerManager().ClearTimer(SwapTimerHandle);
        SwapPlayers();
    }
}

void ANetGameMode::AvatarsOverlapped(ANetAvatar* AvatarA, ANetAvatar* AvatarB)
{
    if (!AvatarA || !AvatarB) return;

    ANetGameState* GState = GetGameState<ANetGameState>();
    if (!GState || GState->WinningPlayer >= 0) return;

    ANetPlayerState* StateA = AvatarA->GetPlayerState<ANetPlayerState>();
    ANetPlayerState* StateB = AvatarB->GetPlayerState<ANetPlayerState>();

    if (!StateA || !StateB) return;
    if (StateA->TeamID == StateB->TeamID) return;

    GetWorld()->GetTimerManager().ClearTimer(SwapTimerHandle);

    if (StateA->TeamID == EPlayerTeam::TEAM_Red)
    {
        GState->WinningPlayer = StateA->PlayerIndex;
        NextBluePlayer = StateA;
    }
    else
    {
        GState->WinningPlayer = StateB->PlayerIndex;
        NextBluePlayer = StateB;
    }

    AvatarA->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
    AvatarB->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

    GState->OnVictory();
    TotalGames++;

    FTimerHandle EndGameTimerHandle;
    GWorld->GetTimerManager().SetTimer(EndGameTimerHandle, this, &ANetGameMode::EndGame, 2.5f, false);
}

void ANetGameMode::EndGame()
{
    PlayerStartIndex = 0;
    ConnectedPlayers = 0;
    GetGameState<ANetGameState>()->WinningPlayer = -1;
    GetGameState<ANetGameState>()->TimeRemaining = 0;

    for (APlayerController* Player : AllPlayers)
    {
        APawn* Pawn = Player->GetPawn();
        Player->UnPossess();
        if (Pawn) Pawn->Destroy();
        Player->StartSpot.Reset();
        RestartPlayer(Player);
    }

    GetGameState<ANetGameState>()->TriggerRestart();
}


//SWAP PLAYERS: Swaps 1 player from red team and 1 player from blue team by random selection

void ANetGameMode::SwapPlayers()
{
    ANetGameState* GState = GetGameState<ANetGameState>();
    if (!GState || GState->WinningPlayer >= 0) return;

    TArray<ANetPlayerState*> RedPlayers;
    int BluePlayerIndex = -1;

    for (TActorIterator<ANetAvatar> It(GWorld); It; ++It)
    {
        ANetPlayerState* PS = It->GetPlayerState<ANetPlayerState>();
        if (!PS) continue;

        if (PS->TeamID == EPlayerTeam::TEAM_Blue)
            BluePlayerIndex = PS->PlayerIndex;
        else if (PS->TeamID == EPlayerTeam::TEAM_Red)
            RedPlayers.Add(PS);
    }

    if (BluePlayerIndex != -1)
        GState->WinningPlayer = BluePlayerIndex;

    if (RedPlayers.Num() > 0)
        NextBluePlayer = RedPlayers[FMath::RandRange(0, RedPlayers.Num() - 1)];

    GState->OnVictory();
    TotalGames++;

    FTimerHandle EndGameTimerHandle;
    GWorld->GetTimerManager().SetTimer(EndGameTimerHandle, this, &ANetGameMode::EndGame, 2.5f, false);
}

void ANetGameMode::ReportDash(ANetAvatar* DashingAvatar, FVector DashStart, FVector DashEnd)
{
    if (!DashingAvatar) return;

    ANetPlayerState* DasherState = DashingAvatar->GetPlayerState<ANetPlayerState>();
    if (!DasherState || DasherState->TeamID != EPlayerTeam::TEAM_Red) return;

    FVector BlueLoc = FVector::ZeroVector;
    bool bFoundBlue = false;

    for (TActorIterator<ANetAvatar> It(GWorld); It; ++It)
    {
        ANetPlayerState* PS = It->GetPlayerState<ANetPlayerState>();
        if (PS && PS->TeamID == EPlayerTeam::TEAM_Blue)
        {
            BlueLoc = It->GetActorLocation();
            bFoundBlue = true;
            break;
        }
    }

    if (!bFoundBlue) return;

    FVector ClosestPoint = FMath::ClosestPointOnSegment(BlueLoc, DashStart, DashEnd);
    float Dist = FVector::Dist(BlueLoc, ClosestPoint);

    if (Dist < DasherState->ClosestApproachToBlue)
        DasherState->ClosestApproachToBlue = Dist;
}
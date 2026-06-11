#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "NetAvatar.h"
#include "NetGameMode.generated.h"

class ANetPlayerState;

UCLASS()
class ANetGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ANetGameMode();

    virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

    UFUNCTION(BlueprintCallable)
    void AvatarsOverlapped(ANetAvatar* AvatarA, ANetAvatar* AvatarB);

    UFUNCTION(BlueprintCallable)
    void EndGame();

    UFUNCTION(BlueprintCallable)
    void ReportDash(ANetAvatar* DashingAvatar, FVector DashStart, FVector DashEnd);

    UFUNCTION(BlueprintCallable)
    void SwapPlayers();

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Game Rules")
    float MatchTime;

private:
    int TotalPlayerCount;
    int TotalGames;
    int PlayerStartIndex;
    int ConnectedPlayers;

    FTimerHandle SwapTimerHandle;
    TArray<APlayerController*> AllPlayers;

    UPROPERTY()
    ANetPlayerState* NextBluePlayer;

    AActor* GetPlayerStart(FString Name, int Index);
    AActor* AssignTeamAndPlayerStart(AController* Player);
    void Timer();
};
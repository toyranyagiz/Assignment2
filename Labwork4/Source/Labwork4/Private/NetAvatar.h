#pragma once

#include "CoreMinimal.h"
#include "NetBaseCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "NetAvatar.generated.h"

UCLASS()
class ANetAvatar : public ANetBaseCharacter
{
    GENERATED_BODY()

public:
    ANetAvatar();

    UPROPERTY(EditAnywhere)
    UCameraComponent* Camera;

    UPROPERTY(EditAnywhere)
    USpringArmComponent* SpringArm;

    virtual void BeginPlay() override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UPROPERTY(BlueprintReadWrite)
    float MovementScale;

    UPROPERTY(ReplicatedUsing = OnRep_IsRunning)
    bool bIsRunning;

    UFUNCTION()
    void OnRep_IsRunning();

    UFUNCTION(Server, Reliable)
    void ServerSetRunning(bool bRunning);

    void RunPressed();
    void RunReleased();

private:
    void MoveForward(float Scale);
    void MoveRight(float Scale);
};
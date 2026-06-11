#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Runtime/Engine/Classes/Engine/DataTable.h"
#include "NetGameInstance.h"
#include "Net/UnrealNetwork.h"
#include "NetBaseCharacter.generated.h"

UENUM(BlueprintType)
enum class EBodyPart : uint8
{
    BP_Face = 0,
    BP_Hair = 1,
    BP_Chest = 2,
    BP_Hands = 3,
    BP_Legs = 4,
    BP_Beard = 5,
    BP_Eyebrows = 6,
    BP_BodyType = 7,
    BP_COUNT = 8,
};

USTRUCT(BlueprintType)
struct FSMeshAssetList : public FTableRowBase
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<USkeletalMesh*> ListSkeletal;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<UStaticMesh*> ListStatic;
};

UCLASS()
class ANetBaseCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    ANetBaseCharacter();

    virtual void BeginPlay() override;
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void Tick(float DeltaTime) override;

    UFUNCTION(BlueprintPure)
    FString GetCustomizationData();

    void ParseCustomizationData(FString Data);

    UFUNCTION(BlueprintCallable)
    void ChangeBodyPart(EBodyPart index, int value, bool DirectSet);

    UFUNCTION(BlueprintCallable)
    void ChangeGender(bool isFemale);

    UFUNCTION(Server, Reliable)
    void SubmitPlayerInfoToServer(FSPlayerInfo Info);

    UFUNCTION(BlueprintImplementableEvent)
    void OnPlayerInfoChanged();

    UFUNCTION()
    void CheckPlayerState();

    UFUNCTION()
    void CheckPlayerInfo();

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    USkeletalMeshComponent* PartFace;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UStaticMeshComponent* PartHair;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UStaticMeshComponent* PartBeard;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UStaticMeshComponent* PartEyes;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UStaticMeshComponent* PartEyebrows;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    USkeletalMeshComponent* PartHands;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    USkeletalMeshComponent* PartLegs;

    bool PlayerInfoReceived;

private:
    int BodyPartIndices[(int)EBodyPart::BP_COUNT];

    static FSMeshAssetList* GetBodyPartList(EBodyPart part, bool isFemale);
    void UpdateBodyParts();

    FTimerHandle ClientDataCheckTimer;
};
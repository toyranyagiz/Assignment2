#include "NetBaseCharacter.h"
#include "NetGameInstance.h"
#include "NetPlayerState.h"

static UDataTable* SBodyParts = nullptr;

static const TArray<FString> BodyPartNames = {
    TEXT("Face"),
    TEXT("Hair"),
    TEXT("Chest"),
    TEXT("Hands"),
    TEXT("Legs"),
    TEXT("Beard"),
    TEXT("Eyebrows"),
    TEXT("BodyType")
};

ANetBaseCharacter::ANetBaseCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    PartFace = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Face"));
    PartFace->SetupAttachment(GetMesh());
    PartFace->SetLeaderPoseComponent(GetMesh());

    PartHands = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Hands"));
    PartHands->SetupAttachment(GetMesh());
    PartHands->SetLeaderPoseComponent(GetMesh());

    PartLegs = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Legs"));
    PartLegs->SetupAttachment(GetMesh());
    PartLegs->SetLeaderPoseComponent(GetMesh());

    PartHair = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Hair"));
    PartHair->SetupAttachment(PartFace, FName("headSocket"));

    PartBeard = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Beard"));
    PartBeard->SetupAttachment(PartFace, FName("headSocket"));

    PartEyes = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Eyes"));
    PartEyes->SetupAttachment(PartFace, FName("headSocket"));

    PartEyebrows = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Eyebrows"));
    PartEyebrows->SetupAttachment(PartFace, FName("headSocket"));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> SK_Eyes(TEXT("StaticMesh'/Game/StylizedModularChar/Meshes/SM_Eyes.SM_Eyes'"));
    PartEyes->SetStaticMesh(SK_Eyes.Object);

    static ConstructorHelpers::FObjectFinder<UDataTable> DT_BodyParts(TEXT("/Script/Engine.DataTable'/Game/Blueprints/Resources/DT_BodyParts.DT_BodyParts'"));
    SBodyParts = DT_BodyParts.Object;

    PlayerInfoReceived = false;
}

void ANetBaseCharacter::BeginPlay()
{
    Super::BeginPlay();

    if (GetNetMode() == ENetMode::NM_Standalone) return;

    SetActorHiddenInGame(true);
    CheckPlayerState();
}

void ANetBaseCharacter::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    UpdateBodyParts();
}

void ANetBaseCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void ANetBaseCharacter::ChangeGender(bool _isFemale)
{
    BodyPartIndices[(int)EBodyPart::BP_BodyType] = _isFemale ? 1 : 0;
    UpdateBodyParts();
}

void ANetBaseCharacter::UpdateBodyParts()
{
    ChangeBodyPart(EBodyPart::BP_Face, 0, false);
    ChangeBodyPart(EBodyPart::BP_Chest, 0, false);
    ChangeBodyPart(EBodyPart::BP_Hair, 0, false);
    ChangeBodyPart(EBodyPart::BP_Hands, 0, false);
    ChangeBodyPart(EBodyPart::BP_Legs, 0, false);
    ChangeBodyPart(EBodyPart::BP_Eyebrows, 0, false);

    bool isFemale = BodyPartIndices[(int)EBodyPart::BP_BodyType] != 0;
    if (isFemale)
    {
        PartBeard->SetStaticMesh(nullptr);
    }
    else
    {
        ChangeBodyPart(EBodyPart::BP_Beard, 0, false);
    }
}

FSMeshAssetList* ANetBaseCharacter::GetBodyPartList(EBodyPart part, bool isFemale)
{
    if (part == EBodyPart::BP_BodyType) return nullptr;

    FString Name = FString::Printf(TEXT("%s%s"), isFemale ? TEXT("Female") : TEXT("Male"), *BodyPartNames[(int)part]);
    return SBodyParts ? SBodyParts->FindRow<FSMeshAssetList>(*Name, TEXT("")) : nullptr;
}

void ANetBaseCharacter::ChangeBodyPart(EBodyPart index, int value, bool DirectSet)
{
    bool isFemale = BodyPartIndices[(int)EBodyPart::BP_BodyType] != 0;
    if (index == EBodyPart::BP_Beard && isFemale) return;

    FSMeshAssetList* List = GetBodyPartList(index, isFemale);
    if (List == nullptr) return;

    int CurrentIndex = BodyPartIndices[(int)index];

    if (DirectSet) {
        CurrentIndex = value;
    }
    else {
        CurrentIndex += value;
        int Num = List->ListSkeletal.Num() + List->ListStatic.Num();

        if (CurrentIndex < 0)
            CurrentIndex += Num;
        else
            CurrentIndex %= Num;
    }

    BodyPartIndices[(int)index] = CurrentIndex;

    switch (index)
    {
    case EBodyPart::BP_Face: PartFace->SetSkeletalMeshAsset(List->ListSkeletal[CurrentIndex]); break;
    case EBodyPart::BP_Beard: PartBeard->SetStaticMesh(List->ListStatic[CurrentIndex]); break;
    case EBodyPart::BP_Chest: GetMesh()->SetSkeletalMeshAsset(List->ListSkeletal[CurrentIndex]); break;
    case EBodyPart::BP_Hair: PartHair->SetStaticMesh(List->ListStatic[CurrentIndex]); break;
    case EBodyPart::BP_Hands: PartHands->SetSkeletalMeshAsset(List->ListSkeletal[CurrentIndex]); break;
    case EBodyPart::BP_Legs: PartLegs->SetSkeletalMeshAsset(List->ListSkeletal[CurrentIndex]); break;
    case EBodyPart::BP_Eyebrows: PartEyebrows->SetStaticMesh(List->ListStatic[CurrentIndex]); break;
    }
}

void ANetBaseCharacter::SubmitPlayerInfoToServer_Implementation(FSPlayerInfo Info)
{
    ANetPlayerState* State = GetPlayerState<ANetPlayerState>();
    if (State) {
        State->Data.Nickname = Info.Nickname;
        State->Data.CustomizationData = Info.CustomizationData;
        State->Data.TeamID = State->TeamID;
        PlayerInfoReceived = true;
    }
}

void ANetBaseCharacter::CheckPlayerState()
{
    ANetPlayerState* State = GetPlayerState<ANetPlayerState>();
    if (State == nullptr) {
        UE_LOG(LogTemp, Warning, TEXT("State == nullptr"));
        GWorld->GetTimerManager().SetTimer(ClientDataCheckTimer, this, &ANetBaseCharacter::CheckPlayerState, 0.25f, false);
    }
    else {
        if (IsLocallyControlled())
        {
            UNetGameInstance* Instance = Cast<UNetGameInstance>(GWorld->GetGameInstance());
            if (Instance) {
                SubmitPlayerInfoToServer(Instance->PlayerInfo);
            }
        }
        CheckPlayerInfo();
    }
}

void ANetBaseCharacter::CheckPlayerInfo()
{
    ANetPlayerState* State = GetPlayerState<ANetPlayerState>();
    if (State && PlayerInfoReceived) {
        ParseCustomizationData(State->Data.CustomizationData);
        UpdateBodyParts();
        OnPlayerInfoChanged();
        SetActorHiddenInGame(false);
    }
    else {
        UE_LOG(LogTemp, Warning, TEXT("State Not Received!"));
        GWorld->GetTimerManager().SetTimer(ClientDataCheckTimer, this, &ANetBaseCharacter::CheckPlayerInfo, 0.25f, false);
    }
}

FString ANetBaseCharacter::GetCustomizationData()
{
    FString Data;
    for (size_t i = 0; i < (int)EBodyPart::BP_COUNT; i++)
    {
        Data += FString::FromInt(BodyPartIndices[i]);
        if (i < ((int)(EBodyPart::BP_COUNT)-1)) Data += TEXT(",");
    }
    return Data;
}

void ANetBaseCharacter::ParseCustomizationData(FString BodyPartData)
{
    TArray<FString> ArrayData;
    BodyPartData.ParseIntoArray(ArrayData, TEXT(","));

    for (size_t i = 0; i < ArrayData.Num(); i++)
    {
        BodyPartIndices[i] = FCString::Atoi(*ArrayData[i]);
    }
}
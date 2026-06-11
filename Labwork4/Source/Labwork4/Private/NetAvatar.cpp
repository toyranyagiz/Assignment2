#include "NetAvatar.h"
#include "GameFramework/CharacterMovementComponent.h"

ANetAvatar::ANetAvatar()
{
    MovementScale = 1.0f;

    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    SpringArm->SetupAttachment(RootComponent);

    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
}

void ANetAvatar::BeginPlay()
{
    Super::BeginPlay();
    Camera->bUsePawnControlRotation = false;
    SpringArm->bUsePawnControlRotation = true;
    bUseControllerRotationYaw = false;
    GetCharacterMovement()->bOrientRotationToMovement = true;
}

void ANetAvatar::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    PlayerInputComponent->BindAxis("Turn", this, &ACharacter::AddControllerYawInput);
    PlayerInputComponent->BindAxis("LookUp", this, &ACharacter::AddControllerPitchInput);
    PlayerInputComponent->BindAxis("MoveForward", this, &ANetAvatar::MoveForward);
    PlayerInputComponent->BindAxis("MoveRight", this, &ANetAvatar::MoveRight);

    PlayerInputComponent->BindAction("Run", IE_Pressed, this, &ANetAvatar::RunPressed);
    PlayerInputComponent->BindAction("Run", IE_Released, this, &ANetAvatar::RunReleased);
}

void ANetAvatar::MoveForward(float Scale)
{
    FRotator Rotation = GetController()->GetControlRotation();
    FRotator YawRotation(0.0f, Rotation.Yaw, 0.0f);
    FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
    AddMovementInput(ForwardDirection, Scale * MovementScale);
}

void ANetAvatar::MoveRight(float Scale)
{
    FRotator Rotation = GetController()->GetControlRotation();
    FRotator YawRotation(0.0f, Rotation.Yaw, 0.0f);
    FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
    AddMovementInput(RightDirection, Scale * MovementScale);
}

void ANetAvatar::RunPressed()
{
    if (IsLocallyControlled())
    {
        ServerSetRunning(true);
    }
}

void ANetAvatar::RunReleased()
{
    if (IsLocallyControlled())
    {
        ServerSetRunning(false);
    }
}

void ANetAvatar::ServerSetRunning_Implementation(bool bRunning)
{
    bIsRunning = bRunning;
    OnRep_IsRunning();
}

void ANetAvatar::OnRep_IsRunning()
{
    if (bIsRunning)
    {
        GetCharacterMovement()->MaxWalkSpeed = 600.0f;
    }
    else
    {
        GetCharacterMovement()->MaxWalkSpeed = 300.0f;
    }
}

void ANetAvatar::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ANetAvatar, bIsRunning);
}
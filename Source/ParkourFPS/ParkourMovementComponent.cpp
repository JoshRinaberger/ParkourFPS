// Fill out your copyright notice in the Description page of Project Settings.


#include "ParkourMovementComponent.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/InputSettings.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "ParkourFPSCharacter.h"

DEFINE_LOG_CATEGORY(LogMovementCorrections);
DEFINE_LOG_CATEGORY(LogParkourMovement);

void UParkourMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	// Only characters who's roles are autonomous proxy and authority should check their own collision
	if (GetPawnOwner()->GetLocalRole() > ROLE_SimulatedProxy) {
		// Bind to the OnActorHot component so we're notified when the owning actor hits something (like a wall)
		GetPawnOwner()->OnActorHit.AddDynamic(this, &UParkourMovementComponent::OnActorHit);
	}
}

void UParkourMovementComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	if (GetPawnOwner() != nullptr && GetPawnOwner()->GetLocalRole() > ROLE_SimulatedProxy)
	{
		// Unbind from all events
		GetPawnOwner()->OnActorHit.RemoveDynamic(this, &UParkourMovementComponent::OnActorHit);
	}

	Super::OnComponentDestroyed(bDestroyingHierarchy);
}

void UParkourMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	// Only perform checks if the character is controlled from this client
	if (GetPawnOwner()->IsLocallyControlled())
	{
		WantsToWallRun = MovementKey1Down;
	}
}

void UParkourMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{

}

void UParkourMovementComponent::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
	if (WantsToWallRun && IsWallRunning && !IsCustomMovementMode(ECustomMovementMode::CMOVE_WallRunning))
	{
		SetMovementMode(EMovementMode::MOVE_Custom, ECustomMovementMode::CMOVE_WallRunning);
	}

	Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);
}

void UParkourMovementComponent::OnActorHit(AActor* SelfActor, AActor* OtherActor, FVector NormalImpulse, const FHitResult& Hit)
{
	// return if a custom move is already being performed
	if (MovementMode == EMovementMode::MOVE_Custom)
	{
		return;
	}

	// Runs checks after hitting a potential wall and begins wall running if the checks are passed
	// If wall run can happen and starts then no need to check further actions
	if (CheckCanWallRun(Hit))
	{
		return;
	}
}

bool UParkourMovementComponent::CheckCanWallRun(const FHitResult Hit)
{
	// No need to check further if the character is already wallrunning
	if (IsCustomMovementMode(ECustomMovementMode::CMOVE_WallRunning))
	{
		return false;
	}

	// Make sure that the character is falling.
	// Wall running can only begin if the character is currently in the air.
	if (IsFalling() == false)
	{
		return false;
	}

	// Make sure the surface can be wall ran based on the angle of the surface that the character hits
	if (CanSurfaceBeWallRan(Hit.ImpactNormal) == false)
	{
		return false;
	}

	FindWallRunSide(Hit.ImpactNormal);

	// Make sure that the character is next to a wall
	if (IsNextToWall() == false)
	{
		return false;
	}

	BeginWallRun();

	return true;
}

bool UParkourMovementComponent::CheckWallRunFloor()
{
	//looks to see if the player is on the floor while wall running and returns false if the player is currently standing on a floor,
	//as the wall run should end when the player hits the floor

	FFindFloorResult FloorResult;
	FHitResult* HitResult = NULL;

	FindFloor(CharacterOwner->GetActorLocation(), FloorResult, false, HitResult);

	if (FloorResult.bWalkableFloor == false)
	{
		return true;
	}

	if (FloorResult.FloorDist < 0.3)
	{
		return false;
	}

	return true;
}

bool UParkourMovementComponent::CheckWallRunTraces()
{
	return true;
}

FVector UParkourMovementComponent::GetWallRunEndVectorL()
{
	FVector EndVector = CharacterOwner->GetActorLocation();
	EndVector += (CharacterOwner->GetActorRightVector()) * (-75.0);
	EndVector += (CharacterOwner->GetActorForwardVector()) * (-35.0);

	return EndVector;
}

FVector UParkourMovementComponent::GetWallRunEndVectorR()
{
	FVector EndVector = CharacterOwner->GetActorLocation();
	EndVector += (CharacterOwner->GetActorRightVector()) * (75.0);
	EndVector += (CharacterOwner->GetActorForwardVector()) * (-35.0);

	return EndVector;
}

bool UParkourMovementComponent::IsValidWallRunVector(FVector InVec, bool SaveVector)
{
	float InZ = InVec.Z;

	if (InZ < 0.52 && InZ > -0.52)
	{
		if (SaveVector)
		{
			WallRunNormal = InVec;
		}

		return true;
	}
	else
	{
		return false;
	}
}

FVector UParkourMovementComponent::PlayerToWallVector()
{
	FVector WallVector = WallRunNormal;
	WallVector *= (CharacterOwner->GetActorLocation() * WallRunNormal).Size();

	return WallVector;
}

bool UParkourMovementComponent::IsNextToWall(float vertical_tolerance)
{
	return true;
}

bool UParkourMovementComponent::CanSurfaceBeWallRan(const FVector& surface_normal) const
{
	return true;
}

int UParkourMovementComponent::FindWallRunSide(const FVector& surface_normal)
{
	return 0;
}

bool UParkourMovementComponent::BeginWallRun()
{
	return true;
}

void UParkourMovementComponent::EndWallRun()
{

}

void UParkourMovementComponent::OnClientCorrectionReceived(class FNetworkPredictionData_Client_Character& ClientData, float TimeStamp, FVector NewLocation, FVector NewVelocity,
	UPrimitiveComponent* NewBase, FName NewBaseBoneName, bool bHasBase, bool bBaseRelativePosition, uint8 ServerMovementMode) 
{

}

FNetworkPredictionData_Client* UParkourMovementComponent::GetPredictionData_Client() const
{
	if (ClientPredictionData == nullptr)
	{
		// Return our custom client prediction class instead
		UParkourMovementComponent* MutableThis = const_cast<UParkourMovementComponent*>(this);
		MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_My(*this);
	}

	return ClientPredictionData;
}

void UParkourMovementComponent::PhysCustom(float deltaTime, int32 Iterations)
{

}

void UParkourMovementComponent::PhysWallRun(float deltaTime, int32 Iterations)
{

}

void UParkourMovementComponent::SetMovementKey1Down(bool KeyIsDown)
{
	MovementKey1Down = KeyIsDown;
}

void UParkourMovementComponent::SetMovementKey2Down(bool KeyIsDown)
{
	MovementKey2Down = KeyIsDown;
}

void UParkourMovementComponent::SetMovementKey3Down(bool KeyIsDown)
{
	MovementKey3Down = KeyIsDown;
}

bool UParkourMovementComponent::IsCustomMovementMode(uint8 custom_movement_mode) const
{
	return MovementMode == EMovementMode::MOVE_Custom && CustomMovementMode == custom_movement_mode;
}

void FSavedMove_My::Clear()
{

}

uint8 FSavedMove_My::GetCompressedFlags() const
{
	uint8 Result = Super::GetCompressedFlags();

	/* There are 4 custom move flags for us to use. Below is what each is currently being used for:
	FLAG_Custom_0		= 0x10, // WallRunning
	FLAG_Custom_1		= 0x20, // ???
	FLAG_Custom_2		= 0x40, // ???
	FLAG_Custom_3		= 0x80, // ???
	*/

	return Result;
}

bool FSavedMove_My::CanCombineWith(const FSavedMovePtr& NewMovePtr, ACharacter* Character, float MaxDelta) const
{
	return Super::CanCombineWith(NewMovePtr, Character, MaxDelta);
}

void FSavedMove_My::SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAccel, class FNetworkPredictionData_Client_Character& ClientData)
{

}

void FSavedMove_My::PrepMoveFor(class ACharacter* Character)
{

}

FNetworkPredictionData_Client_My::FNetworkPredictionData_Client_My(const UCharacterMovementComponent& ClientMovement)
	: Super(ClientMovement)
{

}

FSavedMovePtr FNetworkPredictionData_Client_My::AllocateNewMove()
{
	return FSavedMovePtr(new FSavedMove_My());
}

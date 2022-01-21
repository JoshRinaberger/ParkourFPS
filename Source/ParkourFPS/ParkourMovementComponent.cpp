// Fill out your copyright notice in the Description page of Project Settings.


#include "ParkourMovementComponent.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/InputSettings.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "ParkourFPSCharacter.h"

void UParkourMovementComponent::BeginPlay()
{

}

void UParkourMovementComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{

}

void UParkourMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{

}

void UParkourMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{

}

void UParkourMovementComponent::PhysCustom(float deltaTime, int32 Iterations)
{

}

void UParkourMovementComponent::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
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

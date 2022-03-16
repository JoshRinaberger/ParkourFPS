// Fill out your copyright notice in the Description page of Project Settings.


#include "ParkourMovementComponent.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/InputSettings.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "ParkourFPSCharacter.h"
#include "DrawDebugHelpers.h"
#include "Zipline.h"
#include "Ladder.h"

DEFINE_LOG_CATEGORY(LogMovementCorrections);
DEFINE_LOG_CATEGORY(LogParkourMovement);

UParkourMovementComponent::UParkourMovementComponent(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{

}

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

		if (MovementKey2Down)
		{
			WantsToSlide = (CheckCanSlide());
		}
		else {
			WantsToSlide = false;
		}

		WantsToVerticalWallRun = MovementKey3Down;
	}

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

AParkourFPSCharacter* UParkourMovementComponent::GetParkourFPSCharacter()
{
	return static_cast<AParkourFPSCharacter*>(GetCharacterOwner());
}

void UParkourMovementComponent::SetCameraRotationLimit(float MinPitch, float MaxPitch, float MinRoll, float MaxRoll, float MinYaw, float MaxYaw)
{
	if (GetPawnOwner()->IsLocallyControlled() == false)
	{
		return;
	}

	APlayerController* LocalPlayerController = GetWorld()->GetFirstPlayerController();

	if (LocalPlayerController)
	{
		APlayerCameraManager* LocalCameraManager = LocalPlayerController->PlayerCameraManager;

		if (LocalCameraManager)
		{
			LocalCameraManager->ViewPitchMin = MinPitch;
			LocalCameraManager->ViewPitchMax = MaxPitch;
			LocalCameraManager->ViewRollMin = MinRoll;
			LocalCameraManager->ViewRollMax = MaxRoll;
			LocalCameraManager->ViewYawMin = MinYaw;
			LocalCameraManager->ViewYawMax = MaxYaw;
		}
	}
}

void UParkourMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);

	/*  There are 4 custom move flags for us to use. Below is what each is currently being used for:
		FLAG_Custom_0		= 0x10, // Wall Run
		FLAG_Custom_1		= 0x20, // Slide
		FLAG_Custom_2		= 0x40, // ???
		FLAG_Custom_3		= 0x80, // ???
	*/

	// Read the values from the compressed flags
	WantsToWallRun = (Flags & FSavedMove_Character::FLAG_Custom_0) != 0;
	WantsToSlide = (Flags & FSavedMove_Character::FLAG_Custom_1) != 0;
	WantsToVerticalWallRun = (Flags & FSavedMove_Character::FLAG_Custom_2) != 0;
	WantsToZiplineLadder = (Flags & FSavedMove_Character::FLAG_Custom_3) != 0;
}

void UParkourMovementComponent::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
	if (WantsToWallRun && IsWallRunning && !IsCustomMovementMode(ECustomMovementMode::CMOVE_WallRunning))
	{
		SetMovementMode(EMovementMode::MOVE_Custom, ECustomMovementMode::CMOVE_WallRunning);
	}

	if (WantsToVerticalWallRun && IsVerticalWallRunning && !IsCustomMovementMode(ECustomMovementMode::CMOVE_VerticalWallRunning))
	{
		SetMovementMode(EMovementMode::MOVE_Custom, ECustomMovementMode::CMOVE_VerticalWallRunning);
	}

	if (WantsToSlide && !IsCrouched && !IsSliding)
	{
		BeginSlide();
	}

	if (WantsToZiplineLadder && IsZiplining)
	{
		SetMovementMode(EMovementMode::MOVE_Custom, ECustomMovementMode::CMOVE_Ziplining);
	}

	if (WantsToZiplineLadder && IsClimbingLadder)
	{
		SetMovementMode(EMovementMode::MOVE_Custom, ECustomMovementMode::CMOVE_ClimbLadder);
	}

	Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);
}

void UParkourMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	if (PreviousMovementMode != MovementMode || PreviousCustomMode != CustomMovementMode)
	{
		UE_LOG(LogParkourMovement, Warning, TEXT("Movement Mode Changed To:  %i %s"), MovementMode, *CharacterOwner->GetName());

		if (MovementMode == EMovementMode::MOVE_Custom)
			UE_LOG(LogParkourMovement, Warning, TEXT("Custom Movement Mode Changed To: %i %s"), CustomMovementMode, *CharacterOwner->GetName());
	}

	if (MovementMode == MOVE_Custom)
	{
		switch (CustomMovementMode)
		{
		case ECustomMovementMode::CMOVE_WallRunning:
		{
			if (Velocity.Z > 0.f)
			{
				Velocity.Z = 300;
			}
			else
			{
				Velocity.Z = Velocity.Z / 5;
			}

			break;
		}
		case ECustomMovementMode::CMOVE_VerticalWallRunning:
		{
			SetVerticalWallRunVelocity(VerticalWallRunStartSpeed);

			break;
		}
		case ECustomMovementMode::CMOVE_Ziplining:
		{
			Velocity = ZiplineStartSpeed * ZiplineDirection;

			break;
		}
		}
	}

	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);
}

void UParkourMovementComponent::OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity)
{
	Super::OnMovementUpdated(DeltaSeconds, OldLocation, OldVelocity);

	if (PawnOwner->GetLocalRole() == ROLE_AutonomousProxy)
	{
		if (MovementMode == EMovementMode::MOVE_Custom)
		{
			ServerSetWantsToCustomJump(WantsToCustomJump);
		}

		if (IsVerticalWallRunning)
		{
			ServerSetWantsToVerticalWallRunRotate(WantsToVerticalWallRunRotate);
		}
		
		if (IsClimbingLadder)
		{
			ServerSetWantsToGoUpLadder(WantsToClimbLadderUp);
			ServerSetWantsToGoDownLadder(WantsToClimbLadderDown);
		}
		if (IsLedgeHanging)
		{
			ServerSetWantsToStopLedgeHang(WantsToStopLedgeHang);
		}
	}

	DoCustomJump();

	ApplySlideForce();

	SetVerticalWallRunRotation();

	ApplyVerticalWallRunRotation();

	if (IsVerticalWallRunning && CheckCanHangLedge())
	{
		EndVerticalWallRun();
		BeginLedgeHang();
	}

	if (IsLedgeHanging && WantsToStopLedgeHang)
	{
		EndLedgeHang();
	}
}

void UParkourMovementComponent::OnActorHit(AActor* SelfActor, AActor* OtherActor, FVector NormalImpulse, const FHitResult& Hit)
{
	if (GetPawnOwner()->GetLocalRole() <= ROLE_SimulatedProxy)
	{
		return;
	}

	bool LedgeHang = CheckCanHangLedge();

	if (LedgeHang)
	{
		UE_LOG(LogParkourMovement, Warning, TEXT("Actor Hit Ledge Grab TRUE"));
	}
	else
	{
		UE_LOG(LogParkourMovement, Warning, TEXT("Actor Hit Ledge Grab FALSE"));
	}

	bool LedgeClimb = CheckCanClimb();

	if (LedgeClimb)
	{
		UE_LOG(LogParkourMovement, Warning, TEXT("Actor Hit Ledge Climb TRUE"));
	}
	else
	{
		UE_LOG(LogParkourMovement, Warning, TEXT("Actor Hit Ledge Climb FALSE"));
	}

	bool LedgeQuickClimb = CheckCanQuickClimb();

	if (LedgeQuickClimb)
	{
		UE_LOG(LogParkourMovement, Warning, TEXT("Actor Hit Ledge Quick Climb TRUE"));

		bool LedgeVault = CheckCanVault();

		if (LedgeVault)
		{
			UE_LOG(LogParkourMovement, Warning, TEXT("Actor Hit Ledge Vault TRUE"));
		}
		else
		{
			UE_LOG(LogParkourMovement, Warning, TEXT("Actor Hit Ledge Vault FALSE"));
		}
	}
	else
	{
		UE_LOG(LogParkourMovement, Warning, TEXT("Actor Hit Ledge Quick Climb FALSE"));
	}

	// return if a custom move is already being performed
	if (MovementMode == EMovementMode::MOVE_Custom)
	{
		return;
	}

	if (IsWallRunning || IsVerticalWallRunning || IsZiplining || IsClimbingLadder)
	{
		return;
	}

	if (IsValid(OtherActor))
	{

		if (OtherActor->IsA(AZipline::StaticClass()))
		{
			bool CanZipline = CheckCanZipline();

			if (CanZipline)
			{
				ZiplineStart = static_cast<AZipline*>(OtherActor)->StartPoint;
				ZiplineEnd = static_cast<AZipline*>(OtherActor)->EndPoint;
				ZiplineDirection = static_cast<AZipline*>(OtherActor)->GetZiplineDirection();
			}

			return;
		}
		else if (OtherActor->IsA(ALadder::StaticClass()))
		{
			bool CanClimbLadder = CheckCanClimbLadder();

			if (CanClimbLadder)
			{
				LadderTop = static_cast<ALadder*>(OtherActor)->TopPoint;
				LadderBottom = static_cast<ALadder*>(OtherActor)->BottomPoint;
			}

			return;
		}
	}

	// Runs checks after hitting a potential wall and begins wall running if the checks are passed
	// If wall run can happen and starts then no need to check further actions
	if (CheckCanWallRun(Hit))
	{
		return;
	}

	// Runs checks after hitting a potential wall and begins vertical wall running if the checks are passed
	// If vertical wall run can happen and starts then no need to check further actions
	if (CheckCanVerticalWallRun(Hit))
	{
		return;
	}
}

bool UParkourMovementComponent::IsWalkingForward()
{
	FVector velocity2D = GetPawnOwner()->GetVelocity();
	FVector forward2D = GetPawnOwner()->GetActorForwardVector();
	velocity2D.Z = 0.0f;
	forward2D.Z = 0.0f;
	velocity2D.Normalize();
	forward2D.Normalize();

	return FVector::DotProduct(velocity2D, forward2D) > 0.5f;
}

float UParkourMovementComponent::GetAngleBetweenVectors(FVector Vector1, FVector Vector2)
{
	Vector1.Normalize();
	Vector2.Normalize();

	float Angle = FVector::DotProduct(Vector1, Vector2);
	Angle = acosf(Angle);

	return Angle;
}

FVector UParkourMovementComponent::GetDirectionOfSurface(FVector ImpactNormal)
{
	FVector Direction = FVector::CrossProduct(ImpactNormal, CharacterOwner->GetActorUpVector());
	Direction = FVector::CrossProduct(ImpactNormal, Direction);
	Direction.Normalize();

	return Direction;
}

#pragma region Wall Run Functions

bool UParkourMovementComponent::CheckCanWallRun(const FHitResult Hit)
{
	if (WantsToWallRun == false)
	{
		return false;
	}

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

bool UParkourMovementComponent::CheckWallRunFloor(float Distance)
{
	//looks to see if the player is on the floor while wall running and returns false if the player is currently standing on a floor,
	//as the wall run should end when the player hits the floor

	FFindFloorResult FloorResult;
	FHitResult* HitResult = NULL;

	FindFloor(CharacterOwner->GetActorLocation(), FloorResult, false, HitResult);

	if (HitResult != NULL)
	{
		UE_LOG(LogParkourMovement, Warning, TEXT("Floor Name: %s"), *HitResult->GetActor()->GetName());
	}
	
	if (FloorResult.bWalkableFloor == false)
	{
		UE_LOG(LogParkourMovement, Warning, TEXT("Floor Not Found"));

		return true;
	}

	UE_LOG(LogParkourMovement, Warning, TEXT("Floor Distance: %f"), FloorResult.FloorDist);

	if (FloorResult.FloorDist < Distance)
	{
		return false;
	}

	return true;
}

bool UParkourMovementComponent::CheckWallRunTraces()
{
	FVector TraceStart = CharacterOwner->GetActorLocation();
	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(CharacterOwner);

	// Line trace to the left of the character
	FHitResult HitL;
	FVector TraceEnd = GetWallRunEndVectorL();
	GetWorld()->LineTraceSingleByChannel(HitL, TraceStart, TraceEnd, ECC_Visibility, TraceParams);

	// If the trace hits another actor check if it is valid
	if (HitL.bBlockingHit)
	{
		if (IsValidWallRunVector(HitL.Normal, false))
		{
			WallRunDirection = 1.0;

			UE_LOG(LogParkourMovement, Warning, TEXT("CHECK PASSED L"));

			return true;
		}
	}

	UE_LOG(LogParkourMovement, Warning, TEXT("HIT L FAILED"));

	// Line trace to the right of the character
	FHitResult HitR;
	TraceEnd = GetWallRunEndVectorR();
	GetWorld()->LineTraceSingleByChannel(HitL, TraceStart, TraceEnd, ECC_Visibility, TraceParams);

	// If the trace hits another actor check if it is valid
	if (HitL.bBlockingHit)
	{
		if (IsValidWallRunVector(HitL.Normal, false))
		{
			WallRunDirection = -1.0;

			UE_LOG(LogParkourMovement, Warning, TEXT("CHECK PASSED R"));

			return true;
		}
	}

	UE_LOG(LogParkourMovement, Warning, TEXT("HIT R FAILED"));
	UE_LOG(LogParkourMovement, Warning, TEXT("CHECK FAILED TRACE"));

	return false;
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
	// Do a line trace from the player into the wall to make sure we're stil along the side of a wall
	FVector crossVector = IsWallRunningL ? FVector(0.0f, 0.0f, -1.0f) : FVector(0.0f, 0.0f, 1.0f);
	FVector traceStart = GetPawnOwner()->GetActorLocation() + (WallRunDirectionVector * 20.0f);
	FVector traceEnd = traceStart + (FVector::CrossProduct(WallRunDirectionVector, crossVector) * 100);
	FHitResult hitResult;

	UE_LOG(LogParkourMovement, Warning, TEXT("CROSS VECTOR: %s"), *crossVector.ToString());
	UE_LOG(LogParkourMovement, Warning, TEXT("WALL RUN DIRECTION VECTOR: %s"), *WallRunDirectionVector.ToString());
	UE_LOG(LogParkourMovement, Warning, TEXT("Trace Start: %s,    Trace End: %s"), *traceStart.ToString(), *traceEnd.ToString());

	// Required parameters for line traces
	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(CharacterOwner);
	TraceParams.AddIgnoredActor(GetPawnOwner());

	// Create a helper lambda for performing the line trace
	auto lineTrace = [&](const FVector& start, const FVector& end)
	{
		return (GetWorld()->LineTraceSingleByChannel(hitResult, start, end, ECollisionChannel::ECC_Visibility, TraceParams));
	};

	// If a vertical tolerance was provided we want to do two line traces - one above and one below the calculated line
	if (vertical_tolerance > FLT_EPSILON)
	{
		// If both line traces miss the wall then return false, we're not next to a wall
		if (lineTrace(FVector(traceStart.X, traceStart.Y, traceStart.Z + vertical_tolerance / 2.0f), FVector(traceEnd.X, traceEnd.Y, traceEnd.Z + vertical_tolerance / 2.0f)) == false &&
			lineTrace(FVector(traceStart.X, traceStart.Y, traceStart.Z - vertical_tolerance / 2.0f), FVector(traceEnd.X, traceEnd.Y, traceEnd.Z - vertical_tolerance / 2.0f)) == false)
		{
			UE_LOG(LogParkourMovement, Warning, TEXT("NEXT TO WALL FAILED MULT TRACE"));

			return false;
		}
	}
	// If no vertical tolerance was provided we just want to do one line trace using the caclulated line
	else
	{
		// return false if the line trace misses the wall
		if (lineTrace(traceStart, traceEnd) == false)
		{
			UE_LOG(LogParkourMovement, Warning, TEXT("NEXT TO WALL FAILED SINGLE TRACE"));

			return false;
		}
	}

	UE_LOG(LogParkourMovement, Warning, TEXT("WALL DISTANCE %f"), hitResult.Distance);

	if (hitResult.bBlockingHit)
		UE_LOG(LogParkourMovement, Warning, TEXT("WALL HIT"));

	UE_LOG(LogParkourMovement, Warning, TEXT("WALL NAME %s"), *hitResult.GetComponent()->GetName());

	UE_LOG(LogParkourMovement, Warning, TEXT("WR NORMAL %s"), *hitResult.Normal.ToString());

	if (CheckWallRunTraces() == false)
	{
		return false;
	}



	// Make sure we're still on the side of the wall we expect to be on
	int newWallRunSide = FindWallRunSide(hitResult.ImpactNormal);
	if (newWallRunSide == 0 && !IsWallRunningL)
	{
		UE_LOG(LogTemp, Warning, TEXT("NEXT TO WALL FAILED LEFT"));

		return false;
	}
	else if (newWallRunSide == 1 && !IsWallRunningR)
	{
		UE_LOG(LogTemp, Warning, TEXT("NEXT TO WALL FAILED RIGHT"));

		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("NEXT TO WALL PASSED %i"), newWallRunSide);

	return true;
}

bool UParkourMovementComponent::CanSurfaceBeWallRan(const FVector& surface_normal) const
{
	// Return false if the surface normal is facing down
	if (surface_normal.Z < -0.05f)
		return false;

	FVector normalNoZ = FVector(surface_normal.X, surface_normal.Y, 0.0f);
	normalNoZ.Normalize();

	// Find the angle of the wall
	float wallAngle = FMath::Acos(FVector::DotProduct(normalNoZ, surface_normal));

	// Return true if the wall angle is less than the walkable floor angle
	return wallAngle < GetWalkableFloorAngle();
}

int UParkourMovementComponent::FindWallRunSide(const FVector& surface_normal)
{
	// returns 1 if the wall is to the right, 0 if the wall is to the left

	FVector crossVector;

	if (FVector2D::DotProduct(FVector2D(surface_normal), FVector2D(GetPawnOwner()->GetActorRightVector())) > 0.0)
	{
		crossVector = FVector(0.0f, 0.0f, 1.0f);
		WallRunDirectionVector = FVector::CrossProduct(surface_normal, crossVector);

		IsWallRunningL = false;
		IsWallRunningR = true;
		return 1;
	}
	else
	{
		crossVector = FVector(0.0f, 0.0f, -1.0f);
		WallRunDirectionVector = FVector::CrossProduct(surface_normal, crossVector);

		IsWallRunningL = true;
		IsWallRunningR = false;
		return 0;
	}
}

bool UParkourMovementComponent::BeginWallRun()
{
	UE_LOG(LogParkourMovement, Display, TEXT("BEGIN WALLRUN %i"), GetPawnOwner()->GetLocalRole());

	if (WantsToWallRun == true && !IsCustomMovementMode(ECustomMovementMode::CMOVE_WallRunning))
	{
		IsWallRunning = true;

		if (IsWallRunningL)
		{
			static_cast<AParkourFPSCharacter*>(GetCharacterOwner())->PlayWallRunLMontage();
		}
		else
		{
			static_cast<AParkourFPSCharacter*>(GetCharacterOwner())->PlayWallRunRMontage();
		}

		return true;
	}

	return false;
}

void UParkourMovementComponent::EndWallRun()
{
	UE_LOG(LogTemp, Display, TEXT("WALL RUN END %i"), GetPawnOwner()->GetLocalRole());

	SetMovementMode(EMovementMode::MOVE_Falling);

	if (IsWallRunning)
	{
		static_cast<AParkourFPSCharacter*>(GetCharacterOwner())->EndWallRunLMontage();
	}
	else
	{
		static_cast<AParkourFPSCharacter*>(GetCharacterOwner())->EndWallRunRMontage();
	}

	IsWallRunning = false;
	IsWallRunningL = false;
	IsWallRunningR = false;
}

void UParkourMovementComponent::DoCustomJump()
{
	if (WantsToCustomJump)
	{
		UE_LOG(LogParkourMovement, Warning, TEXT("Custom Jump %i"), GetPawnOwner()->GetLocalRole());

		if (IsWallRunning)
		{
			EndWallRun();

			FVector LaunchVelocity;

			LaunchVelocity.X = WallRunJumpOffForce * (CharacterOwner->GetActorForwardVector().X + WallRunNormal.X);
			LaunchVelocity.Y = WallRunJumpOffForce * (CharacterOwner->GetActorForwardVector().Y + WallRunNormal.Y);
			LaunchVelocity.Z = WallRunJumpHeight;

			Launch(LaunchVelocity);
		}
		else if (IsVerticalWallRunning)
		{
			FVector LaunchVelocity;

			LaunchVelocity.X = WallRunJumpOffForce * (CharacterOwner->GetActorForwardVector().X);
			LaunchVelocity.Y = WallRunJumpOffForce * (CharacterOwner->GetActorForwardVector().Y);
			LaunchVelocity.Z = WallRunJumpHeight;

			if (IsFacingTowardsWall)
			{
				LaunchVelocity.X = LaunchVelocity.X * -1.f;
				LaunchVelocity.Y = LaunchVelocity.Y * -1.f;
			}

			EndVerticalWallRun();

			Launch(LaunchVelocity);

			UE_LOG(LogParkourMovement, Warning, TEXT("Wall Run Jump Velocity: %s"), *LaunchVelocity.ToString());
		}
		else if (IsClimbingLadder)
		{
			FVector LaunchVelocity;
			
			LaunchVelocity.X = LadderJumpOffForce * (CharacterOwner->GetActorForwardVector().X) * -1.f;
			LaunchVelocity.Y = LadderJumpOffForce * (CharacterOwner->GetActorForwardVector().Y) * -1.f;
			LaunchVelocity.Z = LadderJumpHeight;

			EndClimbLadder();

			Launch(LaunchVelocity);

			UE_LOG(LogParkourMovement, Warning, TEXT("Ladder Jump Velocity %s %i"), *LaunchVelocity.ToString(), GetPawnOwner()->GetLocalRole());
		}
		else if (IsLedgeHanging)
		{
			FVector LaunchVelocity;

			LaunchVelocity.X = LadderJumpOffForce * (CharacterOwner->GetActorForwardVector().X) * -1.f;
			LaunchVelocity.Y = LadderJumpOffForce * (CharacterOwner->GetActorForwardVector().Y) * -1.f;
			LaunchVelocity.Z = LadderJumpHeight;

			EndLedgeHang();

			Launch(LaunchVelocity);

			UE_LOG(LogParkourMovement, Warning, TEXT("Ledge Hang Jump Velocity %s %i"), *LaunchVelocity.ToString(), GetPawnOwner()->GetLocalRole());
		}
	}
}

#pragma endregion

#pragma region Vertical Wall Run Functions

bool UParkourMovementComponent::CheckCanVerticalWallRun(const FHitResult Hit)
{
	UE_LOG(LogParkourMovement, Warning, TEXT("Check Vertical Wall Run"));

	if (WantsToVerticalWallRun == false)
	{
		UE_LOG(LogParkourMovement, Warning, TEXT("Vertical Wall Run Check Failed: Key Not Down"));

		return false;
	}

	if (MovementMode != EMovementMode::MOVE_Walking)
	{
		UE_LOG(LogParkourMovement, Warning, TEXT("Vertical Wall Run Check Failed: Not Walking"));

		return false;
	}

	if (CanSurfaceBeWallRan(Hit.ImpactNormal) == false)
	{
		UE_LOG(LogParkourMovement, Warning, TEXT("Vertical Wall Run Check Failed: Wall Angle"));

		return false;
	}

	if (CheckVerticalWallRunTraces() == false)
	{
		return false;
	}

	BeginVerticalWallRun();

	UE_LOG(LogParkourMovement, Warning, TEXT("Vertical Wall Run Check Passed"));

	return true;
}

bool UParkourMovementComponent::CheckVerticalWallRunTraces()
{
	// Required parameters for line traces
	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(CharacterOwner);
	TraceParams.AddIgnoredActor(GetPawnOwner());

	// Line trace at the character's height
	FHitResult HitLow;
	FVector TraceStart = CharacterOwner->GetActorLocation();
	FVector TraceEnd = TraceStart + (CharacterOwner->GetActorForwardVector() * 75);
	GetWorld()->LineTraceSingleByChannel(HitLow, TraceStart, TraceEnd, ECC_Visibility, TraceParams);

	if (DrawDebug)
	{
		DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Emerald, true, 1, 0, 2);
	}

	if (HitLow.bBlockingHit == false)
	{
		UE_LOG(LogParkourMovement, Warning, TEXT("Check Vertical Wall Run Low Trace Failed"));

		return false;
	}

	// Used to find the how far away from the character the high trace should go if the wall is angled
	FVector WallDirection = GetDirectionOfSurface(HitLow.ImpactNormal) * -1;
	float WallAngle = GetAngleBetweenVectors(WallDirection, CharacterOwner->GetActorUpVector());
	float TraceEndDistance = 0;

	if (WallDirection != FVector(0, 0, 0))
	{
		TraceEndDistance = CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + CharacterOwner->GetActorLocation().Z;
		TraceEndDistance *= tanf(WallAngle);
	}

	// Line trace above the character
	// Line trace at the character's height
	FHitResult HitHigh;
	TraceStart = CharacterOwner->GetActorLocation();
	TraceStart.Z += CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	TraceEnd = TraceStart + (CharacterOwner->GetActorForwardVector() * (75 + TraceEndDistance));
	GetWorld()->LineTraceSingleByChannel(HitHigh, TraceStart, TraceEnd, ECC_Visibility, TraceParams);

	if (DrawDebug)
	{
		DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Emerald, true, 1, 0, 2);
	}

	if (HitHigh.bBlockingHit == false)
	{
		UE_LOG(LogParkourMovement, Warning, TEXT("Check Vertical Wall Run High Trace Failed"));

		return false;
	}

	return true;
}

bool UParkourMovementComponent::BeginVerticalWallRun()
{
	UE_LOG(LogParkourMovement, Warning, TEXT("Begin Vertical Wall Run %i"), GetPawnOwner()->GetLocalRole());

	if (WantsToVerticalWallRun == true && !IsCustomMovementMode(ECustomMovementMode::CMOVE_VerticalWallRunning))
	{
		IsVerticalWallRunning = true;
		IsFacingTowardsWall = true;

		static_cast<AParkourFPSCharacter*>(GetCharacterOwner())->PlayVerticalWallRunMontage();

		return true;
	}

	return false;
}

void UParkourMovementComponent::EndVerticalWallRun()
{
	IsVerticalWallRunning = false;
	IsFacingTowardsWall = false;
	IsRotatingAwayFromWall = false;

	MovementMode = EMovementMode::MOVE_Falling;

	static_cast<AParkourFPSCharacter*>(GetCharacterOwner())->EndVerticalWallRunMontage();
}

#pragma endregion

#pragma region Slide and Crouch Functions

bool UParkourMovementComponent::CheckCanSlide()
{
	if (GetPawnOwner()->GetLocalRole() > ROLE_SimulatedProxy)
	{
		if (CustomMovementMode == ECustomMovementMode::CMOVE_Sliding && MovementMode == EMovementMode::MOVE_Custom)
		{
			return true;
		}

		if (!IsWalkingForward())
		{
			UE_LOG(LogParkourMovement, Warning, TEXT("Slide Check Failed: Not Walking Forward"));

			return false;
		}

		if (MovementMode != EMovementMode::MOVE_Walking)
		{
			UE_LOG(LogParkourMovement, Warning, TEXT("Slide Check Failed: Movement Mode Not Walking"));

			return false;
		}

		UE_LOG(LogParkourMovement, Warning, TEXT("Slide Check Passed"));

		return true;
	}

	return false;

	UE_LOG(LogParkourMovement, Warning, TEXT("Slide Check Failed: Too Low Local Role"));
}

bool UParkourMovementComponent::CanStandUp()
{
	return true;
}

bool UParkourMovementComponent::CanStandUpLineTrace(FVector CharacterFeetLocation, FVector CharacterHeadLocation)
{
	return true;
}

void UParkourMovementComponent::BeginSlide()
{
	UE_LOG(LogParkourMovement, Warning, TEXT("BEGIN SLIDE"));

	if (WantsToSlide == true && !IsSliding)
	{
		//SetMovementMode(EMovementMode::MOVE_Custom, ECustomMovementMode::CMOVE_Sliding);
	}

	IsCrouched = true;
	IsSliding = true;

	Velocity = CharacterOwner->GetActorForwardVector() * 800;
	GroundFriction = 0.f;
	BrakingDecelerationWalking = 1000.f;

	AParkourFPSCharacter* characterOwner = static_cast<AParkourFPSCharacter*>(GetCharacterOwner());
	characterOwner->bAcceptingMovementInput = false;
	characterOwner->Crouch();
	characterOwner->PlaySlideStartMontage();
}

void UParkourMovementComponent::EndSlide()
{
	SetMovementMode(EMovementMode::MOVE_Walking);

	IsCrouched = false;
	IsSliding = false;

	WantsToSlide = false;
	MovementKey2Down = false;

	GroundFriction = 8.f;
	BrakingDecelerationWalking = 2048.f;

	AParkourFPSCharacter* characterOwner = static_cast<AParkourFPSCharacter*>(GetCharacterOwner());
	characterOwner->bAcceptingMovementInput = true;
	characterOwner->UnCrouch();
	characterOwner->PlaySlideEndMontage();
}

void UParkourMovementComponent::EndCrouch()
{

}

FVector UParkourMovementComponent::CalculateFloorInfluence(FVector FloorNormal)
{
	// floor is completely flat
	if (FloorNormal == CharacterOwner->GetActorUpVector())
	{
		return FVector(0, 0, 0);
	}

	// Direction of the floor
	FVector FloorInfluence = FVector::CrossProduct(FloorNormal, CharacterOwner->GetActorUpVector());
	FloorInfluence = FVector::CrossProduct(FloorNormal, FloorInfluence);
	FloorInfluence.Normalize();

	//Force that the floor adds
	float FloorForce = FVector::DotProduct(FloorNormal, CharacterOwner->GetActorUpVector());
	FloorForce = 1.0 - FloorForce;
	FloorForce = FMath::Clamp(FloorForce, 0.0f, 1.f) * FloorInfluenceForceFactor;

	FloorInfluence = FloorInfluence * FloorForce;

	return FloorInfluence;
}

#pragma endregion

#pragma region Zipline Functions

bool UParkourMovementComponent::CheckCanZipline()
{
	UE_LOG(LogParkourMovement, Warning, TEXT("Zipline Checks"));

	if (IsCustomMovementMode(ECustomMovementMode::CMOVE_Ziplining))
	{
		return false;
	}

	if (!IsFalling())
	{
		return false;
	}

	if (GetPawnOwner()->IsLocallyControlled())
	{
		WantsToZiplineLadder = true;
	}

	BeginZipline();

	return true;
}

void UParkourMovementComponent::BeginZipline()
{
	if (!IsCustomMovementMode(ECustomMovementMode::CMOVE_Ziplining))
	{
		IsZiplining = true;
	}

	UE_LOG(LogParkourMovement, Warning, TEXT("Zipline Begin"));
}

void UParkourMovementComponent::EndZipline()
{
	WantsToZiplineLadder = false;
	
	IsZiplining = false;

	MovementMode = EMovementMode::MOVE_Falling;

	UE_LOG(LogParkourMovement, Warning, TEXT("Zipline End"));
}

#pragma endregion

#pragma region Ladder Functions

bool UParkourMovementComponent::CheckCanClimbLadder()
{
	if (IsCustomMovementMode(ECustomMovementMode::CMOVE_ClimbLadder))
	{
		return false;
	}

	if (GetPawnOwner()->IsLocallyControlled())
	{
		WantsToZiplineLadder = true;
	}

	BeginClimbLadder();

	return true;
}

void UParkourMovementComponent::BeginClimbLadder()
{
	if (!IsCustomMovementMode(ECustomMovementMode::CMOVE_ClimbLadder))
	{
		IsClimbingLadder = true;
	}

	UE_LOG(LogParkourMovement, Warning, TEXT("Climb Ladder Begin %i"), GetPawnOwner()->GetLocalRole());
}

void UParkourMovementComponent::EndClimbLadder()
{
	WantsToZiplineLadder = false;

	IsClimbingLadder = false;

	SetMovementMode(EMovementMode::MOVE_Falling);

	static_cast<AParkourFPSCharacter*>(GetCharacterOwner())->EndLadderMontage();

	UE_LOG(LogParkourMovement, Warning, TEXT("Climb Ladder End %i"), GetPawnOwner()->GetLocalRole());
}

#pragma endregion

#pragma region Climbing Functions

ELedgeState UParkourMovementComponent::GetStateOfLedge()
{
	ELedgeState LedgeState = ELedgeState::STATE_None;

	if (CheckCanHangLedge())
	{
		LedgeState = ELedgeState::STATE_Grabbable;
	}

	return LedgeState;
}

bool UParkourMovementComponent::CheckCanHangLedge()
{
	FVector TraceStart = CharacterOwner->GetActorLocation() + (CharacterOwner->GetActorForwardVector() * 70.0);
	TraceStart.Z += CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	FVector TraceEnd = CharacterOwner->GetActorLocation() + (CharacterOwner->GetActorForwardVector() * 70.0);
	TraceEnd.Z -= CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	FHitResult HitLow;
	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(CharacterOwner);

	bool SurfaceFoundLow = GetWorld()->LineTraceSingleByChannel(HitLow, TraceStart, TraceEnd, ECC_Visibility, TraceParams);

	if (SurfaceFoundLow)
	{
		UE_LOG(LogParkourMovement, Warning, TEXT("LEDGE HANG LOW HIT"));
	}
	else
	{
		UE_LOG(LogParkourMovement, Warning, TEXT("LEDGE HANG LOW NOT HIT"));
	}

	if (DrawDebug)
	{
		DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Magenta, true, 1, 0, 2);
	}

	if (HitLow.bBlockingHit == false)
	{
		return false;
	}

	LedgeHeight = HitLow.Location.Z;

	// Make sure that the surface is at an appropriate height
	float SurfaceHeight = HitLow.Location.Z - HitLow.TraceEnd.Z;

	if (SurfaceHeight < MinClimbHeight || SurfaceHeight > MaxClimbHeight)
	{
		UE_LOG(LogParkourMovement, Warning, TEXT("LEDGE HANG HEIGHT FAILED"));

		return false;
	}

	TraceEnd = HitLow.ImpactPoint;
	TraceEnd.Z += 1;
	FHitResult HitHi;

	bool SurfaceFoundHigh = GetWorld()->LineTraceSingleByChannel(HitHi, TraceStart, TraceEnd, ECC_Visibility, TraceParams);

	if (DrawDebug)
	{
		DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Red, true, 1, 0, 2);
	}

	if (SurfaceFoundHigh)
	{
		UE_LOG(LogParkourMovement, Warning, TEXT("LEDGE HANG HIGH HIT"));
	}
	else
	{
		UE_LOG(LogParkourMovement, Warning, TEXT("LEDGE HANG HIGH NOT HIT"));
	}

	if (HitHi.bBlockingHit)
	{
		return false;
	}

	if (HitLow.Normal.Z < GetWalkableFloorZ())
	{
		return false;
	}

	// Save the direction of the wall/ledge facing towards the character, used for setting camera rotation limits
	TraceStart = GetCharacterOwner()->GetActorLocation();
	TraceEnd = TraceStart + (GetCharacterOwner()->GetActorForwardVector() * 100);
	FHitResult HitLedgeNormal;

	bool LedgeWasHit = GetWorld()->LineTraceSingleByChannel(HitLedgeNormal, TraceStart, TraceEnd, ECC_Visibility, TraceParams);

	LedgeNormal = HitLedgeNormal.ImpactNormal;

	return true;
}

bool UParkourMovementComponent::CheckCanClimb()
{
	FVector TraceStart = CharacterOwner->GetActorLocation() + (CharacterOwner->GetActorForwardVector() * 70.0);
	TraceStart.Z += CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	FVector TraceEnd = CharacterOwner->GetActorLocation() + (CharacterOwner->GetActorForwardVector() * 70.0);
	TraceEnd.Z -= CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	FHitResult Hit;
	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(CharacterOwner);

	bool SurfaceFound = GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, TraceParams);

	if (DrawDebug)
	{
		DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Blue, true, 1, 0, 2);
	}

	if (SurfaceFound == false)
	{
		UE_LOG(LogParkourMovement, Warning, TEXT("Check Can Climb Failed No Surface"));

		return false;
	}
	else
	{
		// Make sure that the surface is at an appropriate height
		float SurfaceHeight = Hit.Location.Z - Hit.TraceEnd.Z;

		if (SurfaceHeight < MinClimbHeight || SurfaceHeight > MaxClimbHeight)
		{
			UE_LOG(LogParkourMovement, Warning, TEXT("Climb not at correct height"));

			return false;
		}


		if (CheckCanClimbToHit(Hit) == false)
		{
			UE_LOG(LogParkourMovement, Warning, TEXT("Check Can Climb Failed Can't Climb To Hit"));

			return false;
		}
	}

	UE_LOG(LogParkourMovement, Warning, TEXT("Check Can Climb Passed"));

	return true;
}

bool UParkourMovementComponent::CheckCanClimbToHit(FHitResult Hit)
{
	// Make sure the surface thats being climbed to is at a walkable angle
	if (Hit.Normal.Z < GetWalkableFloorZ())
	{
		UE_LOG(LogParkourMovement, Warning, TEXT("CLIMB SURFACE IS NOT WALKABLE"));

		return false;
	}

	float CapsuleHalfHeight = CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	// Make sure that there is nothing above the surface that blocks the player from getting onto the surface
	FVector EndLocation = Hit.Location;
	EndLocation.Z += CapsuleHalfHeight +  1;

	FHitResult ClearHit;
	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(CharacterOwner);

	bool ActorHit = GetWorld()->SweepSingleByChannel(ClearHit, EndLocation, EndLocation, FQuat::Identity, ECC_Visibility,
	FCollisionShape::MakeCapsule(CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleRadius(), CapsuleHalfHeight), TraceParams);

	if (DrawDebug)
	{
		FVector DebugCapsuleCenter = Hit.Location;
		DebugCapsuleCenter.Z += CapsuleHalfHeight + 1;

		DrawDebugCapsule(GetWorld(), DebugCapsuleCenter, CapsuleHalfHeight, CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleRadius(), FQuat::Identity, FColor::Cyan, true, 1, 0, 2);
	}

	if (ActorHit)
	{
		UE_LOG(LogTemp, Warning, TEXT("CLIMB SURFACE NOT CLEAR"));
		UE_LOG(LogTemp, Warning, TEXT("Climb Clear Actor Hit: %s"), *ClearHit.GetActor()->GetName());

		return false;
	}

	return true;
}

bool UParkourMovementComponent::CheckCanQuickClimb()
{
	if (MovementMode != EMovementMode::MOVE_Walking || IsSliding)
	{
		return false;
	}

	FVector TraceStart = CharacterOwner->GetActorLocation() + (CharacterOwner->GetActorForwardVector() * 70.0);
	TraceStart.Z += CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	FVector TraceEnd = CharacterOwner->GetActorLocation() + (CharacterOwner->GetActorForwardVector() * 70.0);
	TraceEnd.Z -= CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	FHitResult Hit;
	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(CharacterOwner);

	bool SurfaceFound = GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, TraceParams);

	if (DrawDebug)
	{
		DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Yellow, true, 1, 0, 2);
	}

	if (SurfaceFound == false)
	{
		UE_LOG(LogParkourMovement, Warning, TEXT("Check Can Quick Climb Failed No Surface"));

		return false;
	}
	else
	{
		// Make sure that the surface is at an appropriate height
		float SurfaceHeight = Hit.Location.Z - Hit.TraceEnd.Z;

		if (SurfaceHeight < MinQuickClimbHeight || SurfaceHeight > MaxQuickClimbHeight)
		{
			UE_LOG(LogParkourMovement, Warning, TEXT("Climb not at correct height"));

			return false;
		}


		if (CheckCanClimbToHit(Hit) == false)
		{
			UE_LOG(LogParkourMovement, Warning, TEXT("Check Can Quick Climb Failed Can't Climb To Hit"));

			return false;
		}
	}

	UE_LOG(LogParkourMovement, Warning, TEXT("Check Can Quick Climb Passed"));

	return true;
}

bool UParkourMovementComponent::CheckCanVault()
{
	float CapsuleRadius = CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleRadius();

	FVector TraceStart = CharacterOwner->GetActorLocation() + (CharacterOwner->GetActorForwardVector() * (CapsuleRadius + MaxQuickClimbWallWidth + 10));
	TraceStart.Z += CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 3;

	FVector TraceEnd = CharacterOwner->GetActorLocation() + (CharacterOwner->GetActorForwardVector() * (CapsuleRadius + MaxQuickClimbWallWidth + 10));
	TraceEnd.Z -= CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	FHitResult Hit;
	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(CharacterOwner);

	bool SurfaceFound = GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, TraceParams);

	if (DrawDebug)
	{
		DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Orange, true, 1, 0, 2);
	}

	if (SurfaceFound)
	{
		return false;
	}

	return true;
}

void UParkourMovementComponent::BeginLedgeHang()
{
	UE_LOG(LogParkourMovement, Warning, TEXT("Begin Ledge Hang %i"), PawnOwner->GetLocalRole());

	SetMovementMode(EMovementMode::MOVE_Custom, ECustomMovementMode::CMOVE_LedgeHang);

	IsLedgeHanging = true;

	GetParkourFPSCharacter()->PlayLedgeHangMontage();

	GetParkourFPSCharacter()->bAcceptingMovementInput = false;
	GetParkourFPSCharacter()->bUseControllerRotationYaw = false;

	
	FVector LedgeRotationVector = LedgeNormal;
	LedgeRotationVector.X *= -1;
	LedgeRotationVector.Y *= -1;

	FRotator LedgeRotation = LedgeRotationVector.Rotation();
	SetCameraRotationLimit(-89.00002, 89.00002, -89.00002, 89.00002, LedgeRotation.Yaw - 70, LedgeRotation.Yaw + 70);


	FVector PlayerOnLedgeLocation = GetParkourFPSCharacter()->GetActorLocation();
	PlayerOnLedgeLocation.Z = LedgeHeight - LedgeHeightOffset;
	GetParkourFPSCharacter()->SetActorLocation(PlayerOnLedgeLocation);
}

void UParkourMovementComponent::EndLedgeHang()
{
	UE_LOG(LogParkourMovement, Warning, TEXT("End Ledge Hang %i"), PawnOwner->GetLocalRole());

	SetMovementMode(EMovementMode::MOVE_Falling);

	IsLedgeHanging = false;
	WantsToStopLedgeHang = false;

	GetParkourFPSCharacter()->EndLedgeHangMontage();

	GetParkourFPSCharacter()->bAcceptingMovementInput = true;
	GetParkourFPSCharacter()->bUseControllerRotationYaw = true;

	SetCameraRotationLimit(-89.00002, 89.00002, -89.00002, 89.00002, 0, 359.98993);
}

#pragma endregion

void UParkourMovementComponent::OnClientCorrectionReceived(class FNetworkPredictionData_Client_Character& ClientData, float TimeStamp, FVector NewLocation, FVector NewVelocity,
	UPrimitiveComponent* NewBase, FName NewBaseBoneName, bool bHasBase, bool bBaseRelativePosition, uint8 ServerMovementMode) 
{
	Super::OnClientCorrectionReceived(ClientData, TimeStamp, NewLocation, NewVelocity, NewBase, NewBaseBoneName, bHasBase, bBaseRelativePosition, ServerMovementMode);
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

#pragma region Phys Functions

void UParkourMovementComponent::PhysCustom(float deltaTime, int32 Iterations)
{
	// Phys* functions should only run for characters with ROLE_Authority or ROLE_AutonomousProxy. However, Unreal calls PhysCustom in
	// two seperate locations, one of which doesn't check the role, so we must check it here to prevent this code from running on simulated proxies.
	if (GetOwner()->GetLocalRole() == ROLE_SimulatedProxy)
		return;

	switch (CustomMovementMode)
	{
	case ECustomMovementMode::CMOVE_WallRunning:
	{
		UE_LOG(LogParkourMovement, Warning, TEXT("Phys Wall Run %i"), GetPawnOwner()->GetLocalRole());

		PhysWallRun(deltaTime, Iterations);

		break;
	}
	case ECustomMovementMode::CMOVE_VerticalWallRunning:
	{
		UE_LOG(LogParkourMovement, Warning, TEXT("Phys Vertical Wall Run %i"), GetPawnOwner()->GetLocalRole());

		PhysVerticalWallRun(deltaTime, Iterations);

		break;
	}
	case ECustomMovementMode::CMOVE_Sliding:
	{
		UE_LOG(LogParkourMovement, Warning, TEXT("Phys Sliding %i"), GetPawnOwner()->GetLocalRole());

		PhysSlide(deltaTime, Iterations);

		break;
	}
	case ECustomMovementMode::CMOVE_Ziplining:
	{
		UE_LOG(LogParkourMovement, Warning, TEXT("Phys Zipline %i"), GetPawnOwner()->GetLocalRole());

		PhysZipline(deltaTime, Iterations);

		break;
	}
	case ECustomMovementMode::CMOVE_ClimbLadder:
	{
		UE_LOG(LogParkourMovement, Warning, TEXT("Phys Climb Ladder %i"), GetPawnOwner()->GetLocalRole());

		PhysClimbLadder(deltaTime, Iterations);

		break;
	}
	}

	Super::PhysCustom(deltaTime, Iterations);
}

void UParkourMovementComponent::PhysWallRun(float deltaTime, int32 Iterations)
{
	// End the wall run if the player is no longer holding down the wall running key
	if (WantsToWallRun == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("RETURN PHYS %i"), GetPawnOwner()->GetLocalRole());
		EndWallRun();
		return;
	}

	// End the wall run if the player is no long on a wall
	if (IsNextToWall(WallRunLineTraceVerticalTolerance) == false)
	{
		EndWallRun();
		return;
	}

	// End the wall run if the player has hit the floor
	if (CheckWallRunFloor(0.7) == false)
	{
		EndWallRun();
		return;
	}

	// required parameters for line traces
	FVector TraceStart = CharacterOwner->GetActorLocation();
	FVector TraceEnd = GetWallRunEndVectorL();
	FHitResult HitL;
	FHitResult HitR;
	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(CharacterOwner);

	GetWorld()->LineTraceSingleByChannel(HitL, TraceStart, TraceEnd, ECC_Visibility, TraceParams);

	// Set the wall run direction based whether a wall is found on the left or the right side of the character
	if (HitL.bBlockingHit)
	{
		if (IsValidWallRunVector(HitL.Normal, true))
		{
			WallRunDirection = 1.0;
		}
	}
	else
	{
		TraceEnd = GetWallRunEndVectorR();

		GetWorld()->LineTraceSingleByChannel(HitL, TraceStart, TraceEnd, ECC_Visibility, TraceParams);

		if (HitL.bBlockingHit)
		{
			if (IsValidWallRunVector(HitL.Normal, true))
			{
				WallRunDirection = -1.0;
			}
		}
	}

	// Add forward force
	FVector CrossVector = FVector(0.0, 0.0, 1.0);
	FVector ForwardForce = FVector::CrossProduct(WallRunNormal, CrossVector);
	ForwardForce *= WallRunSpeed * WallRunDirection;

	const FVector Gravity(0.f, 0.f, WallRunGravity);

	// Set velocity using the forward force and gravity
	Velocity.X = ForwardForce.X;
	Velocity.Y = ForwardForce.Y;
	Velocity = NewFallVelocity(Velocity, Gravity, deltaTime);

	// Apply the velocity to the character taking into account delta time to make the movement independent of frame rate
	const FVector AdjustedVelocity = Velocity * deltaTime;
	FHitResult Hit(1.f);
	SafeMoveUpdatedComponent(AdjustedVelocity, UpdatedComponent->GetComponentQuat(), true, Hit);

}

void UParkourMovementComponent::PhysVerticalWallRun(float deltaTime, int32 Iterations)
{
	if (WantsToVerticalWallRun == false)
	{
		EndVerticalWallRun();

		UE_LOG(LogParkourMovement, Warning, TEXT("Vertical wall run ended by wants to vertical wall run false"));
	}

	float CurrentSpeed = Velocity.Size();

	if (IsFacingTowardsWall && CurrentSpeed < VerticalWallRunMinimumSpeed)
	{
		EndVerticalWallRun();

		UE_LOG(LogParkourMovement, Warning, TEXT("Vertical wall run ended by min speed"));
	}

	if (!IsFacingTowardsWall && !IsRotatingAwayFromWall && CurrentSpeed > VerticalWallRunMaxSpeedFacingAwayFromWall)
	{
		EndVerticalWallRun();

		UE_LOG(LogParkourMovement, Warning, TEXT("Vertical wall run ended by max speed"));
	}

	SetVerticalWallRunVelocity(CurrentSpeed);

	// Apply the velocity to the character taking into account delta time to make the movement independent of frame rate
	const FVector AdjustedVelocity = Velocity * deltaTime;
	FHitResult Hit(1.f);

	SafeMoveUpdatedComponent(AdjustedVelocity, UpdatedComponent->GetComponentRotation(), true, Hit);

	
}

void UParkourMovementComponent::SetVerticalWallRunVelocity(float Speed)
{
	// Required parameters for line traces
	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(CharacterOwner);
	TraceParams.AddIgnoredActor(GetPawnOwner());

	// Line trace at the character's height
	FHitResult HitWall;
	FVector TraceStart = CharacterOwner->GetActorLocation();
	FVector TraceEnd;

	if (IsFacingTowardsWall)
	{
		TraceEnd = TraceStart + (CharacterOwner->GetActorForwardVector() * 75);
	}
	else
	{
		TraceEnd = TraceStart - (CharacterOwner->GetActorForwardVector() * 75);
	}
	


	GetWorld()->LineTraceSingleByChannel(HitWall, TraceStart, TraceEnd, ECC_Visibility, TraceParams);

	FVector WallNormal = HitWall.ImpactNormal;

	// Direction of the wall
	FVector WallDirection = FVector::CrossProduct(WallNormal, CharacterOwner->GetActorUpVector());
	WallDirection = FVector::CrossProduct(WallNormal, WallDirection);
	WallDirection.Normalize();

	WallDirection *= -1;

	FVector GravityToAdd = FVector(0, 0, 0);

	
	if (IsFacingTowardsWall)
	{
		GravityToAdd = WallDirection * VerticalWallRunGravity * -1;
		Velocity = WallDirection * Speed;
		Velocity += GravityToAdd;
	}
	else if (!IsRotatingAwayFromWall)
	{
		GravityToAdd = WallDirection * VerticalWallRunGravityFacingAwayFromWall * -1;
		Velocity += GravityToAdd;
	}
	else
	{
		Velocity = FVector(0, 0, 0);
	}
	

	UE_LOG(LogParkourMovement, Warning, TEXT("Vertical Wall Run Velocity: %s"), *Velocity.ToString());
	UE_LOG(LogParkourMovement, Warning, TEXT("Vertical Wall Run Gravity To Add: %s"), *GravityToAdd.ToString());
}

void UParkourMovementComponent::SetVerticalWallRunRotation()
{
	if (!IsVerticalWallRunning)
	{
		return;
	}

	if (WantsToVerticalWallRunRotate)
		UE_LOG(LogParkourMovement, Warning, TEXT("WANTS TO VERTICAL WALL RUN ROTATE %i"), GetPawnOwner()->GetLocalRole());

	if (WantsToVerticalWallRunRotate && IsFacingTowardsWall && !IsRotatingAwayFromWall)
	{
		IsFacingTowardsWall = false;
		IsRotatingAwayFromWall = true;

		UE_LOG(LogParkourMovement, Warning, TEXT("Set Vertical Wall Run Rotation %i"), GetPawnOwner()->GetLocalRole());

		VerticalWallRunTargetRotation = CharacterOwner->GetActorRotation();

		UE_LOG(LogParkourMovement, Warning, TEXT("Vertical Wall Run Starting Rotation: %s"), *VerticalWallRunTargetRotation.ToString());

		VerticalWallRunTargetRotation.Yaw = VerticalWallRunTargetRotation.Yaw + 180;

		UE_LOG(LogParkourMovement, Warning, TEXT("Vertical Wall Run Target Rotation: %s"), *VerticalWallRunTargetRotation.ToString());
	}
}

void UParkourMovementComponent::ApplyVerticalWallRunRotation()
{
	if (!IsVerticalWallRunning)
	{
		return;
	}

	if (IsRotatingAwayFromWall)
	{
		float YawDifference = FMath::Abs(CharacterOwner->GetActorRotation().Yaw - VerticalWallRunTargetRotation.Yaw);

		FVector CurrentRotationVector = CharacterOwner->GetActorRotation().Vector();
		FVector TargetRotationVector = VerticalWallRunTargetRotation.Vector();

		UE_LOG(LogParkourMovement, Warning, TEXT("Controller Rotation: %s"), *GetPawnOwner()->GetControlRotation().ToString());
		UE_LOG(LogParkourMovement, Warning, TEXT("Yaw Difference: %f"), YawDifference);

		if (FVector::Coincident(CurrentRotationVector, TargetRotationVector, cosf(VerticalWallRunRotationCoincidentCosine)))
		{
			IsRotatingAwayFromWall = false;
			UE_LOG(LogParkourMovement, Warning, TEXT("ENDING VERTICAL WALL RUN ROTATION"));
		}
		else
		{
			GetPawnOwner()->AddControllerYawInput(VerticalWallRunRotationSpeed);
		}
	}
}

void UParkourMovementComponent::PhysSlide(float deltaTime, int32 Iterations)
{
	float CurrentSpeed = Velocity.Size();

	// 
	if (CurrentSpeed < CrouchSpeed || !WantsToSlide)
	{
		EndSlide();

		return;
	}


	FHitResult FloorHitResult;
	FVector TraceStart = CharacterOwner->GetActorLocation();
	FVector TraceEnd = TraceStart;
	TraceEnd.Z -= 200;

	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(CharacterOwner);
	TraceParams.AddIgnoredActor(GetPawnOwner());

	GetWorld()->LineTraceSingleByChannel(FloorHitResult, TraceStart, TraceEnd, ECC_Visibility, TraceParams);

	if (!FloorHitResult.bBlockingHit)
	{
		EndSlide();
	}


	FVector FloorInfluenceForce = CalculateFloorInfluence(FloorHitResult.ImpactNormal);

	Velocity += FloorInfluenceForce;

	UE_LOG(LogParkourMovement, Warning, TEXT("FLOOR INFLUENCE: %s"), *FloorInfluenceForce.ToString());
	UE_LOG(LogParkourMovement, Warning, TEXT("Velocity: %s"), *Velocity.ToString());

	if (FloorInfluenceForce.Z == 0.0)
	{
		Velocity.Z = 0.0;
	}


	CurrentSpeed = Velocity.Size();

	if (CurrentSpeed > SlideTerminalSpeed)
	{
		Velocity.Normalize();
		Velocity *= SlideTerminalSpeed;
	}


	CalcVelocity(deltaTime, 0.0, true, 1000.0);

	const FVector AdjustedVelocity = Velocity * deltaTime;
	FHitResult Hit(1.f);
	SafeMoveUpdatedComponent(AdjustedVelocity, UpdatedComponent->GetComponentQuat(), true, Hit);
}

void UParkourMovementComponent::ApplySlideForce()
{
	if (!IsSliding)
	{
		return;
	}

	float CurrentSpeed = Velocity.Size();

	if (CurrentSpeed < CrouchSpeed)
	{
		UE_LOG(LogParkourMovement, Warning, TEXT("SLIDE SPEED TO SLOW"));
	}

	if (CurrentSpeed < CrouchSpeed || !WantsToSlide)
	{
		EndSlide();

		return;
	}

	if (CurrentSpeed > SlideTerminalSpeed)
	{
		Velocity.Normalize();
		Velocity *= SlideTerminalSpeed;
	}



	FHitResult FloorHitResult;
	FVector TraceStart = CharacterOwner->GetActorLocation();
	FVector TraceEnd = TraceStart;
	TraceEnd.Z -= 200;

	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(CharacterOwner);
	TraceParams.AddIgnoredActor(GetPawnOwner());

	GetWorld()->LineTraceSingleByChannel(FloorHitResult, TraceStart, TraceEnd, ECC_Visibility, TraceParams);

	FVector FloorInfluenceForce = CalculateFloorInfluence(FloorHitResult.ImpactNormal);

	UE_LOG(LogParkourMovement, Warning, TEXT("FLOOR INFLUENCE FORCE: %s"), *FloorInfluenceForce.ToString());

	AddForce(FloorInfluenceForce);
}

void UParkourMovementComponent::PhysZipline(float DeltaTime, int32 Iterations)
{
	if (WantsToZiplineLadder == false)
	{
		EndZipline();
		return;
	}

	
	if (CheckWallRunFloor(0.7) == false)
	{
		EndZipline();
		return;
	}

	FVector CharacterLocation = CharacterOwner->GetActorLocation();
	FVector CharacterLocationDirection = ZiplineEnd - (CharacterLocation + 130);
	CharacterLocationDirection.Normalize();

	// end the zipline if the character has passed the zipline end point and is no longer moving towards the endpoint
	if (FVector::DotProduct(CharacterLocationDirection, ZiplineDirection) < 0)
	{
		EndZipline();
		return;
	}

	Velocity += (ZiplineDirection * ZiplineAcceleration);

	UE_LOG(LogParkourMovement, Warning, TEXT("Zipline velocity: %s"), *Velocity.ToString());
	
	float Speed = Velocity.Size();

	if (Speed > ZiplineMaxSpeed)
	{
		Velocity.Normalize();
		Velocity = Velocity * Speed;
	}

	const FVector AdjustedVelocity = Velocity * DeltaTime;
	FHitResult Hit(1.f);
	SafeMoveUpdatedComponent(AdjustedVelocity, UpdatedComponent->GetComponentQuat(), true, Hit);
}

void UParkourMovementComponent::PhysClimbLadder(float DeltaTime, int32 Iterations)
{
	if (!WantsToZiplineLadder)
	{
		UE_LOG(LogParkourMovement, Warning, TEXT("Climb Ladder Ended By WantsToZiplineLadder false"));

		EndClimbLadder();
		return;
	}

	if (CheckWallRunFloor(1.4) == false && WantsToClimbLadderDown)
	{
		UE_LOG(LogParkourMovement, Warning, TEXT("Climb Ladder Ended By Floor"));

		EndClimbLadder();
		return;
	}

	// End climbing if character has reached top of the ladder
	float CharacterFeetHeight = CharacterOwner->GetActorLocation().Z;
	CharacterFeetHeight -= CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	UE_LOG(LogParkourMovement, Warning, TEXT("Ladder Climb - Character Feet Height: %f, Ladder Top = %f"), CharacterFeetHeight, LadderTop.Z);

	if (CharacterFeetHeight >= LadderTop.Z)
	{
		UE_LOG(LogParkourMovement, Warning, TEXT("Climb Ladder Ended By Passing Top"));

		EndClimbLadder();
		return;
	}

	// End climbing if character has reached bottom of ladder
	float CharacterHeadHeight = CharacterOwner->GetActorLocation().Z;
	CharacterHeadHeight += CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	UE_LOG(LogParkourMovement, Warning, TEXT("Ladder Climb - Character Head Height: %f, Ladder Bottom = %f"), CharacterHeadHeight, LadderBottom.Z);

	if (CharacterHeadHeight <= LadderBottom.Z)
	{
		UE_LOG(LogParkourMovement, Warning, TEXT("Climb Ladder Ended By Passing Bottom"));

		EndClimbLadder();
		return;
	}

	FVector OldVelocity = Velocity;

	if (WantsToClimbLadderUp && !WantsToClimbLadderDown)
	{
		Velocity = FVector(0, 0, LadderSpeedUp);
		
		if (Velocity != OldVelocity)
		{
			static_cast<AParkourFPSCharacter*>(GetCharacterOwner())->PlayLadderUpMontage();
		}
	}
	else if (!WantsToClimbLadderUp && WantsToClimbLadderDown)
	{
		Velocity = FVector(0, 0, LadderSpeedDown * -1);

		if (Velocity != OldVelocity)
		{
			static_cast<AParkourFPSCharacter*>(GetCharacterOwner())->PlayLadderDownMontage();
		}
	}
	else
	{
		Velocity = FVector(0, 0, 0);

		if (Velocity != OldVelocity)
		{
			static_cast<AParkourFPSCharacter*>(GetCharacterOwner())->PauseLadderMontage();
		}
	}

	const FVector AdjustedVelocity = Velocity * DeltaTime;
	FHitResult Hit(1.f);
	SafeMoveUpdatedComponent(AdjustedVelocity, UpdatedComponent->GetComponentQuat(), true, Hit);
}

#pragma endregion

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

void UParkourMovementComponent::SetWantsToCustomJump(bool keyIsDown)
{
	if (MovementMode == EMovementMode::MOVE_Custom)
	{
		WantsToCustomJump = keyIsDown;
	}
	else 
	{
		WantsToCustomJump = false;
	}
}

bool UParkourMovementComponent::ServerSetWantsToCustomJump_Validate(const bool WantsToJump)
{
	return true;
}

void UParkourMovementComponent::ServerSetWantsToCustomJump_Implementation(const bool WantsToJump)
{
	WantsToCustomJump = WantsToJump;
}

void UParkourMovementComponent::SetWantsToVerticalWallRunRotate(bool KeyIsDown)
{
	if (IsCustomMovementMode(ECustomMovementMode::CMOVE_VerticalWallRunning))
	{
		WantsToVerticalWallRunRotate = KeyIsDown;
	}
	else
	{
		WantsToVerticalWallRunRotate = false;
	}
}

bool UParkourMovementComponent::ServerSetWantsToVerticalWallRunRotate_Validate(const bool WantsToRotate)
{
	return true;
}

void UParkourMovementComponent::ServerSetWantsToVerticalWallRunRotate_Implementation(const bool WantsToRotate)
{
	WantsToVerticalWallRunRotate = WantsToRotate;
}

void UParkourMovementComponent::SetWantsToStopZipline(bool KeyIsDown)
{
	if (IsZiplining && KeyIsDown)
	{
		WantsToZiplineLadder = false;
	}
}

void UParkourMovementComponent::SetWantsToGoUpLadder(bool KeyIsDown)
{
	WantsToClimbLadderUp = KeyIsDown;
}

void UParkourMovementComponent::SetWantsToGoDownLadder(bool KeyIsDown)
{
	WantsToClimbLadderDown = KeyIsDown;
}

bool UParkourMovementComponent::ServerSetWantsToGoUpLadder_Validate(const bool WantsToGoUp)
{
	return true;
}

void UParkourMovementComponent::ServerSetWantsToGoUpLadder_Implementation(const bool WantsToGoUp)
{
	WantsToClimbLadderUp = WantsToGoUp;
}

bool UParkourMovementComponent::ServerSetWantsToGoDownLadder_Validate(const bool WantsToGoDown)
{
	return true;
}

void UParkourMovementComponent::ServerSetWantsToGoDownLadder_Implementation(const bool WantsToGoDown)
{
	WantsToClimbLadderDown = WantsToGoDown;
}

void UParkourMovementComponent::SetWantsToStopLedgeHang(bool KeyIsDown)
{
	if (IsLedgeHanging)
	{
		WantsToStopLedgeHang = true;
	}
	else
	{
		WantsToStopLedgeHang = false;
	}
}

bool UParkourMovementComponent::ServerSetWantsToStopLedgeHang_Validate(const bool WantsToStop)
{
	return true;
}

void UParkourMovementComponent::ServerSetWantsToStopLedgeHang_Implementation(const bool WantsToStop)
{
	WantsToStopLedgeHang = WantsToStop;
}

bool UParkourMovementComponent::IsCustomMovementMode(uint8 custom_movement_mode) const
{
	return MovementMode == EMovementMode::MOVE_Custom && CustomMovementMode == custom_movement_mode;
}

void FSavedMove_My::Clear()
{
	Super::Clear();

	// Clear all values
	SavedMove1 = 0;
	SavedMove2 = 0;
	SavedMove3 = 0;
	SavedMove4 = 0;

	SavedWantsToCustomJump = false;
	SavedWantsToVerticalWallRunRotate = false;

	SavedWantsToClimbLadderUp = false;
	SavedWantsToClimbLadderDown = false;

	SavedWantsToStopLedgeHang = false;
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

	// Write to the compressed flags
	if (SavedMove1)
		Result |= FLAG_Custom_0;
	if (SavedMove2)
		Result |= FLAG_Custom_1;
	if (SavedMove3)
		Result |= FLAG_Custom_2;
	if (SavedMove4)
		Result |= FLAG_Custom_3;

	return Result;
}

bool FSavedMove_My::CanCombineWith(const FSavedMovePtr& NewMovePtr, ACharacter* Character, float MaxDelta) const
{
	return Super::CanCombineWith(NewMovePtr, Character, MaxDelta);
}

void FSavedMove_My::SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAccel, class FNetworkPredictionData_Client_Character& ClientData)
{
	Super::SetMoveFor(Character, InDeltaTime, NewAccel, ClientData);

	UParkourMovementComponent* charMove = static_cast<UParkourMovementComponent*>(Character->GetCharacterMovement());

	if (charMove)
	{
		// Copy values into the saved move
		SavedMove1 = charMove->WantsToWallRun;
		SavedMove2 = charMove->WantsToSlide;
		SavedMove3 = charMove->WantsToVerticalWallRun;
		SavedMove4 = charMove->WantsToZiplineLadder;

		SavedWantsToCustomJump = charMove->WantsToCustomJump;
		SavedWantsToVerticalWallRunRotate = charMove->WantsToVerticalWallRunRotate;

		SavedWantsToClimbLadderUp = charMove->WantsToClimbLadderUp;
		SavedWantsToClimbLadderDown = charMove->WantsToClimbLadderDown;

		SavedWantsToStopLedgeHang = charMove->WantsToStopLedgeHang;
	}
}

void FSavedMove_My::PrepMoveFor(class ACharacter* Character)
{
	Super::PrepMoveFor(Character);

	UParkourMovementComponent* charMove = static_cast<UParkourMovementComponent*>(Character->GetCharacterMovement());

	if (charMove)
	{
		// Copy values out of the saved move
		charMove->WantsToWallRun = SavedMove1;
		charMove->WantsToSlide = SavedMove2;
		charMove->WantsToVerticalWallRun = SavedMove3;
		charMove->WantsToZiplineLadder = SavedMove4;

		charMove->WantsToCustomJump = SavedWantsToCustomJump;
		charMove->WantsToVerticalWallRunRotate = SavedWantsToVerticalWallRunRotate;

		charMove->WantsToClimbLadderUp = SavedWantsToClimbLadderUp;
		charMove->WantsToClimbLadderDown = SavedWantsToClimbLadderDown;

		charMove->WantsToStopLedgeHang = SavedWantsToStopLedgeHang;
	}
}

FNetworkPredictionData_Client_My::FNetworkPredictionData_Client_My(const UCharacterMovementComponent& ClientMovement)
	: Super(ClientMovement)
{

}

FSavedMovePtr FNetworkPredictionData_Client_My::AllocateNewMove()
{
	return FSavedMovePtr(new FSavedMove_My());
}

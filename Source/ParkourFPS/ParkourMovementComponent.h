// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ParkourFPSCharacter.h"
#include "ParkourMovementComponent.generated.h"

/**
 * 
 */

DECLARE_LOG_CATEGORY_EXTERN(LogMovementCorrections, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogParkourMovement, Log, All);

UCLASS()
class PARKOURFPS_API UParkourMovementComponent : public UCharacterMovementComponent
{
	GENERATED_UCLASS_BODY()

	friend class FSavedMove_My;

private:
	int ClientRootCount;
	int ServerRootcount;

	// used for wallrunning
	bool MovementKey1Down = false;
	// used for sliding
	bool MovementKey2Down = false;
	// user for ???
	bool MovementKey3Down = false;

	uint8 WantsToWallRun : 1;
	uint8 WantsToSlide : 1;
	uint8 WantsToVerticalWallRun : 1;
	uint8 WantsToZiplineLadder : 1;
	
	uint8 WantsToCustomJump : 1;
	uint8 WantsToVerticalWallRunRotate : 1;

	uint8 WantsToClimbLadderUp : 1;
	uint8 WantsToClimbLadderDown : 1;

	uint8 WantsToStopLedgeHang : 1;
	uint8 WantsToClimbLedge : 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom Character Movement", Meta = (AllowPrivateAccess = "true"))
	bool DrawDebug = true;

	// ========================= WALL RUNNING VARIABLES =======================================

	bool IsWallRunning = false;
	bool IsWallRunningL = false;
	bool IsWallRunningR = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom Character Movement|Wall Running", Meta = (AllowPrivateAccess = "true"))
	float WallRunGravity = .25;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom Character Movement|Wall Running", Meta = (AllowPrivateAccess = "true"))
	float WallRunStartSpeed = 10.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom Character Movement|Wall Running", Meta = (AllowPrivateAccess = "true"))
	float WallRunSpeed = 850;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom Character Movement|Wall Running", Meta = (AllowPrivateAccess = "true"))
	float WallRunJumpHeight = 400.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom Character Movement|Wall Running", Meta = (AllowPrivateAccess = "true"))
	float WallRunJumpOffForce = 400.0;

	float WallRunDirection = 0.0;
	FVector WallRunDirectionVector;
	FVector WallRunNormal;
	FVector WallRunImpactNormal;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Custom Character Movement|Wall Running", Meta = (AllowPrivateAccess = "true"))
	float WallRunLineTraceVerticalTolerance = 50.0f;

	// ========================= VERTICAL WALL RUN  VARIABLES =======================================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom Character Movement|Vertical Wall Run ", Meta = (AllowPrivateAccess = "true"))
	float VerticalWallRunGravity = .25;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom Character Movement|Vertical Wall Run ", Meta = (AllowPrivateAccess = "true"))
	float VerticalWallRunGravityFacingAwayFromWall = .25;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom Character Movement|Vertical Wall Run ", Meta = (AllowPrivateAccess = "true"))
	float VerticalWallRunStartSpeed = 10.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom Character Movement|Vertical Wall Run ", Meta = (AllowPrivateAccess = "true"))
	float VerticalWallRunMinimumSpeed = 5.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom Character Movement|Vertical Wall Run ", Meta = (AllowPrivateAccess = "true"))
	float VerticalWallRunMaxSpeedFacingAwayFromWall = 5.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom Character Movement|Vertical Wall Run ", Meta = (AllowPrivateAccess = "true"))
	float VerticalWallRunRotationSpeed = 10.0;

	bool IsVerticalWallRunning = false;

	bool IsFacingTowardsWall = true;
	bool IsRotatingAwayFromWall = false;

	FRotator VerticalWallRunTargetRotation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom Character Movement|Vertical Wall Run ", Meta = (AllowPrivateAccess = "true"))
	float VerticalWallRunRotationCoincidentCosine = 5.0;

	FVector VerticalWallRunNormal;

	// ========================= SLIDING VARIABLES =======================================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom Character Movement|Sliding", Meta = (AllowPrivateAccess = "true"))
	float SlideTerminalSpeed = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom Character Movement|Sliding", Meta = (AllowPrivateAccess = "true"))
	float CrouchSpeed = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom Character Movement|Sliding", Meta = (AllowPrivateAccess = "true"))
	float FloorInfluenceForceFactor = 300.f;

	bool IsSliding = false;
	bool IsCrouched = false;
	
	// ========================= ZIPLINE VARIABLES =======================================
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom Character Movement|Zip line", Meta = (AllowPrivateAccess = "true"))
	float ZiplineStartSpeed = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom Character Movement|Zip line", Meta = (AllowPrivateAccess = "true"))
	float ZiplineAcceleration = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom Character Movement|Zip line", Meta = (AllowPrivateAccess = "true"))
	float ZiplineMaxSpeed = 1200.f;

	bool IsZiplining = false;

	FVector ZiplineStart;
	FVector ZiplineEnd;
	FVector ZiplineDirection;

	// ========================= LADDER VARIABLES =======================================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom Character Movement|Ladder", Meta = (AllowPrivateAccess = "true"))
	float LadderSpeedUp = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom Character Movement|Ladder", Meta = (AllowPrivateAccess = "true"))
	float LadderSpeedDown = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom Character Movement|Ladder", Meta = (AllowPrivateAccess = "true"))
	float LadderJumpHeight = 400.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom Character Movement|Ladder", Meta = (AllowPrivateAccess = "true"))
	float LadderJumpOffForce = 400.0;

	bool IsClimbingLadder = false;

	FVector LadderTop;
	FVector LadderBottom;
	FVector LadderNormal;

	// ====================== Climbing Variables =================================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom Character Movement|Climbing", Meta = (AllowPrivateAccess = "true"))
	float MinLedgeHangHeight = 120.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom Character Movement|Climbing", Meta = (AllowPrivateAccess = "true"))
	float MaxLedgeHangHeight = 170.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom Character Movement|Climbing", Meta = (AllowPrivateAccess = "true"))
	float LedgeHeightOffset = 50.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom Character Movement|Climbing", Meta = (AllowPrivateAccess = "true"))
	float LedgeHeightAdjustmentSpeed = 100.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom Character Movement|Climbing", Meta = (AllowPrivateAccess = "true"))
	float MinClimbHeight = 100.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom Character Movement|Climbing", Meta = (AllowPrivateAccess = "true"))
	float MaxClimbHeight = 170.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom Character Movement|Climbing", Meta = (AllowPrivateAccess = "true"))
	float MinQuickClimbHeight = 50.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom Character Movement|Climbing", Meta = (AllowPrivateAccess = "true"))
	float MaxQuickClimbHeight = 100.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom Character Movement|Climbing", Meta = (AllowPrivateAccess = "true"))
	float MaxQuickClimbWallWidth = 100.0;

	bool IsLedgeHanging = false;

	FVector LedgeNormal;
	float LedgeHeight;

	bool IsClimbingLedge = false;
	bool ClimbQueued = false;

protected:
	virtual void BeginPlay() override;
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

	void LogClientCorrection(bool isServer, float TimeStamp, FVector NewLocation, FVector NewVelocity, uint8 ServerMovementMode);

	AParkourFPSCharacter* GetParkourFPSCharacter();

	void SetCameraRotationLimit(float MinPitch, float MaxPitch, float MinRoll, float MaxRoll, float MinYaw, float MaxYaw);

	bool IsWalkingForward();

	float GetAngleBetweenVectors(FVector Vector1, FVector Vector2);
	FVector GetDirectionOfSurface(FVector ImpactNormal);

	void DoCustomJump();

	// Wall Running Functions
	bool CheckCanWallRun(const FHitResult Hit);
	bool CheckWallRunFloor(float Distance);
	bool CheckWallRunTraces();
	FVector GetWallRunEndVectorL();
	FVector GetWallRunEndVectorR();
	bool IsValidWallRunVector(FVector InVec, bool SaveVector);
	FVector PlayerToWallVector();
	bool BeginWallRun();
	void EndWallRun();
	void PhysWallRun(float deltaTime, int32 Iterations);
	bool IsNextToWall(float vertical_tolerance = 0.0f);
	bool CanSurfaceBeWallRan(const FVector& surface_normal) const;
	int FindWallRunSide(const FVector& surface_normal);

	UFUNCTION()
	void OnActorHit(AActor* SelfActor, AActor* OtherActor, FVector NormalImpulse, const FHitResult& Hit);

	// Vertical Wall Run Functions
	bool CheckCanVerticalWallRun(const FHitResult Hit);
	bool CheckVerticalWallRunTraces();
	bool BeginVerticalWallRun();
	void EndVerticalWallRun();
	void PhysVerticalWallRun(float deltaTime, int32 Iterations);
	void SetVerticalWallRunVelocity(float Speed);
	void SetVerticalWallRunRotation();
	void ApplyVerticalWallRunRotation();

	// Sliding Functions
	bool CheckCanSlide();

	bool CanStandUp();
	bool CanStandUpLineTrace(FVector CharacterFeetLocation, FVector CharacterHeadLocation);

	void BeginSlide();
	void EndSlide();
	void EndCrouch();

	FVector CalculateFloorInfluence(FVector FloorNormal);

	void PhysSlide(float deltaTime, int32 Iterations);

	void ApplySlideForce();

	// Zipline Functions

	bool CheckCanZipline(AActor* HitActor);
	void BeginZipline();
	void EndZipline();
	void PhysZipline(float DeltaTime, int32 Iterations);

	// Ladder Functions
	bool CheckCanClimbLadder();
	void BeginClimbLadder();
	void EndClimbLadder();
	void PhysClimbLadder(float DeltaTime, int32 Iterations);

	// Climbing Functions
	ELedgeState GetStateOfLedge();
	bool CheckCanHangLedge();
	bool CheckCanClimb();
	bool CheckCanClimbToHit(FHitResult Hit);
	bool CheckCanQuickClimb();
	bool CheckCanVault();

	void BeginLedgeHang();
	void EndLedgeHang();
	void PhysLedgeHang(float DeltaTime, int32 Iterations);
	void UpdateLedgeHangState();

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void EndClimbLedge();


public:
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	virtual void PhysCustom(float deltaTime, int32 Iterations) override;
	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;
	virtual void OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity) override;
	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;

	virtual void OnClientCorrectionReceived(class FNetworkPredictionData_Client_Character& ClientData, float TimeStamp, FVector NewLocation, FVector NewVelocity,
	UPrimitiveComponent* NewBase, FName NewBaseBoneName, bool bHasBase, bool bBaseRelativePosition, uint8 ServerMovementMode) override;

	virtual void ClientAdjustPosition(float TimeStamp, FVector NewLoc, FVector NewVel, UPrimitiveComponent* NewBase, FName NewBaseBoneName, bool bHasBase,
	bool bBaseRelativePosition, uint8 ServerMovementMode) override;


	UFUNCTION(BlueprintCallable, Category = "Movement")
	void SetMovementKey1Down(bool KeyIsDown);

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void SetMovementKey2Down(bool KeyIsDown);

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void SetMovementKey3Down(bool KeyIsDown);

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void SetWantsToCustomJump(bool KeyIsDown);

	UFUNCTION(Unreliable, Server, WithValidation)
	void ServerSetWantsToCustomJump(const bool WantsToJump);

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void SetWantsToVerticalWallRunRotate(bool KeyIsDown);

	UFUNCTION(Unreliable, Server, WithValidation)
	void ServerSetWantsToVerticalWallRunRotate(const bool WantsToRotate);

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void SetWantsToStopZipline(bool KeyIsDown);

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void SetWantsToGoUpLadder(bool KeyIsDown);

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void SetWantsToGoDownLadder(bool KeyIsDown);

	UFUNCTION(Unreliable, Server, WithValidation)
	void ServerSetWantsToGoUpLadder(const bool WantsToGoUp);

	UFUNCTION(Unreliable, Server, WithValidation)
	void ServerSetWantsToGoDownLadder(const bool WantsToGoDown);

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void SetWantsToStopLedgeHang(bool KeyIsDown);

	UFUNCTION(Unreliable, Server, WithValidation)
	void ServerSetWantsToStopLedgeHang(const bool WantsToStop);

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void SetWantsToClimbLedge(bool KeyIsDown);

	UFUNCTION(Unreliable, Server, WithValidation)
	void ServerSetWantsToClimbLedge(const bool WantsToClimb);


	bool IsCustomMovementMode(uint8 custom_movement_mode) const;
};

class FSavedMove_My : public FSavedMove_Character
{
public:

	typedef FSavedMove_Character Super;

	// Resets all saved variables.
	virtual void Clear() override;

	// Store input commands in the compressed flags.
	virtual uint8 GetCompressedFlags() const override;

	// This is used to check whether or not two moves can be combined into one.
	// Basically you just check to make sure that the saved variables are the same.
	virtual bool CanCombineWith(const FSavedMovePtr& NewMovePtr, ACharacter* Character, float MaxDelta) const override;

	// Sets up the move before sending it to the server. 
	virtual void SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAccel, class FNetworkPredictionData_Client_Character& ClientData) override;

	// Sets variables on character movement component before making a predictive correction.
	virtual void PrepMoveFor(class ACharacter* Character) override;

private:
	uint8 SavedMove1 : 1;
	uint8 SavedMove2 : 1;
	uint8 SavedMove3 : 1;
	uint8 SavedMove4 : 1;

	uint8 SavedWantsToCustomJump : 1;
	uint8 SavedWantsToVerticalWallRunRotate : 1;

	uint8 SavedWantsToClimbLadderUp : 1;
	uint8 SavedWantsToClimbLadderDown : 1;

	uint8 SavedWantsToStopLedgeHang;
	uint8 SavedWantsToClimbLedge;
};

class FNetworkPredictionData_Client_My : public FNetworkPredictionData_Client_Character
{
public:
	typedef FNetworkPredictionData_Client_Character Super;

	// Constructor
	FNetworkPredictionData_Client_My(const UCharacterMovementComponent& ClientMovement);

	//brief Allocates a new copy of our custom saved move
	virtual FSavedMovePtr AllocateNewMove() override;
};


/** Custom movement modes for Characters. */
UENUM(BlueprintType)
enum ECustomMovementMode
{
	CMOVE_WallRunning   UMETA(DisplayName = "WallRunning"),
	CMOVE_VerticalWallRunning   UMETA(DisplayName = "WallRunning"),
	CMOVE_LedgeHang UMETA(DisplayName = "LedgeHang"),
	CMOVE_Sliding		UMETA(DisplayName = "Sliding"),
	CMOVE_Ziplining		UMETA(DisplayName = "Ziplining"),
	CMOVE_Vaulting		UMETA(DisplayName = "Vaulting"),
	CMOVE_ClimbLadder	UMETA(DisplayName = "ClimbLadder"),
	CMOVE_ClimbLedge	UMETA(DisplayName = "ClimbLedge"),
	CMOVE_MAX			UMETA(Hidden),
};

UENUM(BlueprintType)
enum class ELedgeState : uint8
{
	STATE_None UMETA(DisplayName = "None"),
	STATE_Grabbable UMETA(DisplayName = "Grabbable"),
	STATE_Climbable UMETA(DisplayName = "Climbable"),
	STATE_QuickClimbable UMETA(DisplayName = "QuickClimbable"),
	STATE_Vaultable UMETA(DisplayName = "Vaultable"),

};
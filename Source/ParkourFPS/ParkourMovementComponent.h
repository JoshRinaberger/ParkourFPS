// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ParkourMovementComponent.generated.h"

/**
 * 
 */

DECLARE_LOG_CATEGORY_EXTERN(LogMovementCorrections, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogParkourMovement, Log, All);

UCLASS()
class PARKOURFPS_API UParkourMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

	friend class FSavedMove_My;

private:
	// used for wallrunning
	bool MovementKey1Down = false;
	// used for sliding
	bool MovementKey2Down = false;
	// user for ???
	bool MovementKey3Down = false;

	uint8 WantsToWallRun;
	uint8 WantsToSlide;

	bool IsWallRunning = false;
	bool IsWallRunningL = false;
	bool IsWallRunningR = false;

	FVector WallRunNormal;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Custom Character Movement|Wall Running", Meta = (AllowPrivateAccess = "true"))
	float WallRunLineTraceVerticalTolerance = 50.0f;
	
protected:
	virtual void BeginPlay() override;
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

	bool CheckCanWallRun(const FHitResult Hit);
	bool CheckWallRunFloor();
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

public:
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	virtual void PhysCustom(float deltaTime, int32 Iterations) override;
	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;
	virtual void OnClientCorrectionReceived(class FNetworkPredictionData_Client_Character& ClientData, float TimeStamp, FVector NewLocation, FVector NewVelocity,
		UPrimitiveComponent* NewBase, FName NewBaseBoneName, bool bHasBase, bool bBaseRelativePosition, uint8 ServerMovementMode) override;
	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;


	UFUNCTION(BlueprintCallable, Category = "Movement")
	void SetMovementKey1Down(bool KeyIsDown);

	UFUNCTION(BlueprintCallable, Category = "Movement")
		void SetMovementKey2Down(bool KeyIsDown);

	UFUNCTION(BlueprintCallable, Category = "Movement")
		void SetMovementKey3Down(bool KeyIsDown);


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
	CMOVE_Sliding		UMETA(DisplayName = "Sliding"),
	CMOVE_Ziplining		UMETA(DisplayName = "Ziplining"),
	CMOVE_Vaulting		UMETA(DisplayName = "Vaulting"),
	CMOVE_MAX			UMETA(Hidden),
};
// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ParkourFPSCharacter.generated.h"

class UParkourMovementComponent;

UCLASS(config=Game, Blueprintable)
class PARKOURFPS_API AParkourFPSCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;
public:
	AParkourFPSCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseLookUpRate;

	/** Returns the custom parkour movement component as opposed to the base character movement component */
	UFUNCTION(BlueprintCallable, Category = "Movement")
	UParkourMovementComponent* GetParkourMovementComponent() const;

	virtual void Jump() override;
	virtual void StopJumping() override;

protected:

	/** Resets HMD orientation in VR. */
	void OnResetVR();

	/** Called for forwards/backward input */
	void MoveForward(float Value);

	/** Called for side to side input */
	void MoveRight(float Value);

	/** 
	 * Called via input to turn at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void TurnAtRate(float Rate);

	/**
	 * Called via input to turn look up/down at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void LookUpAtRate(float Rate);

	/** Handler for when a touch input begins. */
	void TouchStarted(ETouchIndex::Type FingerIndex, FVector Location);

	/** Handler for when a touch input stops. */
	void TouchStopped(ETouchIndex::Type FingerIndex, FVector Location);

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	// End of APawn interface

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }



	bool bAcceptingMovementInput = true;

	UFUNCTION(BlueprintImplementableEvent)
	void PlaySlideStartMontage();

	UFUNCTION(BlueprintImplementableEvent)
	void PlaySlideEndMontage();

	UFUNCTION(BlueprintImplementableEvent)
	void PlayWallRunLMontage();

	UFUNCTION(BlueprintImplementableEvent)
	void PlayWallRunRMontage();

	UFUNCTION(BlueprintImplementableEvent)
	void EndWallRunLMontage();

	UFUNCTION(BlueprintImplementableEvent)
	void EndWallRunRMontage();
};


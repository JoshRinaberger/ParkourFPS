// Fill out your copyright notice in the Description page of Project Settings.


#include "Zipline.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"

FName AZipline::CollisionBoxName(TEXT("CollisionBox"));

// Sets default values
AZipline::AZipline()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(AZipline::CollisionBoxName);

}

// Called when the game starts or when spawned
void AZipline::BeginPlay()
{
	Super::BeginPlay();

	StartPoint = GetActorLocation();
	
}

// Called every frame
void AZipline::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

FVector AZipline::GetZiplineDirection()
{
	FVector Direction = EndPoint - StartPoint;

	Direction.Normalize();

	return Direction;
}


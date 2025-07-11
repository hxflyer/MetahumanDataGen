// Fill out your copyright notice in the Description page of Project Settings.


#include "CaptureActor.h"
#include "Kismet/KismetSystemLibrary.h"
//#include "Editor/UnrealEd/Public/Editor.h"
// Sets default values
ACaptureActor::ACaptureActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ACaptureActor::BeginPlay()
{
	Super::BeginPlay();
}

void ACaptureActor::RandomRotation() {

    // random rotate metahuman character
    float RandomZ = FMath::RandRange(0.0f, 360.0f);
    //RandomZ = 0;
    FRotator RandomRotation(0.0f, RandomZ, 0.0f);
    mhActor->TargetMetaHuman->SetActorRotation(RandomRotation);

    // put camera in front of the character face , with given OrbitRadius , -60 to 60 degree random rotate against the face
    float Radians = FMath::DegreesToRadians(RandomZ)+3.14159f*0.5f;
    float RandomRadians = FMath::DegreesToRadians(FMath::RandRange(-60.0f, 60.0f));
    FVector CamLocation = TargetLocation + FVector(
        FMath::Cos(Radians + RandomRadians) * OrbitRadius,
        FMath::Sin(Radians + RandomRadians) * OrbitRadius,
        0.f);
    CamLocation = CamLocation + FVector(FMath::RandRange(-10.0f, 10.0f), FMath::RandRange(-10.0f, 10.0f), FMath::RandRange(-10.0f, 10.0f));
    FVector CamRandomLookLocation = TargetLocation + FVector(FMath::RandRange(-5.0f, 5.0f), FMath::RandRange(-5.0f, 5.0f), FMath::RandRange(-5.0f, 5.0f));
    FRotator LookAtRotation = (CamRandomLookLocation - CamLocation).Rotation();

    camActor->SetActorLocation(CamLocation);
    camActor->SetActorRotation(LookAtRotation);
    camActor->GetRootComponent()->UpdateComponentToWorld();
}

void ACaptureActor::RefreshTargetPosition() {
    FVector Origin, BoxExtent;
    mhActor->TargetMetaHuman->GetActorBounds(false, Origin, BoxExtent);
    UE_LOG(LogTemp, Log, TEXT("Actor Bounds - Origin: %s, Extent: %s"),
        *Origin.ToString(), *BoxExtent.ToString());

    // Calculate min and max points
    FVector MinPoint = Origin - BoxExtent;
    FVector MaxPoint = Origin + BoxExtent;

    UE_LOG(LogTemp, Log, TEXT("Min Point: %s, Max Point: %s"),
        *MinPoint.ToString(), *MaxPoint.ToString());

    float moveDownScale = MaxPoint.Z / 200.0f;
    TargetLocation = FVector(0.0f, 0.0f, MaxPoint.Z - 15* moveDownScale);
}
//void ACaptureActor::
// Called every frame
void ACaptureActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (camActor->CaptureStep == -1) {
        if (FrameID==10) {
            if (CaptureCount % SnapShotPerHDRI == 0) {
                int32 RandomHDRIID = FMath::RandRange(0, envActor->HDRITextures.Num() - 1);
                envActor->LoadHDRIByID(RandomHDRIID);
            }
            RefreshTargetPosition();
            RandomRotation();
            mhActor->RandomExpression();
            envActor->RandomLighting(1, 10);
        }
        else if (FrameID == 100) {
            FString ID = FString::Printf(TEXT("%d_%d"), mhActor->MetahumanID, CaptureCount);
            camActor->Snapshot(ID);
            FrameID = 0;
            CaptureCount++;
        }else {
            if (CaptureCount >= SnapShotPerHDRI * HDRINumPerPlayer) {
                int nextMHID = mhActor->MetahumanID + 1;
                if (nextMHID < mhActor->MHAssets.Num()) {
                    mhActor->LoadMetahumanByID(nextMHID);
                    
                    //TargetLocation = mhActor->;
                    CaptureCount = 0;
                }
                else {
                    // all metahuman capture are completed, in editor mode
                    //GEditor->RequestEndPlayMap();
                    UKismetSystemLibrary::QuitGame(GetWorld(), nullptr, EQuitPreference::Quit, false);
                }
            }
        }
        
	}
    if (camActor->CaptureStep == -1) {
        FrameID++;
    }
}


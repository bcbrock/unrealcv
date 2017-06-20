#pragma once

#include "UnrealCVPrivate.h"
#include "GameFramework/GameMode.h"
#include "UE4CVServer.h"
#include "UE4CVSegmentMode.generated.h"

/**
 *
 */
UCLASS()
class UNREALCV_API AUE4CVSegmentMode : public AGameMode
{
    GENERATED_BODY()
    AUE4CVSegmentMode(){}

    // We always start play in view mode "object_mask".
    virtual void StartPlay() override {
        Super::StartPlay();
        FUE4CVServer::Get().InitWorld();
    }

public:
    
    // Is UE4CVSegmentMode the current game mode?
    static bool Active() {
        return
            FUE4CVServer::Get().GetPawn() &&
            FUE4CVServer::Get().GetPawn()->GetWorld() &&
            dynamic_cast<AUE4CVSegmentMode*>(FUE4CVServer::Get().GetPawn()->GetWorld()->GetAuthGameMode());
    }
};


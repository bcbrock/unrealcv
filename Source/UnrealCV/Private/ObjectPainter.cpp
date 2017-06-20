#include "UnrealCVPrivate.h"
#include "ObjectPainter.h"
#include "StaticMeshResources.h"
#include "UE4CVServer.h"
#include "SceneViewport.h"
#include "Version.h"
#include "ColorMap.h"
#include "UE4CVSegmentMode.h"

FObjectPainter& FObjectPainter::Get()
{
	static FObjectPainter Singleton;
	return Singleton;
}

FExecStatus FObjectPainter::SetActorColor(FString ActorId, FColor Color)
{
	if (Id2Actor.Contains(ActorId))
	{
		AActor* Actor = Id2Actor[ActorId];
		if (PaintObject(Actor, Color))
		{
			Id2Color.Emplace(ActorId, Color);
			return FExecStatus::OK();
		}
		else
		{
			return FExecStatus::Error(FString::Printf(TEXT("Failed to paint object %s"), *ActorId));
		}
	}
	else
	{
		return FExecStatus::Error(FString::Printf(TEXT("Object %s not exist"), *ActorId));
	}
}

FExecStatus FObjectPainter::GetActorColor(FString ActorId)
{
	// Make sure the object color map is initialized
	if (Id2Color.Num() == 0)
	{
		return FExecStatus::Error(TEXT("The object list is empty, probably not initialized correctly"));
	}
	if (Id2Color.Contains(ActorId))
	{
		FColor ObjectColor = Id2Color[ActorId]; // Make sure the object exist
		FString Message = ObjectColor.ToString();
		// FString Message = "%.3f %.3f %.3f %.3f";
		return FExecStatus::OK(Message);
	}
	else
	{
		return FExecStatus::Error(FString::Printf(TEXT("Object %s not exist"), *ActorId));
	}
}

// TODO: This should be moved to command handler
FExecStatus FObjectPainter::GetObjectList()
{
	TArray<FString> Keys;
	this->Id2Actor.GetKeys(Keys);
	FString Message = "";
	for (auto ActorId : Keys)
	{
		Message += ActorId + " ";
	}
	Message = Message.LeftChop(1);
	return FExecStatus::OK(Message);
}

AActor* FObjectPainter::GetObject(FString ActorId)
{
	/** Return the pointer of an object, return NULL if object not found */
	if (Id2Actor.Contains(ActorId))
	{
		return Id2Actor[ActorId];
	}
	else
	{
		return NULL;
	}
}

void FObjectPainter::Reset(ULevel* InLevel)
{
	this->Level = InLevel;
	this->Id2Color.Empty();
	this->Id2Actor.Empty();

	// This list needs to be generated everytime the game restarted.
	check(Level);

        // Normally all objects are set to be painted in different colors for
        // view mode "object_mask". However, if the game is in
        // UE4CVSegmentMode, and an object has been chosen to be segmented,
        // the segmented object is painted white, and all other objects are
        // painted black.
        bool Segment = AUE4CVSegmentMode::Active() && SegmentedObject.Len() != 0;

	uint32 ObjectIndex = 0;
	for (AActor* Actor : Level->Actors)
	{
		if (Actor && IsPaintable(Actor))
		{
                    // NB: The change below is a hack. The actor "label" is
                    // what is shown in the editor when you click on an
                    // object, and this is not guaranteed to be unique, and is
                    // only available in "development builds" (whatever that
                    // means). As far as I can tell there is no way to get the
                    // "name" of the object in the editor. Using the label
                    // should work for static scenes though, and if there do
                    // happen to be collisions, you can reassign labels in the
                    // editor. Note that our segment-mode segmenter will
                    // segment ALL actors with a given label, so one could
                    // even use this behavior to segment multiple objects by
                    // reassigning their labels to be identical.

                    //FString ActorId = Actor->GetHumanReadableName();
                    FString ActorId = Actor->GetActorLabel();
                    
			Id2Actor.Emplace(ActorId, Actor);
                        FColor NewColor;
                        if (Segment) {
                            if (ActorId == SegmentedObject) {
                                NewColor = FColor::White;
                            } else {
                                NewColor = FColor::Black;
                            }
                        } else {
                            NewColor = GetColorFromColorMap(ObjectIndex);
                        }
			Id2Color.Emplace(ActorId, NewColor);
			ObjectIndex++;
		}
	}

	for (auto& Elem : Id2Color)
	{
		FString ActorId = Elem.Key;
		FColor NewColor = Elem.Value;
		AActor* Actor = Id2Actor[ActorId];
		check(PaintObject(Actor, NewColor));
	}
}

void FObjectPainter::SetSegmentedObject(FString Object)
{
    SegmentedObject = Object;
    if (FUE4CVServer::Get().GetPawn() && FUE4CVServer::Get().GetPawn()->GetLevel()) {
        Reset(FUE4CVServer::Get().GetPawn()->GetLevel());
    }
}

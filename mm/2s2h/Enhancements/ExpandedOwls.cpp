#include "2s2h/BenPort.h"
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/ShipInit.hpp"
#include "2s2h/resource/type/Scene.h"
#include "2s2h/resource/type/scenecommand/SetActorList.h"
#include "2s2h/resource/type/scenecommand/SetObjectList.h"

#include <algorithm>
#include <memory>

extern "C" {
#include "variables.h"
}

namespace {

struct ExpandedOwlPlacement {
    s16 sceneId;
    s8 roomNum;
    SOH::ActorEntry actor;
};

// Exact five ActorEntry records from the mm.o2r build already validated in-game.
static const ExpandedOwlPlacement sExpandedOwls[] = {
    { SCENE_BOTI, 1, { ACTOR_OBJ_WARPSTONE, { 118, 323, -2108 }, { 7, 22, 127 }, 10 } },
    { SCENE_TENMON_DAI, 1, { ACTOR_OBJ_WARPSTONE, { -3, -129, -414 }, { 7, 16, 127 }, 11 } },
    { SCENE_22DEKUCITY, 0, { ACTOR_OBJ_WARPSTONE, { -252, 0, 3018 }, { 7, 6814, 127 }, 12 } },
    { SCENE_16GORON_HOUSE, 0, { ACTOR_OBJ_WARPSTONE, { -563, -134, -532 }, { 7, 2835, 127 }, 13 } },
    { SCENE_TORIDE, 0, { ACTOR_OBJ_WARPSTONE, { 325, 200, -712 }, { 7, -30963, 127 }, 14 } },
};

const ExpandedOwlPlacement* FindExpandedOwl(s16 sceneId, s8 roomNum) {
    for (const auto& owl : sExpandedOwls) {
        if (owl.sceneId == sceneId && owl.roomNum == roomNum) {
            return &owl;
        }
    }
    return nullptr;
}

void InjectExpandedOwlIntoLoadedRoom(s16 sceneId, s8 roomNum) {
    const ExpandedOwlPlacement* owl = FindExpandedOwl(sceneId, roomNum);
    if (owl == nullptr || gPlayState == nullptr || roomNum < 0 || roomNum >= gPlayState->roomList.count) {
        return;
    }

    const char* roomResourceName = gPlayState->roomList.romFiles[roomNum].fileName;
    if (roomResourceName == nullptr) {
        return;
    }

    auto room = std::static_pointer_cast<SOH::Scene>(
        Ship::Context::GetRawInstance()->GetResourceManager()->LoadResource(roomResourceName));
    if (room == nullptr) {
        return;
    }

    for (const auto& command : room->commands) {
        if (command->cmdId == SOH::SceneCommandID::SetObjectList) {
            auto* objects = static_cast<SOH::SetObjectList*>(command.get());
            if (std::find(objects->objects.begin(), objects->objects.end(), OBJECT_SEK) == objects->objects.end()) {
                objects->objects.push_back(OBJECT_SEK);
                objects->numObjects = static_cast<uint32_t>(objects->objects.size());
            }
        } else if (command->cmdId == SOH::SceneCommandID::SetActorList) {
            auto* actors = static_cast<SOH::SetActorList*>(command.get());
            const bool alreadyPresent = std::any_of(
                actors->actorList.begin(), actors->actorList.end(), [owl](const SOH::ActorEntry& actor) {
                    return actor.id == ACTOR_OBJ_WARPSTONE && actor.params == owl->actor.params &&
                           actor.pos.x == owl->actor.pos.x && actor.pos.y == owl->actor.pos.y &&
                           actor.pos.z == owl->actor.pos.z;
                });
            if (!alreadyPresent) {
                actors->actorList.push_back(owl->actor);
                actors->numActors = static_cast<uint32_t>(actors->actorList.size());
            }
        }
    }
}

void RegisterExpandedOwls() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnRoomInit>(
        [](s8 sceneId, s8 roomNum) { InjectExpandedOwlIntoLoadedRoom(sceneId, roomNum); });
}

static RegisterShipInitFunc initFunc(RegisterExpandedOwls, {});

} // namespace

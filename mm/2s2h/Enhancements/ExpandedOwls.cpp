#include "2s2h/BenPort.h"
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/ShipInit.hpp"
#include "2s2h/resource/type/Scene.h"
#include "2s2h/resource/type/scenecommand/SetActorCutsceneList.h"
#include "2s2h/resource/type/scenecommand/SetActorList.h"
#include "2s2h/resource/type/scenecommand/SetCsCamera.h"
#include "2s2h/resource/type/scenecommand/SetObjectList.h"
#include "2s2h/resource/type/scenecommand/SetRoomList.h"

#include <algorithm>
#include <cstring>
#include <memory>

extern "C" {
#include "variables.h"
}

namespace {

struct ExpandedOwlPlacement {
    s16 sceneId;
    s8 roomNum;
    const char* sceneResource;
    SOH::ActorEntry actor;
    s16 installedCsId;
};

// Exact placements from the validated mm.o2r build. IDs 10-14 are the five
// expanded owl slots. The low 7 bits of rot.y are replaced with the scene-local
// ActorCutscene ID when the owl is injected; the remaining rotation bits stay intact.
static ExpandedOwlPlacement sExpandedOwls[] = {
    { SCENE_BOTI, 1, "__OTR__scenes/nonmq/Z2_BOTI/Z2_BOTI",
      { ACTOR_OBJ_WARPSTONE, { 118, 323, -2108 }, { 7, 22, 127 }, 10 }, -1 },
    { SCENE_TENMON_DAI, 1, "__OTR__scenes/nonmq/Z2_TENMON_DAI/Z2_TENMON_DAI",
      { ACTOR_OBJ_WARPSTONE, { -3, -129, -414 }, { 7, 16, 127 }, 11 }, -1 },
    { SCENE_22DEKUCITY, 0, "__OTR__scenes/nonmq/Z2_22DEKUCITY/Z2_22DEKUCITY",
      { ACTOR_OBJ_WARPSTONE, { -252, 0, 3018 }, { 7, 6814, 127 }, 12 }, -1 },
    { SCENE_16GORON_HOUSE, 0, "__OTR__scenes/nonmq/Z2_16GORON_HOUSE/Z2_16GORON_HOUSE",
      { ACTOR_OBJ_WARPSTONE, { -563, -134, -532 }, { 7, 2835, 127 }, 13 }, -1 },
    { SCENE_TORIDE, 0, "__OTR__scenes/nonmq/Z2_TORIDE/Z2_TORIDE",
      { ACTOR_OBJ_WARPSTONE, { 325, 200, -712 }, { 7, -30963, 127 }, 14 }, -1 },
};

static constexpr const char* VANILLA_OWL_TEMPLATE_SCENE =
    "__OTR__scenes/nonmq/Z2_30GYOSON/Z2_30GYOSON";

struct OwlCutsceneTemplate {
    SOH::CutsceneEntry entry;
    SOH::ActorCsCamInfoData camera;
    SOH::ActorEntry sourceOwl;
};

std::shared_ptr<SOH::Scene> LoadSceneResource(const char* resourceName) {
    return std::static_pointer_cast<SOH::Scene>(
        Ship::Context::GetRawInstance()->GetResourceManager()->LoadResource(resourceName));
}

bool FindVanillaOwlCutsceneTemplate(OwlCutsceneTemplate& out) {
    auto scene = LoadSceneResource(VANILLA_OWL_TEMPLATE_SCENE);
    if (!scene) {
        return false;
    }

    SOH::SetRoomList* rooms = nullptr;
    SOH::SetActorCutsceneList* cutscenes = nullptr;
    SOH::SetCsCamera* cameras = nullptr;
    for (const auto& command : scene->commands) {
        if (command->cmdId == SOH::SceneCommandID::SetRoomList) {
            rooms = static_cast<SOH::SetRoomList*>(command.get());
        } else if (command->cmdId == SOH::SceneCommandID::SetActorCutsceneList) {
            cutscenes = static_cast<SOH::SetActorCutsceneList*>(command.get());
        } else if (command->cmdId == SOH::SceneCommandID::SetCsCamera) {
            cameras = static_cast<SOH::SetCsCamera*>(command.get());
        }
    }
    if (!rooms || !cutscenes || !cameras) {
        return false;
    }

    s16 csId = -1;
    SOH::ActorEntry sourceOwl{};
    for (const auto& roomName : rooms->fileNames) {
        auto room = std::static_pointer_cast<SOH::Scene>(
            Ship::Context::GetRawInstance()->GetResourceManager()->LoadResource(roomName.c_str()));
        if (!room) {
            continue;
        }
        for (const auto& command : room->commands) {
            if (command->cmdId != SOH::SceneCommandID::SetActorList) {
                continue;
            }
            auto* actors = static_cast<SOH::SetActorList*>(command.get());
            auto it = std::find_if(actors->actorList.begin(), actors->actorList.end(),
                                   [](const SOH::ActorEntry& actor) {
                                       return actor.id == ACTOR_OBJ_WARPSTONE;
                                   });
            if (it != actors->actorList.end()) {
                sourceOwl = *it;
                csId = it->rot.y & 0x7F;
                break;
            }
        }
        if (csId >= 0) {
            break;
        }
    }

    if (csId < 0 || static_cast<size_t>(csId) >= cutscenes->entries.size()) {
        return false;
    }
    const auto& entry = cutscenes->entries[csId];
    if (entry.csCamId < 0 || static_cast<size_t>(entry.csCamId) >= cameras->csCamera.size()) {
        return false;
    }
    const auto& camera = cameras->csCamera[entry.csCamId];
    if (camera.count <= 0 || !camera.actorCsCamFuncData) {
        return false;
    }

    out.entry = entry;
    out.camera.setting = camera.setting;
    out.camera.count = camera.count;
    out.camera.actorCsCamFuncData = new SOH::z64Vec3s[camera.count];
    std::memcpy(out.camera.actorCsCamFuncData, camera.actorCsCamFuncData,
                sizeof(SOH::z64Vec3s) * camera.count);
    out.sourceOwl = sourceOwl;
    return true;
}

s16 InstallExpandedOwlCutscene(ExpandedOwlPlacement& owl) {
    if (owl.installedCsId >= 0) {
        return owl.installedCsId;
    }

    OwlCutsceneTemplate templateData{};
    if (!FindVanillaOwlCutsceneTemplate(templateData)) {
        return -1;
    }

    // Proven by the Astral Observatory test: the first Great Bay camera record
    // is a world-space point. Keep its native owl-relative (-41,+35,+63)
    // offset by translating it from the template owl to this owl. The remaining
    // camera records are control/terminator data and are copied unchanged.
    templateData.camera.actorCsCamFuncData[0].x += owl.actor.pos.x - templateData.sourceOwl.pos.x;
    templateData.camera.actorCsCamFuncData[0].y += owl.actor.pos.y - templateData.sourceOwl.pos.y;
    templateData.camera.actorCsCamFuncData[0].z += owl.actor.pos.z - templateData.sourceOwl.pos.z;

    auto scene = LoadSceneResource(owl.sceneResource);
    if (!scene) {
        delete[] templateData.camera.actorCsCamFuncData;
        return -1;
    }

    SOH::SetActorCutsceneList* cutscenes = nullptr;
    SOH::SetCsCamera* cameras = nullptr;
    for (const auto& command : scene->commands) {
        if (command->cmdId == SOH::SceneCommandID::SetActorCutsceneList) {
            cutscenes = static_cast<SOH::SetActorCutsceneList*>(command.get());
        } else if (command->cmdId == SOH::SceneCommandID::SetCsCamera) {
            cameras = static_cast<SOH::SetCsCamera*>(command.get());
        }
    }

    if (!cutscenes || !cameras || cutscenes->entries.size() >= 0x78 ||
        cameras->csCamera.size() >= 0x7FFF) {
        delete[] templateData.camera.actorCsCamFuncData;
        return -1;
    }

    const s16 cameraId = static_cast<s16>(cameras->csCamera.size());
    const s16 csId = static_cast<s16>(cutscenes->entries.size());
    templateData.entry.csCamId = cameraId;
    cameras->csCamera.push_back(templateData.camera);
    cutscenes->entries.push_back(templateData.entry);

    // Vector insertion can move the backing storage. Refresh the live pointers
    // exactly as in the runtime-confirmed Astral implementation.
    gPlayState->actorCsCamList = reinterpret_cast<ActorCsCamInfo*>(cameras->csCamera.data());
    CutsceneManager_Init(gPlayState, reinterpret_cast<ActorCutscene*>(cutscenes->entries.data()),
                         static_cast<s16>(cutscenes->entries.size()));

    owl.installedCsId = csId;
    return csId;
}

ExpandedOwlPlacement* FindExpandedOwl(s16 sceneId, s8 roomNum) {
    for (auto& owl : sExpandedOwls) {
        if (owl.sceneId == sceneId && owl.roomNum == roomNum) {
            return &owl;
        }
    }
    return nullptr;
}

void InjectExpandedOwlIntoLoadedRoom(s8 sceneId, s8 roomNum) {
    ExpandedOwlPlacement* placement = FindExpandedOwl(sceneId, roomNum);
    if (!placement || !gPlayState || roomNum < 0 || roomNum >= gPlayState->roomList.count) {
        return;
    }

    const s16 csId = InstallExpandedOwlCutscene(*placement);
    if (csId < 0 || csId >= 0x78) {
        return;
    }

    const char* roomResourceName = gPlayState->roomList.romFiles[roomNum].fileName;
    if (!roomResourceName) {
        return;
    }
    auto room = std::static_pointer_cast<SOH::Scene>(
        Ship::Context::GetRawInstance()->GetResourceManager()->LoadResource(roomResourceName));
    if (!room) {
        return;
    }

    SOH::ActorEntry actor = placement->actor;
    actor.rot.y = (actor.rot.y & ~0x7F) | csId;

    for (const auto& command : room->commands) {
        if (command->cmdId == SOH::SceneCommandID::SetObjectList) {
            auto* objects = static_cast<SOH::SetObjectList*>(command.get());
            if (std::find(objects->objects.begin(), objects->objects.end(), OBJECT_SEK) ==
                objects->objects.end()) {
                objects->objects.push_back(OBJECT_SEK);
                objects->numObjects = static_cast<uint32_t>(objects->objects.size());
            }
        } else if (command->cmdId == SOH::SceneCommandID::SetActorList) {
            auto* actors = static_cast<SOH::SetActorList*>(command.get());
            const bool alreadyPresent = std::any_of(
                actors->actorList.begin(), actors->actorList.end(),
                [placement](const SOH::ActorEntry& existing) {
                    return existing.id == ACTOR_OBJ_WARPSTONE &&
                           existing.params == placement->actor.params &&
                           existing.pos.x == placement->actor.pos.x &&
                           existing.pos.y == placement->actor.pos.y &&
                           existing.pos.z == placement->actor.pos.z;
                });
            if (!alreadyPresent) {
                actors->actorList.push_back(actor);
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

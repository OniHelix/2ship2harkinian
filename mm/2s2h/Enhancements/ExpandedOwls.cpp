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

// Work on one expanded owl at a time until the complete native activation path is proven.
static constexpr s16 ASTRAL_OWL_WARP_ID = 11;
static constexpr const char* ASTRAL_SCENE_RESOURCE = "__OTR__scenes/nonmq/Z2_TENMON_DAI/Z2_TENMON_DAI";

// Great Bay Coast contains a vanilla owl statue. Its scene/room resources are used as the
// authoritative template for the ActorCutscene entry and one-point camera data instead of
// fabricating replacement camera values.
static constexpr const char* VANILLA_OWL_TEMPLATE_SCENE = "__OTR__scenes/nonmq/Z2_30GYOSON/Z2_30GYOSON";

static const SOH::ActorEntry sAstralOwl = {
    ACTOR_OBJ_WARPSTONE,
    { -3, -129, -414 },
    { 7, 16, 127 },
    ASTRAL_OWL_WARP_ID,
};

struct OwlCutsceneTemplate {
    SOH::CutsceneEntry entry;
    SOH::ActorCsCamInfoData camera;
};

std::shared_ptr<SOH::Scene> LoadSceneResource(const char* resourceName) {
    return std::static_pointer_cast<SOH::Scene>(
        Ship::Context::GetRawInstance()->GetResourceManager()->LoadResource(resourceName));
}

bool FindVanillaOwlCutsceneTemplate(OwlCutsceneTemplate& outTemplate) {
    auto sourceScene = LoadSceneResource(VANILLA_OWL_TEMPLATE_SCENE);
    if (sourceScene == nullptr) {
        return false;
    }

    SOH::SetRoomList* sourceRooms = nullptr;
    SOH::SetActorCutsceneList* sourceCutscenes = nullptr;
    SOH::SetCsCamera* sourceCameras = nullptr;

    for (const auto& command : sourceScene->commands) {
        if (command->cmdId == SOH::SceneCommandID::SetRoomList) {
            sourceRooms = static_cast<SOH::SetRoomList*>(command.get());
        } else if (command->cmdId == SOH::SceneCommandID::SetActorCutsceneList) {
            sourceCutscenes = static_cast<SOH::SetActorCutsceneList*>(command.get());
        } else if (command->cmdId == SOH::SceneCommandID::SetCsCamera) {
            sourceCameras = static_cast<SOH::SetCsCamera*>(command.get());
        }
    }

    if (sourceRooms == nullptr || sourceCutscenes == nullptr || sourceCameras == nullptr) {
        return false;
    }

    s16 owlCsId = -1;
    for (const auto& roomName : sourceRooms->fileNames) {
        auto room = std::static_pointer_cast<SOH::Scene>(
            Ship::Context::GetRawInstance()->GetResourceManager()->LoadResource(roomName.c_str()));
        if (room == nullptr) {
            continue;
        }

        for (const auto& command : room->commands) {
            if (command->cmdId != SOH::SceneCommandID::SetActorList) {
                continue;
            }

            auto* actors = static_cast<SOH::SetActorList*>(command.get());
            auto owl = std::find_if(actors->actorList.begin(), actors->actorList.end(), [](const SOH::ActorEntry& actor) {
                return actor.id == ACTOR_OBJ_WARPSTONE;
            });
            if (owl != actors->actorList.end()) {
                owlCsId = owl->rot.y & 0x7F;
                break;
            }
        }
        if (owlCsId >= 0) {
            break;
        }
    }

    if (owlCsId < 0 || static_cast<size_t>(owlCsId) >= sourceCutscenes->entries.size()) {
        return false;
    }

    const SOH::CutsceneEntry& sourceEntry = sourceCutscenes->entries[owlCsId];
    if (sourceEntry.csCamId < 0 || static_cast<size_t>(sourceEntry.csCamId) >= sourceCameras->csCamera.size()) {
        return false;
    }

    const SOH::ActorCsCamInfoData& sourceCamera = sourceCameras->csCamera[sourceEntry.csCamId];
    if (sourceCamera.count <= 0 || sourceCamera.actorCsCamFuncData == nullptr) {
        return false;
    }

    outTemplate.entry = sourceEntry;
    outTemplate.camera.setting = sourceCamera.setting;
    outTemplate.camera.count = sourceCamera.count;
    outTemplate.camera.actorCsCamFuncData = new SOH::z64Vec3s[sourceCamera.count];
    std::memcpy(outTemplate.camera.actorCsCamFuncData, sourceCamera.actorCsCamFuncData,
                sizeof(SOH::z64Vec3s) * sourceCamera.count);
    return true;
}

s16 InstallAstralOwlCutscene() {
    static s16 installedCsId = -1;
    if (installedCsId >= 0) {
        return installedCsId;
    }

    OwlCutsceneTemplate owlTemplate{};
    if (!FindVanillaOwlCutsceneTemplate(owlTemplate)) {
        return -1;
    }

    auto astralScene = LoadSceneResource(ASTRAL_SCENE_RESOURCE);
    if (astralScene == nullptr) {
        delete[] owlTemplate.camera.actorCsCamFuncData;
        return -1;
    }

    SOH::SetActorCutsceneList* astralCutscenes = nullptr;
    SOH::SetCsCamera* astralCameras = nullptr;
    for (const auto& command : astralScene->commands) {
        if (command->cmdId == SOH::SceneCommandID::SetActorCutsceneList) {
            astralCutscenes = static_cast<SOH::SetActorCutsceneList*>(command.get());
        } else if (command->cmdId == SOH::SceneCommandID::SetCsCamera) {
            astralCameras = static_cast<SOH::SetCsCamera*>(command.get());
        }
    }

    if (astralCutscenes == nullptr || astralCameras == nullptr || astralCutscenes->entries.size() >= 0x78 ||
        astralCameras->csCamera.size() >= 0x7FFF) {
        delete[] owlTemplate.camera.actorCsCamFuncData;
        return -1;
    }

    const s16 newCameraId = static_cast<s16>(astralCameras->csCamera.size());
    const s16 newCsId = static_cast<s16>(astralCutscenes->entries.size());
    owlTemplate.entry.csCamId = newCameraId;

    astralCameras->csCamera.push_back(owlTemplate.camera);
    astralCutscenes->entries.push_back(owlTemplate.entry);

    // The scene commands were already processed before OnRoomInit. Refresh the two runtime pointers
    // that consume the resized vectors so the newly appended entries are visible to native code.
    gPlayState->actorCsCamList = reinterpret_cast<ActorCsCamInfo*>(astralCameras->csCamera.data());
    CutsceneManager_Init(gPlayState, reinterpret_cast<ActorCutscene*>(astralCutscenes->entries.data()),
                         static_cast<s16>(astralCutscenes->entries.size()));

    installedCsId = newCsId;
    return installedCsId;
}

void InjectAstralOwlIntoLoadedRoom(s8 sceneId, s8 roomNum) {
    if (sceneId != SCENE_TENMON_DAI || roomNum != 1 || gPlayState == nullptr || roomNum < 0 ||
        roomNum >= gPlayState->roomList.count) {
        return;
    }

    const s16 owlCsId = InstallAstralOwlCutscene();
    if (owlCsId < 0 || owlCsId >= 0x78) {
        // Do not inject a statue with an invalid cutscene dependency.
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

    SOH::ActorEntry astralOwl = sAstralOwl;
    astralOwl.rot.y = (astralOwl.rot.y & ~0x7F) | owlCsId;

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
                actors->actorList.begin(), actors->actorList.end(), [](const SOH::ActorEntry& actor) {
                    return actor.id == ACTOR_OBJ_WARPSTONE && actor.params == ASTRAL_OWL_WARP_ID &&
                           actor.pos.x == sAstralOwl.pos.x && actor.pos.y == sAstralOwl.pos.y &&
                           actor.pos.z == sAstralOwl.pos.z;
                });
            if (!alreadyPresent) {
                actors->actorList.push_back(astralOwl);
                actors->numActors = static_cast<uint32_t>(actors->actorList.size());
            }
        }
    }
}

void RegisterExpandedOwls() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnRoomInit>(
        [](s8 sceneId, s8 roomNum) { InjectAstralOwlIntoLoadedRoom(sceneId, roomNum); });
}

static RegisterShipInitFunc initFunc(RegisterExpandedOwls, {});

} // namespace

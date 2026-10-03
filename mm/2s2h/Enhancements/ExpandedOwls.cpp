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
#include <fstream>
#include <memory>

extern "C" {
#include "variables.h"
}

namespace {
static constexpr s16 ASTRAL_OWL_WARP_ID = 11;
static constexpr const char* ASTRAL_SCENE_RESOURCE = "__OTR__scenes/nonmq/Z2_TENMON_DAI/Z2_TENMON_DAI";
static constexpr const char* VANILLA_OWL_TEMPLATE_SCENE = "__OTR__scenes/nonmq/Z2_30GYOSON/Z2_30GYOSON";
static const SOH::ActorEntry sAstralOwl = { ACTOR_OBJ_WARPSTONE, { -3, -129, -414 }, { 7, 16, 127 }, ASTRAL_OWL_WARP_ID };

struct OwlCutsceneTemplate { SOH::CutsceneEntry entry; SOH::ActorCsCamInfoData camera; SOH::ActorEntry sourceOwl; s16 sourceCsId; };

std::shared_ptr<SOH::Scene> LoadSceneResource(const char* resourceName) {
    return std::static_pointer_cast<SOH::Scene>(Ship::Context::GetRawInstance()->GetResourceManager()->LoadResource(resourceName));
}

void DumpAstralCameraTemplate(const OwlCutsceneTemplate& t) {
    std::ofstream out("expanded_owl_camera.txt", std::ios::out | std::ios::trunc);
    if (!out.is_open()) return;
    out << "SOURCE_SCENE=" << VANILLA_OWL_TEMPLATE_SCENE << '\n';
    out << "SOURCE_OWL_POS=" << t.sourceOwl.pos.x << ',' << t.sourceOwl.pos.y << ',' << t.sourceOwl.pos.z << '\n';
    out << "SOURCE_OWL_ROT=" << t.sourceOwl.rot.x << ',' << t.sourceOwl.rot.y << ',' << t.sourceOwl.rot.z << '\n';
    out << "SOURCE_CS_ID=" << t.sourceCsId << '\n';
    out << "ASTRAL_OWL_POS=" << sAstralOwl.pos.x << ',' << sAstralOwl.pos.y << ',' << sAstralOwl.pos.z << '\n';
    out << "ASTRAL_OWL_ROT=" << sAstralOwl.rot.x << ',' << sAstralOwl.rot.y << ',' << sAstralOwl.rot.z << '\n';
    out << "CUTSCENE_PRIORITY=" << t.entry.priority << '\n';
    out << "CUTSCENE_LENGTH=" << t.entry.length << '\n';
    out << "CUTSCENE_CAM_ID=" << t.entry.csCamId << '\n';
    out << "CUTSCENE_SCRIPT_INDEX=" << t.entry.scriptIndex << '\n';
    out << "CUTSCENE_ADDITIONAL_ID=" << t.entry.additionalCsId << '\n';
    out << "CUTSCENE_END_SFX=" << (int)t.entry.endSfx << '\n';
    out << "CUTSCENE_CUSTOM_VALUE=" << (int)t.entry.customValue << '\n';
    out << "CUTSCENE_HUD_VISIBILITY=" << (int)t.entry.hudVisibility << '\n';
    out << "CUTSCENE_END_CAM=" << (int)t.entry.endCam << '\n';
    out << "CAMERA_SETTING=" << t.camera.setting << '\n';
    out << "CAMERA_COUNT=" << t.camera.count << '\n';
    for (s16 i = 0; i < t.camera.count; ++i) {
        const auto& v = t.camera.actorCsCamFuncData[i];
        out << "CAMERA_DATA[" << i << "]=" << v.x << ',' << v.y << ',' << v.z << '\n';
    }
}

bool FindVanillaOwlCutsceneTemplate(OwlCutsceneTemplate& out) {
    auto scene = LoadSceneResource(VANILLA_OWL_TEMPLATE_SCENE);
    if (!scene) return false;
    SOH::SetRoomList* rooms=nullptr; SOH::SetActorCutsceneList* cuts=nullptr; SOH::SetCsCamera* cams=nullptr;
    for (const auto& c: scene->commands) {
        if (c->cmdId==SOH::SceneCommandID::SetRoomList) rooms=static_cast<SOH::SetRoomList*>(c.get());
        else if (c->cmdId==SOH::SceneCommandID::SetActorCutsceneList) cuts=static_cast<SOH::SetActorCutsceneList*>(c.get());
        else if (c->cmdId==SOH::SceneCommandID::SetCsCamera) cams=static_cast<SOH::SetCsCamera*>(c.get());
    }
    if (!rooms||!cuts||!cams) return false;
    s16 cs=-1; SOH::ActorEntry owl{};
    for (const auto& name: rooms->fileNames) {
        auto room=std::static_pointer_cast<SOH::Scene>(Ship::Context::GetRawInstance()->GetResourceManager()->LoadResource(name.c_str()));
        if (!room) continue;
        for (const auto& c: room->commands) if (c->cmdId==SOH::SceneCommandID::SetActorList) {
            auto* a=static_cast<SOH::SetActorList*>(c.get());
            auto it=std::find_if(a->actorList.begin(),a->actorList.end(),[](const SOH::ActorEntry& e){return e.id==ACTOR_OBJ_WARPSTONE;});
            if(it!=a->actorList.end()){owl=*it;cs=it->rot.y&0x7F;break;}
        }
        if(cs>=0) break;
    }
    if(cs<0||(size_t)cs>=cuts->entries.size()) return false;
    const auto& e=cuts->entries[cs];
    if(e.csCamId<0||(size_t)e.csCamId>=cams->csCamera.size()) return false;
    const auto& cam=cams->csCamera[e.csCamId];
    if(cam.count<=0||!cam.actorCsCamFuncData) return false;
    out.entry=e; out.camera.setting=cam.setting; out.camera.count=cam.count;
    out.camera.actorCsCamFuncData=new SOH::z64Vec3s[cam.count];
    std::memcpy(out.camera.actorCsCamFuncData,cam.actorCsCamFuncData,sizeof(SOH::z64Vec3s)*cam.count);
    out.sourceOwl=owl; out.sourceCsId=cs; return true;
}

s16 InstallAstralOwlCutscene() {
    static s16 installed=-1; if(installed>=0) return installed;
    OwlCutsceneTemplate t{}; if(!FindVanillaOwlCutsceneTemplate(t)) return -1;
    DumpAstralCameraTemplate(t);
    // The first Great Bay camera record is a world-space point. Translate it by the
    // owl-to-owl delta so it keeps the native (-41,+35,+63) offset at Astral.
    if (t.camera.count > 0) {
        t.camera.actorCsCamFuncData[0].x += sAstralOwl.pos.x - t.sourceOwl.pos.x;
        t.camera.actorCsCamFuncData[0].y += sAstralOwl.pos.y - t.sourceOwl.pos.y;
        t.camera.actorCsCamFuncData[0].z += sAstralOwl.pos.z - t.sourceOwl.pos.z;
    }
    auto scene=LoadSceneResource(ASTRAL_SCENE_RESOURCE); if(!scene){delete[] t.camera.actorCsCamFuncData;return -1;}
    SOH::SetActorCutsceneList* cuts=nullptr; SOH::SetCsCamera* cams=nullptr;
    for(const auto& c:scene->commands){if(c->cmdId==SOH::SceneCommandID::SetActorCutsceneList)cuts=static_cast<SOH::SetActorCutsceneList*>(c.get());else if(c->cmdId==SOH::SceneCommandID::SetCsCamera)cams=static_cast<SOH::SetCsCamera*>(c.get());}
    if(!cuts||!cams||cuts->entries.size()>=0x78||cams->csCamera.size()>=0x7FFF){delete[] t.camera.actorCsCamFuncData;return -1;}
    s16 camId=(s16)cams->csCamera.size(), csId=(s16)cuts->entries.size(); t.entry.csCamId=camId;
    cams->csCamera.push_back(t.camera); cuts->entries.push_back(t.entry);
    gPlayState->actorCsCamList=reinterpret_cast<ActorCsCamInfo*>(cams->csCamera.data());
    CutsceneManager_Init(gPlayState,reinterpret_cast<ActorCutscene*>(cuts->entries.data()),(s16)cuts->entries.size());
    installed=csId; return installed;
}

void InjectAstralOwlIntoLoadedRoom(s8 sceneId,s8 roomNum){
    if(sceneId!=SCENE_TENMON_DAI||roomNum!=1||!gPlayState||roomNum<0||roomNum>=gPlayState->roomList.count)return;
    s16 cs=InstallAstralOwlCutscene(); if(cs<0||cs>=0x78)return;
    const char* name=gPlayState->roomList.romFiles[roomNum].fileName; if(!name)return;
    auto room=std::static_pointer_cast<SOH::Scene>(Ship::Context::GetRawInstance()->GetResourceManager()->LoadResource(name)); if(!room)return;
    SOH::ActorEntry owl=sAstralOwl; owl.rot.y=(owl.rot.y&~0x7F)|cs;
    for(const auto& c:room->commands){
        if(c->cmdId==SOH::SceneCommandID::SetObjectList){auto* o=static_cast<SOH::SetObjectList*>(c.get());if(std::find(o->objects.begin(),o->objects.end(),OBJECT_SEK)==o->objects.end()){o->objects.push_back(OBJECT_SEK);o->numObjects=(uint32_t)o->objects.size();}}
        else if(c->cmdId==SOH::SceneCommandID::SetActorList){auto* a=static_cast<SOH::SetActorList*>(c.get());bool present=std::any_of(a->actorList.begin(),a->actorList.end(),[](const SOH::ActorEntry& e){return e.id==ACTOR_OBJ_WARPSTONE&&e.params==ASTRAL_OWL_WARP_ID&&e.pos.x==sAstralOwl.pos.x&&e.pos.y==sAstralOwl.pos.y&&e.pos.z==sAstralOwl.pos.z;});if(!present){a->actorList.push_back(owl);a->numActors=(uint32_t)a->actorList.size();}}
    }
}
void RegisterExpandedOwls(){GameInteractor::Instance->RegisterGameHook<GameInteractor::OnRoomInit>([](s8 sceneId,s8 roomNum){InjectAstralOwlIntoLoadedRoom(sceneId,roomNum);});}
static RegisterShipInitFunc initFunc(RegisterExpandedOwls,{});
} // namespace

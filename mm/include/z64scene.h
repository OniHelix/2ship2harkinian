#ifndef Z64SCENE_H
#define Z64SCENE_H

#include "ultra64.h"
#include "z64dma.h"
#include "z64cutscene.h"
#include "unk.h"

struct GameState;
struct PlayState;

#define ROOM_MAX 32 // maximum number of rooms in a scene
#define ROOM_TRANSITION_MAX 48 // maximum number of transition actors in a scene

#define SPAWN_ROT_FLAGS(rotation, flags) (((rotation) << 7) | (flags))

typedef struct {
    /* 0x0 */ uintptr_t vromStart;
    /* 0x4 */ uintptr_t vromEnd;
    char* fileName;
} RomFile; // size = 0x8

#define ROOM_DRAW_OPA (1 << 0)
#define ROOM_DRAW_XLU (1 << 1)

typedef struct {
    /* 0x0 */ u8  code;
    /* 0x1 */ u8  data1;
    /* 0x4 */ u32 data2;
} SCmdBase; // size = 0x8

typedef struct {
    /* 0x0 */ u8  code;
    /* 0x1 */ u8  data1;
    /* 0x4 */ void* segment;
} SCmdSpawnList; // size = 0x8

typedef struct {
    /* 0x0 */ u8  code;
    /* 0x1 */ u8  num;
    /* 0x4 */ void* segment;
} SCmdActorList; // size = 0x8

typedef struct {
    /* 0x0 */ u8  code;
    /* 0x1 */ u8  data1;
    /* 0x4 */ void* segment;
} SCmdCsCameraList; // size = 0x8

typedef struct {
    /* 0x0 */ u8  code;
    /* 0x1 */ u8  data1;
    /* 0x4 */ void* segment;
} SCmdColHeader; // size = 0x8

typedef struct {
    /* 0x0 */ u8  code;
    /* 0x1 */ u8  num;
    /* 0x4 */ void* segment;
} SCmdRoomList; // size = 0x8

typedef struct {
    /* 0x0 */ u8  code;
    /* 0x1 */ u8  data1;
    /* 0x2 */ UNK_TYPE1 pad2[2];
    /* 0x4 */ s8  west;
    /* 0x5 */ s8  vertical;
    /* 0x6 */ s8  south;
    /* 0x7 */ u8  clothIntensity;
} SCmdWindSettings; // size = 0x8

typedef struct {
    /* 0x0 */ u8  code;
    /* 0x1 */ u8  data1;
    /* 0x4 */ void* segment;
} SCmdEntranceList; // size = 0x8

typedef struct {
    /* 0x0 */ u8  code;
    /* 0x1 */ u8  naviQuestHintFileId;
    /* 0x4 */ u32 subKeepId;
} SCmdSpecialFiles; // size = 0x8

typedef struct {
    /* 0x0 */ u8  code;
    /* 0x1 */ u8  gpFlag1;
    /* 0x4 */ u32 gpFlag2;
} SCmdRoomBehavior; // size = 0x8

typedef struct {
    /* 0x0 */ u8  code;
    /* 0x1 */ u8  data1;
    /* 0x4 */ void* segment;
} SCmdMesh; // size = 0x8

typedef struct {
    /* 0x0 */ u8  code;
    /* 0x1 */ u8  num;
    /* 0x4 */ void* segment;
} SCmdObjectList; // size = 0x8

typedef struct {
    /* 0x0 */ u8  code;
    /* 0x1 */ u8  num;
    /* 0x4 */ void* segment;
} SCmdLightList; // size = 0x8

typedef struct {
    /* 0x0 */ u8  code;
    /* 0x1 */ u8  data1;
    /* 0x4 */ void* segment;
} SCmdPathList; // size = 0x8

typedef struct {
    /* 0x0 */ u8  code;
    /* 0x1 */ u8  num;
    /* 0x4 */ void* segment;
} SCmdTransitionActorList; // size = 0x8

typedef struct {
    /* 0x0 */ u8  code;
    /* 0x1 */ u8  num;
    /* 0x4 */ void* segment;
} SCmdLightSettingList; // size = 0x8

typedef struct {
    /* 0x0 */ u8  code;
    /* 0x1 */ u8  data1;
    /* 0x2 */ UNK_TYPE1 pad2[2];
    /* 0x4 */ u8  hour;
    /* 0x5 */ u8  min;
    /* 0x6 */ u8  timeSpeed;
} SCmdTimeSettings; // size = 0x7

typedef struct {
    /* 0x0 */ u8  code;
    /* 0x1 */ u8  data1;
    /* 0x2 */ UNK_TYPE1 pad2[2];
    /* 0x4 */ u8  skyboxId;
    /* 0x5 */ u8  skyboxConfig;
    /* 0x6 */ u8  envLightMode;
} SCmdSkyboxSettings; // size = 0x7

typedef struct {
    /* 0x0 */ u8  code;
    /* 0x1 */ u8  data1;
    /* 0x2 */ UNK_TYPE1 unk_02[5];
    /* 0x7 */ u8  echo;
} SCmdEchoSettings; // size = 0x8

// Sets warp points for owl statues
typedef enum OwlWarpId {
    /*  0x0 */ OWL_WARP_GREAT_BAY_COAST,
    /*  0x1 */ OWL_WARP_ZORA_CAPE,
    /*  0x2 */ OWL_WARP_SNOWHEAD,
    /*  0x3 */ OWL_WARP_MOUNTAIN_VILLAGE,
    /*  0x4 */ OWL_WARP_CLOCK_TOWN,
    /*  0x5 */ OWL_WARP_MILK_ROAD,
    /*  0x6 */ OWL_WARP_WOODFALL,
    /*  0x7 */ OWL_WARP_SOUTHERN_SWAMP,
    /*  0x8 */ OWL_WARP_IKANA_CANYON,
    /*  0x9 */ OWL_WARP_STONE_TOWER,
    /*  0xA */ OWL_WARP_IKANA_GRAVEYARD,
    /*  0xB */ OWL_WARP_ASTRAL_OBSERVATORY,
    /*  0xC */ OWL_WARP_DEKU_PALACE,
    /*  0xD */ OWL_WARP_GORON_SHRINE,
    /*  0xE */ OWL_WARP_PIRATES_FORTRESS,
    /*  0xF */ OWL_WARP_WEST_CLOCK_TOWN,
    /* 0x10 */ OWL_WARP_ENTRANCE, // Special index for warping to the entrance of a scene
    /* 0x11 */ OWL_WARP_MAX,
    /* 0xFF */ OWL_WARP_NONE = 0xFF
} OwlWarpId;

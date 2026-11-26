#include <libultraship/bridge/consolevariablebridge.h>
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/ShipInit.hpp"

extern "C" {
#include "src/overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_scope.h"
#include "z64scene.h"

extern PlayState* gPlayState;
extern u16 sOwlWarpPauseItems[];
}

#define CVAR_NAME "gEnhancements.Songs.BetterOwlWarpMenu"
#define CVAR CVarGetInteger(CVAR_NAME, 0)

static constexpr s16 NAV_BLOCK_SIZE = OWL_WARP_STONE_TOWER - OWL_WARP_GREAT_BAY_COAST;

// Arrays indicating the order of navigation from each owl warp point on the menu. There is one array for each compass
// direction. Each array is broken up into segments relating to each warp point where the cursor is currently. The
// navigation will iterate through each entry in the segment and find the first point that is unlocked. Ten warp points
// means there are ten segments, with nine entries (one fewer) per segment.
static s16 sOwlWarpPauseNavigationLeft[] = {
    // OWL_WARP_GREAT_BAY_COAST
    OWL_WARP_IKANA_CANYON,
    OWL_WARP_STONE_TOWER,
    OWL_WARP_SOUTHERN_SWAMP,
    OWL_WARP_CLOCK_TOWN,
    OWL_WARP_MILK_ROAD,
    OWL_WARP_WOODFALL,
    OWL_WARP_MOUNTAIN_VILLAGE,
    OWL_WARP_SNOWHEAD,
    OWL_WARP_ZORA_CAPE,

    // OWL_WARP_ZORA_CAPE
    OWL_WARP_GREAT_BAY_COAST,
    OWL_WARP_SOUTHERN_SWAMP,
    OWL_WARP_WOODFALL,
    OWL_WARP_IKANA_CANYON,
    OWL_WARP_STONE_TOWER,
    OWL_WARP_CLOCK_TOWN,
    OWL_WARP_MOUNTAIN_VILLAGE,
    OWL_WARP_MILK_ROAD,
    OWL_WARP_SNOWHEAD,

    // OWL_WARP_SNOWHEAD
    OWL_WARP_GREAT_BAY_COAST,
    OWL_WARP_ZORA_CAPE,
    OWL_WARP_MILK_ROAD,
    OWL_WARP_STONE_TOWER,
    OWL_WARP_IKANA_CANYON,
    OWL_WARP_SOUTHERN_SWAMP,
    OWL_WARP_MOUNTAIN_VILLAGE,
    OWL_WARP_WOODFALL,
    OWL_WARP_CLOCK_TOWN,

    // OWL_WARP_MOUNTAIN_VILLAGE
    OWL_WARP_GREAT_BAY_COAST,
    OWL_WARP_ZORA_CAPE,
    OWL_WARP_SNOWHEAD,
    OWL_WARP_CLOCK_TOWN,
    OWL_WARP_MILK_ROAD,
    OWL_WARP_STONE_TOWER,
    OWL_WARP_IKANA_CANYON,
    OWL_WARP_SOUTHERN_SWAMP,
    OWL_WARP_WOODFALL,

    // OWL_WARP_CLOCK_TOWN
    OWL_WARP_MILK_ROAD,
    OWL_WARP_GREAT_BAY_COAST,
    OWL_WARP_ZORA_CAPE,
    OWL_WARP_IKANA_CANYON,
    OWL_WARP_STONE_TOWER,
    OWL_WARP_SOUTHERN_SWAMP,
    OWL_WARP_WOODFALL,
    OWL_WARP_MOUNTAIN_VILLAGE,
    OWL_WARP_SNOWHEAD,

    // OWL_WARP_MILK_ROAD
    OWL_WARP_ZORA_CAPE,
    OWL_WARP_GREAT_BAY_COAST,
    OWL_WARP_IKANA_CANYON,
    OWL_WARP_STONE_TOWER,
    OWL_WARP_SOUTHERN_SWAMP,
    OWL_WARP_WOODFALL,
    OWL_WARP_CLOCK_TOWN,
    OWL_WARP_MOUNTAIN_VILLAGE,
    OWL_WARP_SNOWHEAD,

    // OWL_WARP_WOODFALL
    OWL_WARP_ZORA_CAPE,
    OWL_WARP_GREAT_BAY_COAST,
    OWL_WARP_MILK_ROAD,
    OWL_WARP_CLOCK_TOWN,
    OWL_WARP_SOUTHERN_SWAMP,
    OWL_WARP_STONE_TOWER,
    OWL_WARP_IKANA_CANYON,
    OWL_WARP_MOUNTAIN_VILLAGE,
    OWL_WARP_SNOWHEAD,

    // OWL_WARP_SOUTHERN_SWAMP
    OWL_WARP_WOODFALL,
    OWL_WARP_MILK_ROAD,
    OWL_WARP_ZORA_CAPE,
    OWL_WARP_CLOCK_TOWN,
    OWL_WARP_GREAT_BAY_COAST,
    OWL_WARP_MOUNTAIN_VILLAGE,
    OWL_WARP_SNOWHEAD,
    OWL_WARP_STONE_TOWER,
    OWL_WARP_IKANA_CANYON,

    // OWL_WARP_IKANA_CANYON
    OWL_WARP_CLOCK_TOWN,
    OWL_WARP_SOUTHERN_SWAMP,
    OWL_WARP_MILK_ROAD,
    OWL_WARP_WOODFALL,
    OWL_WARP_MOUNTAIN_VILLAGE,
    OWL_WARP_SNOWHEAD,
    OWL_WARP_ZORA_CAPE,
    OWL_WARP_GREAT_BAY_COAST,
    OWL_WARP_STONE_TOWER,

    // OWL_WARP_STONE_TOWER
    OWL_WARP_MOUNTAIN_VILLAGE,
    OWL_WARP_SNOWHEAD,
    OWL_WARP_IKANA_CANYON,
    OWL_WARP_SOUTHERN_SWAMP,
    OWL_WARP_CLOCK_TOWN,
    OWL_WARP_WOODFALL,
    OWL_WARP_MILK_ROAD,
    OWL_WARP_ZORA_CAPE,
    OWL_WARP_GREAT_BAY_COAST,
};

static s16 sOwlWarpPauseNavigationRight[] = {
    // OWL_WARP_GREAT_BAY_COAST
    OWL_WARP_MILK_ROAD,
    OWL_WARP_CLOCK_TOWN,
    OWL_WARP_WOODFALL,
    OWL_WARP_SOUTHERN_SWAMP,
    OWL_WARP_IKANA_CANYON,
    OWL_WARP_MOUNTAIN_VILLAGE,
    OWL_WARP_ZORA_CAPE,
    OWL_WARP_SNOWHEAD,
    OWL_WARP_STONE_TOWER,

    // OWL_WARP_ZORA_CAPE
    OWL_WARP_WOODFALL,
    OWL_WARP_MILK_ROAD,
    OWL_WARP_SOUTHERN_SWAMP,
    OWL_WARP_CLOCK_TOWN,
    OWL_WARP_IKANA_CANYON,
    OWL_WARP_SNOWHEAD,
    OWL_WARP_MOUNTAIN_VILLAGE,
    OWL_WARP_STONE_TOWER,
    OWL_WARP_GREAT_BAY_COAST,

    // OWL_WARP_SNOWHEAD
    OWL_WARP_STONE_TOWER,
    OWL_WARP_MOUNTAIN_VILLAGE,
    OWL_WARP_IKANA_CANYON,
    OWL_WARP_SOUTHERN_SWAMP,
    OWL_WARP_WOODFALL,
    OWL_WARP_GREAT_BAY_COAST,
    OWL_WARP_ZORA_CAPE,
    OWL_WARP_MILK_ROAD,
    OWL_WARP_CLOCK_TOWN,

    // OWL_WARP_MOUNTAIN_VILLAGE
    OWL_WARP_STONE_TOWER,
    OWL_WARP_IKANA_CANYON,
    OWL_WARP_SOUTHERN_SWAMP,
    OWL_WARP_GREAT_BAY_COAST,
    OWL_WARP_ZORA_CAPE,
    OWL_WARP_MILK_ROAD,
    OWL_WARP_SNOWHEAD,
    OWL_WARP_CLOCK_TOWN,
    OWL_WARP_WOODFALL,

    // OWL_WARP_CLOCK_TOWN
    OWL_WARP_IKANA_CANYON,
    OWL_WARP_SOUTHERN_SWAMP,
    OWL_WARP_STONE_TOWER,
    OWL_WARP_WOODFALL,
    OWL_WARP_GREAT_BAY_COAST,
    OWL_WARP_ZORA_CAPE,
    OWL_WARP_MILK_ROAD,
    OWL_WARP_MOUNTAIN_VILLAGE,
    OWL_WARP_SNOWHEAD,

    // OWL_WARP_MILK_ROAD
    OWL_WARP_CLOCK_TOWN,
    OWL_WARP_IKANA_CANYON,
    OWL_WARP_WOODFALL,
    OWL_WARP_SOUTHERN_SWAMP,
    OWL_WARP_STONE_TOWER,
    OWL_WARP_MOUNTAIN_VILLAGE,
    OWL_WARP_SNOWHEAD,
    OWL_WARP_GREAT_BAY_COAST,
    OWL_WARP_ZORA_CAPE,

    // OWL_WARP_WOODFALL
    OWL_WARP_SOUTHERN_SWAMP,
    OWL_WARP_IKANA_CANYON,
    OWL_WARP_STONE_TOWER,
    OWL_WARP_ZORA_CAPE,
    OWL_WARP_GREAT_BAY_COAST,
    OWL_WARP_MILK_ROAD,
    OWL_WARP_CLOCK_TOWN,
    OWL_WARP_SNOWHEAD,
    OWL_WARP_MOUNTAIN_VILLAGE,

    // OWL_WARP_SOUTHERN_SWAMP
    OWL_WARP_IKANA_CANYON,
    OWL_WARP_STONE_TOWER,
    OWL_WARP_ZORA_CAPE,
    OWL_WARP_GREAT_BAY_COAST,
    OWL_WARP_MILK_ROAD,
    OWL_WARP_WOODFALL,
    OWL_WARP_SNOWHEAD,
    OWL_WARP_CLOCK_TOWN,
    OWL_WARP_MOUNTAIN_VILLAGE,

    // OWL_WARP_IKANA_CANYON
    OWL_WARP_STONE_TOWER,
    OWL_WARP_GREAT_BAY_COAST,
    OWL_WARP_ZORA_CAPE,
    OWL_WARP_MILK_ROAD,
    OWL_WARP_CLOCK_TOWN,
    OWL_WARP_WOODFALL,
    OWL_WARP_SNOWHEAD,
    OWL_WARP_MOUNTAIN_VILLAGE,
    OWL_WARP_SOUTHERN_SWAMP,

    // OWL_WARP_STONE_TOWER
    OWL_WARP_GREAT_BAY_COAST,
    OWL_WARP_ZORA_CAPE,
    OWL_WARP_SNOWHEAD,
    OWL_WARP_MOUNTAIN_VILLAGE,
    OWL_WARP_MILK_ROAD,
    OWL_WARP_CLOCK_TOWN,
    OWL_WARP_WOODFALL,
    OWL_WARP_SOUTHERN_SWAMP,
    OWL_WARP_IKANA_CANYON,
};

static s16 sOwlWarpPauseNavigationUp[] = {
    // OWL_WARP_GREAT_BAY_COAST
    OWL_WARP_SNOWHEAD,
    OWL_WARP_MOUNTAIN_VILLAGE,
    OWL_WARP_ZORA_CAPE,
    OWL_WARP_CLOCK_TOWN,
    OWL_WARP_MILK_ROAD,
    OWL_WARP_STONE_TOWER,
    OWL_WARP_IKANA_CANYON,
    OWL_WARP_WOODFALL,
    OWL_WARP_SOUTHERN_SWAMP,

    // OWL_WARP_ZORA_CAPE
    OWL_WARP_GREAT_BAY_COAST,
    OWL_WARP_MILK_ROAD,
    OWL_WARP_CLOCK_TOWN,
    OWL_WARP_SNOWHEAD,
    OWL_WARP_MOUNTAIN_VILLAGE,
    OWL_WARP_WOODFALL,
    OWL_WARP_SOUTHERN_SWAMP,
    OWL_WARP_IKANA_CANYON,
    OWL_WARP_STONE_TOWER,

    // OWL_WARP_SNOWHEAD
    OWL_WARP_WOODFALL,
    OWL_WARP_ZORA_CAPE,
    OWL_WARP_SOUTHERN_SWAMP,
    OWL_WARP_MILK_ROAD,
    OWL_WARP_CLOCK_TOWN,
    OWL_WARP_GREAT_BAY_COAST,
    OWL_WARP_IKANA_CANYON,
    OWL_WARP_MOUNTAIN_VILLAGE,
    OWL_WARP_STONE_TOWER,

    // OWL_WARP_MOUNTAIN_VILLAGE
    OWL_WARP_SNOWHEAD,
    OWL_WARP_WOODFALL,
    OWL_WARP_SOUTHERN_SWAMP,
    OWL_WARP_MILK_ROAD,
    OWL_WARP_CLOCK_TOWN,
    OWL_WARP_ZORA_CAPE,
    OWL_WARP_GREAT_BAY_COAST,
    OWL_WARP_IKANA_CANYON,
    OWL_WARP_STONE_TOWER,

    // OWL_WARP_CLOCK_TOWN
    OWL_WARP_MOUNTAIN_VILLAGE,
    OWL_WARP_SNOWHEAD,
    OWL_WARP_STONE_TOWER,
    OWL_WARP_WOODFALL,
    OWL_WARP_SOUTHERN_SWAMP,
    OWL_WARP_MILK_ROAD,
    OWL_WARP_ZORA_CAPE,
    OWL_WARP_GREAT_BAY_COAST,
    OWL_WARP_IKANA_CANYON,

    // OWL_WARP_MILK_ROAD
    OWL_WARP_CLOCK_TOWN,
    OWL_WARP_MOUNTAIN_VILLAGE,
    OWL_WARP_SNOWHEAD,
    OWL_WARP_IKANA_CANYON,
    OWL_WARP_STONE_TOWER,
    OWL_WARP_WOODFALL,
    OWL_WARP_ZORA_CAPE,
    OWL_WARP_SOUTHERN_SWAMP,
    OWL_WARP_GREAT_BAY_COAST,

    // OWL_WARP_WOODFALL
    OWL_WARP_MILK_ROAD,
    OWL_WARP_CLOCK_TOWN,
    OWL_WARP_MOUNTAIN_VILLAGE,
    OWL_WARP_SNOWHEAD,
    OWL_WARP_SOUTHERN_SWAMP,
    OWL_WARP_IKANA_CANYON,
    OWL_WARP_STONE_TOWER,
    OWL_WARP_GREAT_BAY_COAST,
    OWL_WARP_ZORA_CAPE,

    // OWL_WARP_SOUTHERN_SWAMP
    OWL_WARP_IKANA_CANYON,
    OWL_WARP_STONE_TOWER,
    OWL_WARP_CLOCK_TOWN,
    OWL_WARP_MOUNTAIN_VILLAGE,
    OWL_WARP_SNOWHEAD,
    OWL_WARP_MILK_ROAD,
    OWL_WARP_GREAT_BAY_COAST,
    OWL_WARP_ZORA_CAPE,
    OWL_WARP_WOODFALL,

    // OWL_WARP_IKANA_CANYON
    OWL_WARP_STONE_TOWER,
    OWL_WARP_MOUNTAIN_VILLAGE,
    OWL_WARP_SNOWHEAD,
    OWL_WARP_CLOCK_TOWN,
    OWL_WARP_SOUTHERN_SWAMP,
    OWL_WARP_ZORA_CAPE,
    OWL_WARP_WOODFALL,
    OWL_WARP_GREAT_BAY_COAST,
    OWL_WARP_MILK_ROAD,

    // OWL_WARP_STONE_TOWER
    OWL_WARP_MOUNTAIN_VILLAGE,
    OWL_WARP_SNOWHEAD,
    OWL_WARP_SOUTHERN_SWAMP,
    OWL_WARP_IKANA_CANYON,
    OWL_WARP_WOODFALL,
    OWL_WARP_ZORA_CAPE,
    OWL_WARP_MILK_ROAD,
    OWL_WARP_GREAT_BAY_COAST,
    OWL_WARP_CLOCK_TOWN,
};

static s16 sOwlWarpPauseNavigationDown[] = {
    // OWL_WARP_GREAT_BAY_COAST
    OWL_WARP_ZORA_CAPE,
    OWL_WARP_WOODFALL,
    OWL_WARP_SOUTHERN_SWAMP,
    OWL_WARP_SNOWHEAD,
    OWL_WARP_MOUNTAIN_VILLAGE,
    OWL_WARP_STONE_TOWER,
    OWL_WARP_CLOCK_TOWN,
    OWL_WARP_IKANA_CANYON,
    OWL_WARP_MILK_ROAD,

    // OWL_WARP_ZORA_CAPE
    OWL_WARP_SNOWHEAD,
    OWL_WARP_MOUNTAIN_VILLAGE,
    OWL_WARP_GREAT_BAY_COAST,
    OWL_WARP_STONE_TOWER,
    OWL_WARP_CLOCK_TOWN,
    OWL_WARP_MILK_ROAD,
    OWL_WARP_IKANA_CANYON,
    OWL_WARP_SOUTHERN_SWAMP,
    OWL_WARP_WOODFALL,

    // OWL_WARP_SNOWHEAD
    OWL_WARP_MOUNTAIN_VILLAGE,
    OWL_WARP_CLOCK_TOWN,
    OWL_WARP_MILK_ROAD,
    OWL_WARP_WOODFALL,
    OWL_WARP_SOUTHERN_SWAMP,
    OWL_WARP_ZORA_CAPE,
    OWL_WARP_GREAT_BAY_COAST,
    OWL_WARP_IKANA_CANYON,
    OWL_WARP_STONE_TOWER,

    // OWL_WARP_MOUNTAIN_VILLAGE
    OWL_WARP_CLOCK_TOWN,
    OWL_WARP_MILK_ROAD,
    OWL_WARP_WOODFALL,
    OWL_WARP_SOUTHERN_SWAMP,
    OWL_WARP_IKANA_CANYON,
    OWL_WARP_ZORA_CAPE,
    OWL_WARP_GREAT_BAY_COAST,
    OWL_WARP_STONE_TOWER,
    OWL_WARP_SNOWHEAD,

    // OWL_WARP_CLOCK_TOWN
    OWL_WARP_MILK_ROAD,
    OWL_WARP_WOODFALL,
    OWL_WARP_SOUTHERN_SWAMP,
    OWL_WARP_ZORA_CAPE,
    OWL_WARP_IKANA_CANYON,
    OWL_WARP_GREAT_BAY_COAST,
    OWL_WARP_SNOWHEAD,
    OWL_WARP_MOUNTAIN_VILLAGE,
    OWL_WARP_STONE_TOWER,

    // OWL_WARP_MILK_ROAD
    OWL_WARP_WOODFALL,
    OWL_WARP_SOUTHERN_SWAMP,
    OWL_WARP_ZORA_CAPE,
    OWL_WARP_SNOWHEAD,
    OWL_WARP_MOUNTAIN_VILLAGE,
    OWL_WARP_CLOCK_TOWN,
    OWL_WARP_GREAT_BAY_COAST,
    OWL_WARP_STONE_TOWER,
    OWL_WARP_IKANA_CANYON,

    // OWL_WARP_WOODFALL
    OWL_WARP_ZORA_CAPE,
    OWL_WARP_SNOWHEAD,
    OWL_WARP_MOUNTAIN_VILLAGE,
    OWL_WARP_CLOCK_TOWN,
    OWL_WARP_STONE_TOWER,
    OWL_WARP_IKANA_CANYON,
    OWL_WARP_MILK_ROAD,
    OWL_WARP_GREAT_BAY_COAST,
    OWL_WARP_SOUTHERN_SWAMP,

    // OWL_WARP_SOUTHERN_SWAMP
    OWL_WARP_STONE_TOWER,
    OWL_WARP_SNOWHEAD,
    OWL_WARP_MOUNTAIN_VILLAGE,
    OWL_WARP_IKANA_CANYON,
    OWL_WARP_CLOCK_TOWN,
    OWL_WARP_ZORA_CAPE,
    OWL_WARP_MILK_ROAD,
    OWL_WARP_GREAT_BAY_COAST,
    OWL_WARP_WOODFALL,

    // OWL_WARP_IKANA_CANYON
    OWL_WARP_SOUTHERN_SWAMP,
    OWL_WARP_WOODFALL,
    OWL_WARP_ZORA_CAPE,
    OWL_WARP_STONE_TOWER,
    OWL_WARP_MILK_ROAD,
    OWL_WARP_GREAT_BAY_COAST,
    OWL_WARP_SNOWHEAD,
    OWL_WARP_MOUNTAIN_VILLAGE,
    OWL_WARP_CLOCK_TOWN,

    // OWL_WARP_STONE_TOWER
    OWL_WARP_IKANA_CANYON,
    OWL_WARP_SOUTHERN_SWAMP,
    OWL_WARP_MOUNTAIN_VILLAGE,
    OWL_WARP_CLOCK_TOWN,
    OWL_WARP_SNOWHEAD,
    OWL_WARP_WOODFALL,
    OWL_WARP_MILK_ROAD,
    OWL_WARP_ZORA_CAPE,
    OWL_WARP_GREAT_BAY_COAST,
};

// Arrays indicating how many points are considered to be to the left or right of each Owl Warp Statue.
static s16 sOwlWarpCutoffLeft[] = {
    0, // OWL_WARP_GREAT_BAY_COAST
    1, // OWL_WARP_ZORA_CAPE
    3, // OWL_WARP_SNOWHEAD
    5, // OWL_WARP_MOUNTAIN_VILLAGE
    3, // OWL_WARP_CLOCK_TOWN
    2, // OWL_WARP_MILK_ROAD
    4, // OWL_WARP_WOODFALL
    7, // OWL_WARP_SOUTHERN_SWAMP
    8, // OWL_WARP_IKANA_CANYON
    9, // OWL_WARP_STONE_TOWER
};

static s16 sOwlWarpCutoffRight[] = {
    9, // OWL_WARP_GREAT_BAY_COAST
    8, // OWL_WARP_ZORA_CAPE
    4, // OWL_WARP_SNOWHEAD
    3, // OWL_WARP_MOUNTAIN_VILLAGE
    4, // OWL_WARP_CLOCK_TOWN
    7, // OWL_WARP_MILK_ROAD
    3, // OWL_WARP_WOODFALL
    2, // OWL_WARP_SOUTHERN_SWAMP
    1, // OWL_WARP_IKANA_CANYON
    0, // OWL_WARP_STONE_TOWER
};

// Expanded tables derived from the fifteen world-map cursor coordinates.
static constexpr s16 EXPANDED_NAV_BLOCK_SIZE = 14;
static s16 sExpandedNavigationLeft[] = {
    14, 1, 5, 4, 11, 12, 6, 3, 2, 7, 10, 8, 9, 13, // owl ID 0
    0, 14, 12, 5, 6, 4, 11, 7, 3, 2, 10, 8, 9, 13, // owl ID 1
    14, 0, 1, 5, 12, 3, 4, 11, 6, 10, 13, 7, 9, 8, // owl ID 2
    2, 0, 5, 14, 1, 4, 12, 11, 6, 10, 7, 9, 8, 13, // owl ID 3
    5, 0, 1, 14, 12, 2, 11, 3, 6, 7, 10, 8, 9, 13, // owl ID 4
    0, 1, 14, 12, 4, 11, 6, 3, 2, 7, 10, 8, 9, 13, // owl ID 5
    5, 12, 1, 0, 14, 4, 2, 3, 11, 7, 10, 8, 9, 13, // owl ID 6
    6, 5, 11, 4, 12, 1, 0, 3, 14, 2, 10, 8, 9, 13, // owl ID 7
    10, 11, 4, 7, 6, 5, 3, 2, 12, 1, 0, 14, 9, 13, // owl ID 8
    10, 3, 11, 2, 4, 13, 8, 5, 6, 7, 0, 14, 1, 12, // owl ID 9
    11, 3, 4, 2, 5, 6, 0, 1, 14, 7, 12, 8, 9, 13, // owl ID 10
    4, 5, 0, 1, 14, 12, 2, 3, 6, 7, 10, 8, 9, 13, // owl ID 11
    1, 0, 14, 5, 6, 4, 11, 7, 3, 2, 10, 8, 9, 13, // owl ID 12
    2, 3, 4, 11, 5, 10, 14, 0, 6, 1, 12, 7, 8, 9, // owl ID 13
    0, 1, 3, 2, 5, 4, 11, 12, 6, 10, 7, 9, 8, 13, // owl ID 14
};
static s16 sExpandedNavigationRight[] = {
    5, 4, 1, 11, 6, 3, 12, 2, 7, 10, 8, 9, 13, 14, // owl ID 0
    12, 6, 5, 4, 11, 7, 8, 3, 10, 9, 2, 13, 0, 14, // owl ID 1
    3, 13, 10, 9, 8, 7, 11, 4, 6, 5, 12, 14, 0, 1, // owl ID 2
    10, 9, 13, 8, 7, 11, 6, 2, 4, 5, 12, 14, 0, 1, // owl ID 3
    11, 10, 8, 7, 9, 13, 3, 6, 5, 2, 12, 0, 1, 14, // owl ID 4
    4, 11, 6, 7, 10, 8, 9, 3, 13, 2, 12, 1, 0, 14, // owl ID 5
    7, 8, 10, 9, 13, 11, 4, 5, 3, 12, 2, 1, 0, 14, // owl ID 6
    8, 9, 10, 13, 6, 11, 4, 5, 3, 12, 2, 1, 0, 14, // owl ID 7
    9, 13, 10, 7, 11, 4, 6, 3, 5, 2, 12, 0, 1, 14, // owl ID 8
    13, 8, 10, 3, 7, 11, 2, 4, 6, 5, 12, 14, 0, 1, // owl ID 9
    9, 8, 13, 7, 11, 3, 4, 2, 6, 5, 12, 0, 1, 14, // owl ID 10
    10, 8, 7, 9, 13, 4, 6, 3, 5, 2, 12, 0, 1, 14, // owl ID 11
    6, 7, 8, 10, 9, 11, 13, 4, 3, 5, 2, 1, 0, 14, // owl ID 12
    9, 8, 10, 2, 3, 7, 11, 4, 6, 5, 12, 14, 0, 1, // owl ID 13
    0, 2, 5, 3, 4, 11, 6, 10, 7, 8, 13, 9, 1, 12, // owl ID 14
};
static s16 sExpandedNavigationUp[] = {
    14, 2, 3, 13, 9, 10, 4, 11, 8, 5, 1, 6, 7, 12, // owl ID 0
    0, 14, 5, 2, 3, 4, 11, 13, 10, 9, 8, 6, 7, 12, // owl ID 1
    13, 3, 4, 9, 11, 10, 5, 14, 8, 6, 0, 7, 12, 1, // owl ID 2
    2, 13, 4, 9, 11, 10, 5, 14, 8, 6, 7, 0, 12, 1, // owl ID 3
    3, 2, 13, 10, 9, 14, 11, 5, 8, 6, 7, 0, 12, 1, // owl ID 4
    4, 3, 2, 11, 13, 10, 9, 14, 8, 0, 6, 7, 12, 1, // owl ID 5
    11, 4, 5, 3, 2, 10, 8, 13, 9, 14, 0, 7, 12, 1, // owl ID 6
    8, 11, 10, 4, 3, 9, 2, 13, 5, 14, 0, 6, 12, 1, // owl ID 7
    10, 9, 13, 3, 2, 14, 4, 11, 7, 5, 6, 0, 12, 1, // owl ID 8
    13, 2, 10, 3, 8, 11, 4, 7, 5, 14, 6, 0, 12, 1, // owl ID 9
    13, 9, 2, 3, 14, 8, 11, 4, 7, 5, 6, 0, 12, 1, // owl ID 10
    4, 3, 2, 13, 10, 9, 14, 5, 8, 6, 7, 0, 12, 1, // owl ID 11
    6, 5, 4, 11, 3, 7, 2, 10, 0, 8, 1, 13, 9, 14, // owl ID 12
    9, 2, 10, 3, 8, 11, 4, 7, 5, 14, 6, 0, 12, 1, // owl ID 13
    2, 13, 0, 3, 4, 5, 9, 11, 10, 1, 8, 6, 7, 12, // owl ID 14
};
static s16 sExpandedNavigationDown[] = {
    1, 12, 6, 7, 5, 14, 4, 11, 8, 3, 10, 2, 9, 13, // owl ID 0
    12, 0, 6, 5, 7, 4, 11, 14, 8, 3, 10, 2, 9, 13, // owl ID 1
    3, 4, 11, 5, 6, 7, 12, 10, 8, 1, 0, 9, 14, 13, // owl ID 2
    4, 11, 5, 6, 7, 12, 8, 1, 10, 0, 2, 9, 14, 13, // owl ID 3
    5, 11, 6, 7, 12, 1, 0, 8, 3, 10, 2, 9, 14, 13, // owl ID 4
    6, 12, 7, 1, 4, 11, 0, 8, 3, 10, 2, 9, 14, 13, // owl ID 5
    12, 1, 7, 5, 11, 4, 8, 3, 10, 0, 2, 9, 13, 14, // owl ID 6
    12, 6, 1, 8, 11, 5, 4, 10, 3, 9, 0, 2, 13, 14, // owl ID 7
    7, 6, 12, 1, 5, 0, 10, 11, 4, 9, 3, 13, 2, 14, // owl ID 8
    8, 10, 7, 6, 11, 12, 4, 5, 1, 0, 13, 3, 2, 14, // owl ID 9
    8, 7, 6, 11, 12, 5, 4, 1, 0, 9, 3, 13, 2, 14, // owl ID 10
    6, 7, 5, 12, 1, 0, 4, 8, 3, 10, 2, 9, 13, 14, // owl ID 11
    6, 1, 5, 7, 4, 11, 0, 8, 3, 10, 2, 9, 14, 13, // owl ID 12
    9, 10, 8, 7, 11, 6, 4, 5, 3, 12, 1, 0, 2, 14, // owl ID 13
    0, 1, 12, 6, 5, 7, 4, 11, 8, 10, 3, 2, 9, 13, // owl ID 14
};
static s16 sExpandedCutoffLeft[] = { 1, 2, 5, 7, 6, 4, 8, 10, 12, 14, 11, 9, 3, 13, 0 };
static s16 sExpandedCutoffRight[] = { 13, 12, 9, 7, 8, 10, 6, 4, 2, 0, 3, 5, 11, 1, 14 };
static bool HasExpandedOwls(PauseContext* pauseCtx) {
    for (int i = 10; i <= OWL_WARP_PIRATES_FORTRESS; i++) {
        if (pauseCtx->worldMapPoints[i]) return true;
    }
    return false;
}

// Used by Pause Owl Warp to figure out when to move the cursor onto the page turn slots.
extern "C" bool BetterOwlWarp_IsCutoffOnSide(s16 cursorPoint, PauseContext* pauseCtx) {
    if (!CVAR || cursorPoint < OWL_WARP_GREAT_BAY_COAST || cursorPoint > OWL_WARP_PIRATES_FORTRESS) {
        return false;
    }

    bool goingLeft = pauseCtx->stickAdjX < -30;
    bool goingRight = pauseCtx->stickAdjX > 30;

    if (CVarGetInteger("gModes.MirroredWorld.State", 0) && (goingLeft || goingRight)) {
        goingLeft = !goingLeft;
        goingRight = !goingRight;
    }

    bool expanded = HasExpandedOwls(pauseCtx) || cursorPoint > OWL_WARP_STONE_TOWER;
    s16 blockSize = expanded ? EXPANDED_NAV_BLOCK_SIZE : NAV_BLOCK_SIZE;
    s16* navPoints;
    s16 navCutoff;

    if (goingLeft) {
        navPoints = expanded ? sExpandedNavigationLeft : sOwlWarpPauseNavigationLeft;
        navCutoff = (expanded ? sExpandedCutoffLeft : sOwlWarpCutoffLeft)[cursorPoint];
    } else if (goingRight) {
        navPoints = expanded ? sExpandedNavigationRight : sOwlWarpPauseNavigationRight;
        navCutoff = (expanded ? sExpandedCutoffRight : sOwlWarpCutoffRight)[cursorPoint];
    } else {
        return false;
    }

    s16 i;
    for (i = 0; i < navCutoff; i++) {
        if (pauseCtx->worldMapPoints[navPoints[cursorPoint * blockSize + i]]) {
            return false;
        }
    }

    return true;
}

extern "C" bool BetterOwlWarp_NextCursorPoint(s16* cursorPoint, PauseContext* pauseCtx) {
    if (*cursorPoint < OWL_WARP_GREAT_BAY_COAST || *cursorPoint > OWL_WARP_PIRATES_FORTRESS) {
        return false;
    }

    bool goingLeft = pauseCtx->stickAdjX < -30;
    bool goingRight = pauseCtx->stickAdjX > 30;
    bool goingUp = pauseCtx->stickAdjY > 30;
    bool goingDown = pauseCtx->stickAdjY < -30;

    bool expanded = HasExpandedOwls(pauseCtx) || *cursorPoint > OWL_WARP_STONE_TOWER;
    s16 blockSize = expanded ? EXPANDED_NAV_BLOCK_SIZE : NAV_BLOCK_SIZE;
    s16 tmpCursorPoint, i;
    s16* navigationMap = nullptr;

    if (CVarGetInteger("gModes.MirroredWorld.State", 0) && (goingLeft || goingRight)) {
        goingLeft = !goingLeft;
        goingRight = !goingRight;
    }

    if (goingLeft) {
        navigationMap = expanded ? sExpandedNavigationLeft : sOwlWarpPauseNavigationLeft;
    } else if (goingRight) {
        navigationMap = expanded ? sExpandedNavigationRight : sOwlWarpPauseNavigationRight;
    } else if (goingUp) {
        navigationMap = expanded ? sExpandedNavigationUp : sOwlWarpPauseNavigationUp;
    } else if (goingDown) {
        navigationMap = expanded ? sExpandedNavigationDown : sOwlWarpPauseNavigationDown;
    }

    if (navigationMap == nullptr) {
        return false;
    }

    for (i = 0; i < blockSize; i++) {
        tmpCursorPoint = navigationMap[*cursorPoint * blockSize + i];
        if (pauseCtx->worldMapPoints[tmpCursorPoint]) {
            *cursorPoint = tmpCursorPoint;
            break;
        }
    }

    return true;
}

static void HandleBetterOwlWarpMenuNavigation() {
    PauseContext* pauseCtx = &gPlayState->pauseCtx;
    s16 nextCursorPoint = pauseCtx->cursorPoint[PAUSE_WORLD_MAP];

    if (BetterOwlWarp_NextCursorPoint(&nextCursorPoint, pauseCtx)) {
        pauseCtx->cursorShrinkRate = 4.0f;

        if (pauseCtx->cursorPoint[PAUSE_WORLD_MAP] != nextCursorPoint) {
            pauseCtx->cursorPoint[PAUSE_WORLD_MAP] = nextCursorPoint;
            Audio_PlaySfx(NA_SE_SY_CURSOR);
        }
    }

    pauseCtx->cursorColorSet = PAUSE_CURSOR_COLOR_SET_BLUE;
    pauseCtx->cursorItem[PAUSE_MAP] = nextCursorPoint == OWL_WARP_DEKU_PALACE ? REGION_DEKU_PALACE : sOwlWarpPauseItems[nextCursorPoint] - ITEM_MAP_POINT_GREAT_BAY;
    pauseCtx->cursorSlot[PAUSE_MAP] = 31 + (nextCursorPoint == OWL_WARP_DEKU_PALACE ? REGION_DEKU_PALACE : nextCursorPoint);
}

static void RegisterBetterOwlWarpMenu() {
    COND_VB_SHOULD(VB_OWL_WARP_MENU_USE_LINEAR_CURSOR, CVAR, {
        if (*should) {
            HandleBetterOwlWarpMenuNavigation();
            *should = false;
        }
    });
}

static RegisterShipInitFunc initFunc(RegisterBetterOwlWarpMenu, { CVAR_NAME });

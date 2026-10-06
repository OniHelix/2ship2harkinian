#!/usr/bin/env python3
"""Apply the five validated native owl-statue room edits to mm.o2r.

The offsets below address OTR-exported Room resources, not ROM data. Every
edit checks the expected vanilla byte first so extraction changes fail loudly
instead of producing a corrupt archive.
"""
from __future__ import annotations
import sys
import math
import struct
import zipfile
from pathlib import Path

# name: (object_count_offset, expected object count,
#        object_insert_offset, actor_count_offset, expected actor count,
#        actor_insert_offset, serialized Obj_Warpstone record)
ROOMS = {
 "scenes/nonmq/Z2_BOTI/Z2_BOTI_room_01":
   (1427,0x0C,1455,1459,0x33,2279,bytes.fromhex("230276004301c4f7070016007f000a00")),
 "scenes/nonmq/Z2_TENMON_DAI/Z2_TENMON_DAI_room_01":
   (1644,0x0F,1678,1682,0x0A,1846,bytes.fromhex("2302fdff7fff62fe070010007f000b00")),
 "scenes/nonmq/Z2_22DEKUCITY/Z2_22DEKUCITY_room_00":
   (2851,0x11,2889,2893,0x2A,3569,bytes.fromhex("230204ff0000ca0b07009e1a7f000c00")),
 "scenes/nonmq/Z2_16GORON_HOUSE/Z2_16GORON_HOUSE_room_00":
   (1812,0x05,1826,1830,0x2B,2522,bytes.fromhex("2302cdfd7affecfd0700130b7f000d00")),
 "scenes/nonmq/Z2_TORIDE/Z2_TORIDE_room_00":
   (2102,0x0B,2128,2132,0x18,2520,bytes.fromhex("23024501c80038fd07000d877f000e00")),
}
OBJECT_SEK_SERIALIZED = bytes.fromhex("7001") # 0x0170, little-endian OTR serialization

DEKU_SCENE = "scenes/nonmq/Z2_22DEKUCITY/Z2_22DEKUCITY"
DEKU_START_HEADER = bytes.fromhex("000000000b000000")  # command 0, 11 Start Positions
DEKU_ENTRANCE_HEADER = bytes.fromhex("060000000c000000")  # command 6, 12 Entrance List slots

# Added physical Deku Palace owl: (1279, 0, 3018), yaw 0x1A9E.
# Vanilla soaring Start Positions consistently use Player actor 0, rot.x 7,
# rot.z 0x7F, and params 0x06FF. Place Link 70 units in front of the owl
# and face him back toward it.
DEKU_OWL_X = 1279
DEKU_OWL_Y = 0
DEKU_OWL_Z = 3018
DEKU_OWL_YAW = 0x1A9E
SOARING_PARAMS = 0x06FF

def patch_deku_scene(payload: bytes) -> bytes:
    start_off = payload.find(DEKU_START_HEADER)
    entrance_off = payload.find(DEKU_ENTRANCE_HEADER)
    if start_off < 0 or payload.find(DEKU_START_HEADER, start_off + 1) >= 0:
        raise RuntimeError("could not uniquely locate Deku Palace Start Position List")
    if entrance_off < 0 or payload.find(DEKU_ENTRANCE_HEADER, entrance_off + 1) >= 0:
        raise RuntimeError("could not uniquely locate Deku Palace Entrance List")

    # The archive already contains Entrance List slot 11 as a dummy (spawn 0, room 0).
    # Replace it with spawn 11 / room 0; do not increase the Entrance List count.
    entrance_11 = entrance_off + 8 + (11 * 2)
    if payload[entrance_11:entrance_11 + 2] != bytes((0, 0)):
        raise RuntimeError("unexpected Deku Palace Entrance List slot 11")

    angle = DEKU_OWL_YAW * (2.0 * math.pi / 65536.0)
    spawn_x = round(DEKU_OWL_X + math.sin(angle) * 70.0)
    spawn_z = round(DEKU_OWL_Z + math.cos(angle) * 70.0)
    spawn_yaw = (DEKU_OWL_YAW + 0x8000) & 0xFFFF
    if spawn_yaw >= 0x8000:
        spawn_yaw -= 0x10000

    start_11 = struct.pack(
        "<HhhhhhhH",
        0,              # Player actor
        spawn_x,
        DEKU_OWL_Y,
        spawn_z,
        7,              # matches vanilla 0x06FF soaring starts
        spawn_yaw,
        0x7F,           # matches vanilla 0x06FF soaring starts
        SOARING_PARAMS,
    )

    p = bytearray(payload)
    p[entrance_11:entrance_11 + 2] = bytes((11, 0))

    # Expand only the Start Position List from 11 to 12 and append index 11.
    struct.pack_into("<I", p, start_off + 4, 12)
    start_insert = start_off + 8 + (11 * 16)
    p[start_insert:start_insert] = start_11
    return bytes(p)

def patch(payload: bytes, spec) -> bytes:
    oo, oc, oi, ao, ac, ai, actor = spec
    if payload[oo] != oc or payload[ao] != ac:
        raise RuntimeError(f"unexpected vanilla counts: objects={payload[oo]:02X}, actors={payload[ao]:02X}")
    # Insert object first. Actor count/insert offsets consequently move by 2.
    p = bytearray(payload)
    p[oo] = oc + 1
    p[oi:oi] = OBJECT_SEK_SERIALIZED
    ao += 2; ai += 2
    if p[ao] != ac:
        raise RuntimeError("actor count moved unexpectedly after object insertion")
    p[ao] = ac + 1
    # Serialized Room ends in command 0x14; insert the native actor before it.
    if p[ai:ai+1] != b"\x14":
        raise RuntimeError("expected room terminator at actor insertion point")
    p[ai:ai] = actor
    return bytes(p)

def main() -> int:
    archive = Path(sys.argv[1] if len(sys.argv)>1 else "mm.o2r")
    temp = archive.with_suffix(archive.suffix+".tmp")
    found=set()
    scene_patched = False
    with zipfile.ZipFile(archive,"r") as zin, zipfile.ZipFile(temp,"w") as zout:
        for info in zin.infolist():
            data=zin.read(info.filename)
            if info.filename in ROOMS:
                data=patch(data,ROOMS[info.filename]); found.add(info.filename)
            if info.filename == DEKU_SCENE:
                data=patch_deku_scene(data); scene_patched = True
            zout.writestr(info,data)
    missing=set(ROOMS)-found
    if missing:
        temp.unlink(missing_ok=True)
        raise RuntimeError("missing room resources: "+", ".join(sorted(missing)))
    if not scene_patched:
        temp.unlink(missing_ok=True)
        raise RuntimeError("missing Deku Palace scene resource")
    temp.replace(archive)
    print(f"Expanded Owls: patched {len(found)} room resources plus Deku Palace scene entrance 11 in {archive}")
    return 0
if __name__=="__main__": raise SystemExit(main())

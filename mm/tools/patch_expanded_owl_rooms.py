#!/usr/bin/env python3
"""Apply the five validated native owl-statue room edits to mm.o2r.

The offsets below address OTR-exported Room resources, not ROM data. Every
edit checks the expected vanilla byte first so extraction changes fail loudly
instead of producing a corrupt archive.
"""
from __future__ import annotations
import sys
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
    with zipfile.ZipFile(archive,"r") as zin, zipfile.ZipFile(temp,"w") as zout:
        for info in zin.infolist():
            data=zin.read(info.filename)
            if info.filename in ROOMS:
                data=patch(data,ROOMS[info.filename]); found.add(info.filename)
            zout.writestr(info,data)
    missing=set(ROOMS)-found
    if missing:
        temp.unlink(missing_ok=True)
        raise RuntimeError("missing room resources: "+", ".join(sorted(missing)))
    temp.replace(archive)
    print(f"Expanded Owls: patched {len(found)} room resources in {archive}")
    return 0
if __name__=="__main__": raise SystemExit(main())

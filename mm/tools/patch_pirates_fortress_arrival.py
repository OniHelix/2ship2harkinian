#!/usr/bin/env python3
"""Set the Pirates Fortress owl soaring arrival in an existing mm.o2r archive."""
from pathlib import Path
import struct
import sys
import zipfile

SCENE = "scenes/nonmq/Z2_TORIDE/Z2_TORIDE"
HEADER = bytes.fromhex("0000000007000000")
OLD = (0, 218, 200, -760, 7, 0, 127, 0x0E00)
NEW = (0, 255, 200, -712, 7, 0x4000, 127, 0x0E00)

def main():
    archive = Path(sys.argv[1] if len(sys.argv) > 1 else "mm.o2r")
    temp = archive.with_suffix(archive.suffix + ".tmp")
    patched = False
    try:
        with zipfile.ZipFile(archive) as src, zipfile.ZipFile(temp, "w") as dst:
            for info in src.infolist():
                data = src.read(info.filename)
                if info.filename == SCENE:
                    offset = data.find(HEADER)
                    if offset < 0 or data.find(HEADER, offset + 1) >= 0:
                        raise RuntimeError("Pirates Fortress start-position header not unique")
                    offset += 8 + 6 * 16
                    values = struct.unpack_from("<HhhhhhhH", data, offset)
                    if values not in (OLD, NEW):
                        raise RuntimeError(f"Unexpected Pirates Fortress entrance 6: {values}")
                    if values == OLD:
                        b = bytearray(data)
                        struct.pack_into("<HhhhhhhH", b, offset, *NEW)
                        data = bytes(b)
                    patched = True
                dst.writestr(info, data)
        if not patched:
            raise RuntimeError("Pirates Fortress scene missing")
        temp.replace(archive)
    finally:
        temp.unlink(missing_ok=True)
    print("Pirates Fortress entrance 6: (255, 200, -712), yaw 0x4000")

if __name__ == "__main__":
    main()

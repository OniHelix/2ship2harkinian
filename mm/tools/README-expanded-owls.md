# Expanded owl room assets

The extraction-time patcher applies the five room-resource edits already validated in Battler Bravo: Ikana Graveyard, Astral Observatory, Deku Palace, Goron Shrine, and Pirates' Fortress. It appends `OBJECT_SEK` and a native `Obj_Warpstone` record to each exported Room resource.

The top-level optional `CMake/GlobalSettingsInclude.cmake` attaches this transform to the end of the existing `ExtractAssets` target, so a locally extracted `mm.o2r` receives the room changes automatically. The patcher checks the expected vanilla counts and room terminators and aborts instead of silently patching an unexpected upstream layout.

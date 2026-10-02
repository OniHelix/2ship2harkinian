# Project-local post-extraction asset transforms.
# This file is included before ExtractAssets is declared, so defer attaching
# the command until the top-level CMakeLists has finished defining targets.
function(add_expanded_owl_asset_patch)
    add_custom_command(
        TARGET ExtractAssets POST_BUILD
        COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_SOURCE_DIR}/mm/tools/patch_expanded_owl_rooms.py
                ${CMAKE_SOURCE_DIR}/mm/mm.o2r
        COMMENT "Appending five native expanded owl statues to extracted room assets..."
        VERBATIM
    )
endfunction()
cmake_language(DEFER CALL add_expanded_owl_asset_patch)

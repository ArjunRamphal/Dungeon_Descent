add_executable(dungeon_asset_packer tools/PackAssets.cpp)
target_link_libraries(dungeon_asset_packer PRIVATE dungeon_assets)

file(GLOB_RECURSE DD_ASSET_INPUTS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/assets/*")
file(GLOB_RECURSE DD_NOTICE_INPUTS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/notices/*")
set(DD_ASSET_BUNDLE_PATH "${CMAKE_CURRENT_BINARY_DIR}/embedded/assets.pak")
add_custom_command(OUTPUT "${DD_ASSET_BUNDLE_PATH}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/embedded"
    COMMAND dungeon_asset_packer "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/notices" "${DD_ASSET_BUNDLE_PATH}"
    DEPENDS dungeon_asset_packer ${DD_ASSET_INPUTS} ${DD_NOTICE_INPUTS}
    VERBATIM)
add_custom_target(dungeon_embedded_assets DEPENDS "${DD_ASSET_BUNDLE_PATH}")
add_dependencies(Dungeon_Descent dungeon_embedded_assets)
target_sources(Dungeon_Descent PRIVATE src/DungeonDescent/portable/EmbeddedAssets.cpp)
target_link_libraries(Dungeon_Descent PRIVATE dungeon_assets)
target_compile_definitions(Dungeon_Descent PRIVATE DD_EMBED_ASSETS)
if(WIN32)
    enable_language(RC)
    configure_file(cmake/EmbeddedAssets.rc.in embedded/GameAssets.rc @ONLY)
    set(DD_EMBED_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/embedded/GameAssets.rc")
    target_link_libraries(Dungeon_Descent PRIVATE shell32 ole32)
    get_filename_component(DD_COMPILER_DIRECTORY "${CMAKE_CXX_COMPILER}" DIRECTORY)
    find_program(DD_DUMPBIN NAMES dumpbin HINTS "${DD_COMPILER_DIRECTORY}" REQUIRED)
    add_custom_command(TARGET Dungeon_Descent POST_BUILD
        COMMAND ${CMAKE_COMMAND} "-DEXECUTABLE=$<TARGET_FILE:Dungeon_Descent>" "-DDUMPBIN=${DD_DUMPBIN}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/VerifyStandalone.cmake"
        VERBATIM)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    enable_language(ASM)
    configure_file(cmake/EmbeddedAssets.S.in embedded/GameAssets.S @ONLY)
    set(DD_EMBED_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/embedded/GameAssets.S")
else()
    message(FATAL_ERROR "Embedded standalone builds currently support Windows and Linux")
endif()
set_source_files_properties("${DD_EMBED_SOURCE}" PROPERTIES OBJECT_DEPENDS "${DD_ASSET_BUNDLE_PATH}")
target_sources(Dungeon_Descent PRIVATE "${DD_EMBED_SOURCE}")

# It's recommended to set a minimum CMake version.
# If you use CMake features from higher versions, update this to match.
cmake_minimum_required(VERSION 3.21)

#global test options
set(BUILD_TESTS OFF CACHE BOOL "Build unit tests for CommonLibVR." FORCE)
set(ENABLE_SKYRIM_VR OFF CACHE BOOL "" FORCE)

# Set your project name. This will be the name of your SKSE .dll file.
project(EldenParry VERSION 0.0.1 LANGUAGES CXX)

#set(OUTPUT_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/output")
set(OUTPUT_FOLDER "D:/Skyrim SE MO2/mods/Elden Parry Remake")

add_subdirectory(extern/CommonLibVR-ng)
include(extern/CommonLibVR-ng/cmake/CommonLibSSE.cmake)

# Setup your SKSE plugin as an SKSE plugin!
# find_package(CommonLibSSE CONFIG REQUIRED)

set(SOURCES
    src/plugin.cpp
    src/hooks.h
    src/hooks.cpp
    src/form_config.h
    src/form_config.cpp
    src/utils.h
    src/settings.h
    src/settings.cpp
)

add_commonlibsse_plugin(${PROJECT_NAME} SOURCES ${SOURCES}) # <--- specifies plugin.cpp

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23) # <--- use C++23 standard
target_precompile_headers(${PROJECT_NAME} PRIVATE 
    src/PCH.h
) 

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/extern
)

# When your SKSE .dll is compiled, this will automatically copy the .dll into your mods folder.
# Only works if you configure DEPLOY_ROOT above (or set the SKYRIM_MODS_FOLDER environment variable)
if(DEFINED OUTPUT_FOLDER)
    # If you specify an <OUTPUT_FOLDER> (including via environment variables)
    # then we'll copy your mod files into Skyrim or a mod manager for you!

    # Copy the SKSE plugin .dll files into the SKSE/Plugins/ folder
    set(DLL_FOLDER "${OUTPUT_FOLDER}/SKSE/Plugins")

    message(STATUS "SKSE plugin output folder: ${DLL_FOLDER}")

    add_custom_command(
        TARGET "${PROJECT_NAME}"
        POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${DLL_FOLDER}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:${PROJECT_NAME}>" "${DLL_FOLDER}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
        VERBATIM
    )
    add_custom_command(
            TARGET "${PROJECT_NAME}"
            POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${DLL_FOLDER}"
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_PDB_FILE:${PROJECT_NAME}>" "${DLL_FOLDER}/$<TARGET_PDB_FILE_NAME:${PROJECT_NAME}>"
            VERBATIM
        )
   
endif()

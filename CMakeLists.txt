cmake_minimum_required(VERSION 3.10)
project(lvgl C CXX)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Default build type" FORCE)
endif()

include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)

function(add_flag_if_supported flag)
    string(REPLACE "-" "_" flag_var ${flag})
    check_c_compiler_flag(${flag} HAS_${flag_var})

    if(HAS_${flag_var})
        add_compile_options(${flag})
    endif()
endfunction()

# AddressSanitizer
# add_compile_options(-fsanitize=address)
# add_link_options(-fsanitize=address)

if(NOT MSVC)
  add_flag_if_supported("-Wno-unterminated-string-initialization")
  add_flag_if_supported("-Wno-enum-enum-conversion")
  add_flag_if_supported("-Wno-error=enum-enum-conversion")
endif()

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(EOS_PLATFORM "Native" CACHE STRING "ElenixOS simulator platform")
set_property(CACHE EOS_PLATFORM PROPERTY STRINGS Native WASM)

set(EOS_IS_WASM OFF)
if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten" OR EMSCRIPTEN OR EOS_PLATFORM STREQUAL "WASM")
    set(EOS_IS_WASM ON)
    set(EOS_PLATFORM "WASM" CACHE STRING "ElenixOS simulator platform" FORCE)
else()
    set(EOS_PLATFORM "Native" CACHE STRING "ElenixOS simulator platform" FORCE)
endif()

message(STATUS "ElenixOS simulator platform: ${EOS_PLATFORM}")

if(EOS_IS_WASM)
    add_compile_definitions(EOS_SYS_ROOT_DIR="/")
else()
    add_compile_definitions(EOS_SYS_ROOT_DIR="${CMAKE_SOURCE_DIR}/fs/")
endif()

if(EOS_IS_WASM)
    set(ASSETS_DIR "/.simulator/img")
else()
    set(ASSETS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets/native/")
endif()
add_definitions(-DASSETS_PATH=\"${ASSETS_DIR}/\")

set(FREERTOS_PORT GCC_POSIX CACHE STRING "Port for FreeRTOS on Posix environment")
unset(USE_FREERTOS CACHE)
option(USE_FREERTOS "Enable FreeRTOS" OFF)

if(EOS_IS_WASM AND USE_FREERTOS)
    message(WARNING "USE_FREERTOS is not supported for WASM builds. Disabling it for EOS_PLATFORM=WASM.")
    set(USE_FREERTOS OFF CACHE BOOL "Enable FreeRTOS" FORCE)
endif()

if(USE_FREERTOS)
    message(STATUS "FreeRTOS is enabled")

    add_library(freertos_config INTERFACE)
    target_include_directories(freertos_config SYSTEM INTERFACE ${PROJECT_SOURCE_DIR}/config)
    target_compile_definitions(freertos_config INTERFACE projCOVERAGE_TEST=0)

    add_subdirectory(FreeRTOS)

    include_directories(${PROJECT_SOURCE_DIR}/FreeRTOS/include)
    include_directories(${PROJECT_SOURCE_DIR}/FreeRTOS/portable/ThirdParty/GCC/Posix)
    include_directories(${PROJECT_SOURCE_DIR}/config)

    file(GLOB FREERTOS_SOURCES
        "${PROJECT_SOURCE_DIR}/FreeRTOS/*.c"
        "${PROJECT_SOURCE_DIR}/FreeRTOS/portable/MemMang/heap_4.c"
        "${PROJECT_SOURCE_DIR}/FreeRTOS/portable/ThirdParty/GCC/Posix/*.c"
    )
else()
    message(STATUS "FreeRTOS is disabled")
    set(FREERTOS_SOURCES "")
endif()

include_directories(${PROJECT_SOURCE_DIR}/main/src)
include_directories(${PROJECT_SOURCE_DIR}/main/src/port/vibrator)
include_directories(${PROJECT_SOURCE_DIR}/main/src/port/display)
include_directories(${PROJECT_SOURCE_DIR}/main/src/port/power)
include_directories(${PROJECT_SOURCE_DIR}/main/src/port/time)
include_directories(${PROJECT_SOURCE_DIR}/main/src/port/sensor)
include_directories(${PROJECT_SOURCE_DIR}/main/src/port/battery)
include_directories(${PROJECT_SOURCE_DIR}/main/src/port/audio)
include_directories(${PROJECT_SOURCE_DIR}/main/inc)
include_directories(${PROJECT_SOURCE_DIR}/debugger_box)

# Kconfig: merge project defaults → build-dir .config → generated header
find_package(Python3 COMPONENTS Interpreter)
if(Python3_FOUND)
    set(KCONFIG_SCRIPT "${CMAKE_SOURCE_DIR}/scripts/kconfig.py")
    set(KCONFIG_DEFAULTS "${CMAKE_SOURCE_DIR}/main/src/config/defaults.conf")
    set(KCONFIG_DIR  "${CMAKE_BINARY_DIR}")
    set(KCONFIG_OUT  "${CMAKE_BINARY_DIR}/generated/eos_config_gen.h")

    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/generated")
    include_directories("${CMAKE_BINARY_DIR}/generated")

    execute_process(
        COMMAND ${Python3_EXECUTABLE} ${KCONFIG_SCRIPT} genconfig
                ${KCONFIG_DIR} ${KCONFIG_OUT} --defaults ${KCONFIG_DEFAULTS}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        RESULT_VARIABLE _kconfig_ret
    )
    if(NOT _kconfig_ret EQUAL 0)
        message(WARNING "kconfig genconfig failed – using kernel defaults")
    endif()

    add_custom_target(menuconfig
        COMMAND ${Python3_EXECUTABLE} ${KCONFIG_SCRIPT} menuconfig ${KCONFIG_DIR}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Launching Kconfig menuconfig for ${CMAKE_BINARY_DIR} ..."
        USES_TERMINAL
    )

endif()

option(LV_USE_DRAW_SDL "Use SDL draw unit" OFF)
option(LV_USE_LIBPNG "Use libpng to decode PNG" OFF)
option(LV_USE_LIBJPEG_TURBO "Use libjpeg turbo to decode JPEG" OFF)
option(LV_USE_FFMPEG "Use libffmpeg to display video using lv_ffmpeg" OFF)
option(LV_USE_FREETYPE "Use freetype library" OFF)

set(WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

if(EOS_IS_WASM)
    set(EOS_RUNTIME_OUTPUT_DIR "${CMAKE_BINARY_DIR}/bin")
    set(EOS_WASM_LIVE_SERVER_PORT "5500" CACHE STRING "Live Server port for WASM debugging")
    set(EOS_WASM_SOURCE_MAP_BASE
        "http://127.0.0.1:${EOS_WASM_LIVE_SERVER_PORT}/build-wasm/bin/"
        CACHE STRING "Public base URL used in generated WASM source maps"
    )

    add_compile_options(
        "-sUSE_SDL=2"
    )
else()
    set(EOS_RUNTIME_OUTPUT_DIR "${PROJECT_SOURCE_DIR}/bin")

    find_package(SDL2 REQUIRED)

    add_compile_options(
        -march=native
    )
endif()

add_compile_definitions($<$<BOOL:${LV_USE_DRAW_SDL}>:LV_USE_DRAW_SDL=1>)
add_compile_definitions($<$<BOOL:${LV_USE_LIBPNG}>:LV_USE_LIBPNG=1>)
add_compile_definitions($<$<BOOL:${LV_USE_LIBJPEG_TURBO}>:LV_USE_LIBJPEG_TURBO=1>)
add_compile_definitions($<$<BOOL:${LV_USE_FFMPEG}>:LV_USE_FFMPEG=1>)
add_compile_definitions(EOS_PLATFORM_${EOS_PLATFORM}=1)

set(LV_CONF_BUILD_DISABLE_EXAMPLES ON CACHE BOOL "Disable LVGL examples" FORCE)
set(LV_CONF_BUILD_DISABLE_DEMOS ON CACHE BOOL "Disable LVGL demos" FORCE)
add_subdirectory(lvgl)

if(EOS_IS_WASM)
    target_include_directories(lvgl PUBLIC
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/lvgl/src
        ${PROJECT_SOURCE_DIR}/ElenixOS/third_party/jerryscript/jerry-core/include
    )
else()
    target_include_directories(lvgl PUBLIC
        ${PROJECT_SOURCE_DIR}
        ${SDL2_INCLUDE_DIRS}
        ${PROJECT_SOURCE_DIR}/lvgl/src
        ${PROJECT_SOURCE_DIR}/ElenixOS/third_party/jerryscript/jerry-core/include
    )
endif()

if(TARGET lvgl_thorvg)
    if(MSVC)
        target_compile_options(lvgl_thorvg PRIVATE /FIstdint.h)
    else()
        target_compile_options(lvgl_thorvg PRIVATE -include stdint.h)
    endif()
endif()

set(EOS_MAIN_SOURCES
    ${PROJECT_SOURCE_DIR}/main/src/main.c
    ${PROJECT_SOURCE_DIR}/main/src/eos_port.c

    ${PROJECT_SOURCE_DIR}/main/src/mac_api.c
    ${PROJECT_SOURCE_DIR}/main/src/mouse_cursor_icon.c
    ${PROJECT_SOURCE_DIR}/main/src/eos_debug_anim.c
    ${PROJECT_SOURCE_DIR}/main/src/eos_debug_anim_panel.c
    ${PROJECT_SOURCE_DIR}/main/src/eos_debug_sensor_panel.c
    ${PROJECT_SOURCE_DIR}/main/src/port/vibrator/eos_port_vibrator.c
    ${PROJECT_SOURCE_DIR}/main/src/port/display/eos_port_display.c
    ${PROJECT_SOURCE_DIR}/main/src/port/power/eos_port_power.c
    ${PROJECT_SOURCE_DIR}/main/src/port/time/eos_port_time.c
    ${PROJECT_SOURCE_DIR}/main/src/port/sensor/eos_port_sensor.c
    ${PROJECT_SOURCE_DIR}/main/src/port/battery/eos_port_battery.c
    ${PROJECT_SOURCE_DIR}/main/src/port/audio/eos_port_audio.c
    ${PROJECT_SOURCE_DIR}/main/src/port/audio/eos_port_audio_decoder.c
    ${PROJECT_SOURCE_DIR}/main/src/port/audio/eos_port_audio_util.c
)

if(USE_FREERTOS)
    list(APPEND EOS_MAIN_SOURCES
        ${PROJECT_SOURCE_DIR}/main/src/freertos_main.cpp
        ${PROJECT_SOURCE_DIR}/main/src/FreeRTOS_Posix_Port.c
        ${FREERTOS_SOURCES}
    )
endif()

add_executable(main ${EOS_MAIN_SOURCES})
set_target_properties(main PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${EOS_RUNTIME_OUTPUT_DIR}")

if(USE_FREERTOS)
    target_link_libraries(main PRIVATE freertos_config FreeRTOS)
endif()

add_subdirectory(${PROJECT_SOURCE_DIR}/ElenixOS)

target_compile_definitions(main PRIVATE LV_CONF_INCLUDE_SIMPLE)

if(MSVC)
    target_link_options(main PRIVATE "/WHOLEARCHIVE:$<TARGET_FILE:elenixos_lib>")
elseif(APPLE)
    target_link_options(main PRIVATE "-Wl,-force_load,$<TARGET_FILE:elenixos_lib>")
elseif(MINGW)
    target_link_options(main PRIVATE
        "-Wl,--start-group"
        "$<TARGET_FILE:elenixos_lib>"
    )
else()
    target_link_options(main PRIVATE
        "-Wl,--whole-archive"
        "$<TARGET_FILE:elenixos_lib>"
        "-Wl,--no-whole-archive"
    )
endif()

if(EOS_IS_WASM)
    set_target_properties(main PROPERTIES SUFFIX ".html")

    # Only add actual input files as dependencies, not globs
    # Emscripten automatically handles the preload-file dependencies
    set_property(TARGET main APPEND PROPERTY LINK_DEPENDS
        "${PROJECT_SOURCE_DIR}/main/wasm_shell.html"
        "${PROJECT_SOURCE_DIR}/main/static/WatchCard.png"
        "${PROJECT_SOURCE_DIR}/main/static/Crown.png"
        "${PROJECT_SOURCE_DIR}/main/static/SideButton.png"
    )

    # Create a custom target to monitor fs directory changes without forcing rebuild
    # If fs/.sys or main/static are modified, users should manually rebuild
    add_custom_command(TARGET main PRE_LINK
        COMMAND ${CMAKE_COMMAND} -E make_directory "${EOS_RUNTIME_OUTPUT_DIR}"
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${EOS_RUNTIME_OUTPUT_DIR}/static"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/main/static" "${EOS_RUNTIME_OUTPUT_DIR}/static"
        COMMENT "Ensure WASM runtime output directory exists"
        VERBATIM
    )

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_link_options(main PRIVATE
            "-sUSE_SDL=2"
            "-sALLOW_MEMORY_GROWTH=1"
            "-sSTACK_SIZE=1048576"
            "-sASSERTIONS=1"
            "-sSAFE_HEAP=0"
            "-sEXIT_RUNTIME=0"
            "-sEXPORTED_FUNCTIONS=['_main','_eos_wasm_crown_click','_eos_wasm_side_click','_eos_wasm_launch_app_by_id','_eos_wasm_reload_current_script','_eos_wasm_back_to_watchface','_eos_wasm_write_app_main_js','_eos_wasm_read_app_main_js']"
            "-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap']"
            "-sNO_DISABLE_EXCEPTION_CATCHING"
            "--shell-file=${PROJECT_SOURCE_DIR}/main/wasm_shell.html"
            "--preload-file=${PROJECT_SOURCE_DIR}/fs/.sys@/.sys"
            "--preload-file=${PROJECT_SOURCE_DIR}/main/static@/.simulator/img"
            "--source-map-base=${EOS_WASM_SOURCE_MAP_BASE}"
        )
    else()
        target_link_options(main PRIVATE
            "-O3"
            "-sUSE_SDL=2"
            "-sALLOW_MEMORY_GROWTH=1"
            "-sSTACK_SIZE=1048576"
            "-sASSERTIONS=0"
            "-sSAFE_HEAP=0"
            "-sEXIT_RUNTIME=0"
            "-sNO_DISABLE_EXCEPTION_CATCHING"
            "-sEXPORTED_FUNCTIONS=['_main','_eos_wasm_crown_click','_eos_wasm_side_click','_eos_wasm_launch_app_by_id','_eos_wasm_reload_current_script','_eos_wasm_back_to_watchface','_eos_wasm_write_app_main_js','_eos_wasm_read_app_main_js']"
            "-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap']"
            "--shell-file=${PROJECT_SOURCE_DIR}/main/wasm_shell.html"
            "--preload-file=${PROJECT_SOURCE_DIR}/fs/.sys@/.sys"
            "--preload-file=${PROJECT_SOURCE_DIR}/main/static@/.simulator/img"
        )
    endif()

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_options(main PRIVATE -gsource-map)
        target_link_options(main PRIVATE -gsource-map)
    endif()
else()
    target_link_libraries(main PRIVATE
        elenixos_lib
        lvgl lvgl::thorvg
        ${SDL2_LIBRARIES}
        m
        pthread
    )
endif()

if(MINGW AND NOT EOS_IS_WASM)
    target_link_libraries(main PRIVATE "-Wl,--end-group")
endif()

if(EOS_IS_WASM)
    target_link_libraries(main PRIVATE
        elenixos_lib
        lvgl lvgl::thorvg
    )
endif()

if(WIN32 AND MINGW)
    get_filename_component(MINGW_BIN_DIR ${CMAKE_C_COMPILER} DIRECTORY)
    set(WIN_RUNTIME_DLLS
        libgcc_s_seh-1.dll
        libstdc++-6.dll
        libwinpthread-1.dll
        SDL2.dll
    )

    foreach(RUNTIME_DLL ${WIN_RUNTIME_DLLS})
        add_custom_command(TARGET main POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${MINGW_BIN_DIR}/${RUNTIME_DLL}"
            "$<TARGET_FILE_DIR:main>"
        )
    endforeach()
endif()

add_custom_target(run COMMAND ${EOS_RUNTIME_OUTPUT_DIR}/main DEPENDS main)

if(LV_USE_DRAW_SDL AND NOT EOS_IS_WASM)
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
    find_package(SDL2_image REQUIRED)
    target_include_directories(lvgl PUBLIC ${SDL2_IMAGE_INCLUDE_DIRS})
    target_link_libraries(main PRIVATE ${SDL2_IMAGE_LIBRARIES})
endif()

if(LV_USE_LIBPNG)
    find_package(PNG REQUIRED)
    target_include_directories(lvgl PUBLIC ${PNG_INCLUDE_DIRS})
    target_link_libraries(main PRIVATE ${PNG_LIBRARIES})
endif()

if(LV_USE_LIBJPEG_TURBO)
    find_package(JPEG REQUIRED)
    target_include_directories(lvgl PUBLIC ${JPEG_INCLUDE_DIRS})
    target_link_libraries(main PRIVATE ${JPEG_LIBRARIES})
endif()

if(LV_USE_FFMPEG)
    target_link_libraries(main PRIVATE avformat avcodec avutil swscale z)
endif()

if(LV_USE_FREETYPE)
    find_package(Freetype REQUIRED)
    target_include_directories(lvgl PUBLIC ${FREETYPE_INCLUDE_DIRS})
    target_link_libraries(main PRIVATE ${FREETYPE_LIBRARIES})
endif()

if(APPLE AND NOT EOS_IS_WASM)
    message(STATUS "Building on macOS: linking CoreAudio and AudioToolbox frameworks")
    target_link_libraries(main PRIVATE
        "-framework CoreAudio"
        "-framework AudioToolbox"
        "-framework IOKit"
        "-framework CoreFoundation"
    )
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Debug mode enabled")

    target_compile_options(lvgl PRIVATE
        -pedantic-errors
        -Wall
        $<$<C_COMPILER_ID:GNU>:-Wclobbered>
        -Wdeprecated
        -Wdouble-promotion
        -Wempty-body
        -Wextra
        -Wformat-security
        $<$<C_COMPILER_ID:GNU>:-Wmaybe-uninitialized>
        -Wpointer-arith
        -Wmultichar
        -Wno-pedantic
        -Wreturn-type
        -Wshadow
        -Wshift-negative-value
        -Wsizeof-pointer-memaccess
        -Wtype-limits
        -Wundef
        -Wuninitialized
        -Wunreachable-code
        -Wfloat-conversion
        -Wstrict-aliasing
    )

    if(ASAN)
        message(STATUS "AddressSanitizer enabled")

        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=address")
    else()
        message(STATUS "AddressSanitizer disabled")
    endif()
endif()

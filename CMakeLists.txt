cmake_minimum_required(VERSION 3.27)
project(doomgeneric VERSION 1.4.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

FILE(GLOB_RECURSE CONVERTED_SOURCES
    doomgeneric/m_misc.c
    doomgeneric/m_argv.c
    doomgeneric/m_config.c
    doomgeneric/dstrings.c
    doomgeneric/dummy.c
    doomgeneric/f_finale.c
    doomgeneric/f_wipe.c
    doomgeneric/g_game.c
    doomgeneric/hu_lib.c
    doomgeneric/hu_stuff.c
    doomgeneric/i_cdmus.c
    doomgeneric/icon.c
    doomgeneric/i_endoom.c
    doomgeneric/i_input.c
    doomgeneric/info.c
    doomgeneric/i_sound.c
    doomgeneric/i_system.c
    doomgeneric/i_timer.c
    doomgeneric/i_video.c
    doomgeneric/m_bbox.c
    doomgeneric/m_cheat.c
    doomgeneric/m_controls.c
    doomgeneric/memio.c
    doomgeneric/m_fixed.c
    doomgeneric/m_menu.c
    doomgeneric/m_random.c
    doomgeneric/mus2mid.c
    doomgeneric/p_ceilng.c
    doomgeneric/p_doors.c
    doomgeneric/p_enemy.c
    doomgeneric/p_floor.c
    doomgeneric/p_inter.c
    doomgeneric/p_lights.c
    doomgeneric/p_map.c
    doomgeneric/p_maputl.c
    doomgeneric/p_mobj.c
    doomgeneric/p_plats.c
    doomgeneric/p_pspr.c
    doomgeneric/p_saveg.c
    doomgeneric/p_setup.c
    doomgeneric/p_sight.c
    doomgeneric/p_spec.c
    doomgeneric/p_switch.c
    doomgeneric/p_telept.c
    doomgeneric/p_tick.c
    doomgeneric/p_user.c
    doomgeneric/r_bsp.c
    doomgeneric/r_data.c
    doomgeneric/r_draw.c
    doomgeneric/r_main.c
    doomgeneric/r_plane.c
    doomgeneric/r_segs.c
    doomgeneric/r_sky.c
    doomgeneric/r_things.c
    doomgeneric/sha1.c
    doomgeneric/sounds.c
    doomgeneric/s_sound.c
    doomgeneric/statdump.c
    doomgeneric/st_lib.c
    doomgeneric/st_stuff.c
    doomgeneric/tables.c
    doomgeneric/v_video.c
    doomgeneric/w_checksum.c
    doomgeneric/w_file.c
    doomgeneric/w_file_stdc.c
    doomgeneric/wi_stuff.c
    doomgeneric/w_wad.c
    doomgeneric/am_map.c
    doomgeneric/d_event.c
    doomgeneric/d_items.c
    doomgeneric/d_iwad.c
    doomgeneric/d_loop.c
    doomgeneric/d_main.c
    doomgeneric/d_mode.c
    doomgeneric/d_net.c
    doomgeneric/doomdef.c
    doomgeneric/doomgeneric.c
    doomgeneric/doomstat.c
    doomgeneric/z_zone.c
)

option(UEFIDOOM OFF)

if(NOT UEFIDOOM)
    add_compile_options(-g)
    add_library(dlibc STATIC dlibc/printf.c dlibc/scanf.c dlibc/utils.c)
    target_include_directories(dlibc PUBLIC dlibc)
    add_library(doomgeneric STATIC ${SOURCES} ${CONVERTED_SOURCES})
    target_link_libraries(doomgeneric PUBLIC dlibc)
    target_compile_options(doomgeneric PRIVATE -Wimplicit-function-declaration)
    add_executable(doom_sdl doomgeneric/doomgeneric_sdl.c doomgeneric/hostlibc/host.c)
    target_link_libraries(doom_sdl PRIVATE SDL2 doomgeneric)

    add_executable(doom_embedded doomgeneric/doomgeneric_embedded_sdl.c doomgeneric/hostlibc/embedded.c)
    target_link_libraries(doom_embedded PRIVATE SDL2 doomgeneric)

    set(GOOGLETEST_VERSION 1.14.0)
    enable_testing()
    add_subdirectory(thirdparty/googletest)

    add_executable(doomgeneric_unittests tests/printf_tests.cpp tests/scanf_tests.cpp tests/aspect_ratio.cpp doomgeneric/hostlibc/host.c)
    target_link_libraries(doomgeneric_unittests PRIVATE gtest gtest_main doomgeneric dlibc)
    target_include_directories(doomgeneric_unittests PRIVATE doomgeneric)
else()
    set(CMAKE_C_COMPILER clang)
    add_compile_options(-ffreestanding -MMD -mno-red-zone -std=c11 -target x86_64-unknown-windows -Wno-microsoft-static-assert -Wno-unused-command-line-argument)

    add_library(dlibc STATIC dlibc/printf.c dlibc/scanf.c dlibc/utils.c)
    target_include_directories(dlibc PUBLIC dlibc)

    add_library(doomgeneric_freestanding STATIC ${CONVERTED_SOURCES} doomgeneric/hostlibc/embedded.c)
    target_link_libraries(doomgeneric_freestanding PUBLIC dlibc)
    target_include_directories(doomgeneric_freestanding PUBLIC doomgeneric)
    
    add_library(efiutils STATIC efidoom/efi_utils.c efidoom/x86.c)
    target_link_libraries(efiutils PUBLIC dlibc)
    target_include_directories(efiutils PUBLIC efidoom/efi/inc)

    set(CMAKE_C_LINK_EXECUTABLE "lld <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -out:<TARGET>.efi <LINK_LIBRARIES>")

function(add_efi_executable executable_name)
    add_executable(efi_${executable_name} efidoom/${executable_name}.c)
    target_link_libraries(efi_${executable_name} efiutils doomgeneric_freestanding)
    target_link_options(efi_${executable_name} PRIVATE -flavor link -subsystem:efi_application -entry:efi_main)
endfunction()

    add_efi_executable(doom)
    add_efi_executable(keyboard)
    add_efi_executable(test)
    add_efi_executable(tick)
    add_efi_executable(mouse)
    add_efi_executable(pit_test)
endif()

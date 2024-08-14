#
# 86Box    A hypervisor and IBM PC system emulator that specializes in
#          running old operating systems and software designed for IBM
#          PC systems and compatibles from 1981 through fairly recent
#          system designs based on the PCI bus.
#
#          This file is part of the 86Box distribution.
#
#          CMake build script.
#

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(lz4 STATIC
    lz4/lz4.c
    lz4/lz4file.c
    lz4/lz4frame.c
    lz4/lz4hc.c
    lz4/xxhash.c
)

add_library(switchres STATIC
    switchres/monitor.cpp 
    switchres/modeline.cpp 
    switchres/switchres.cpp 
    switchres/display.cpp 
    switchres/custom_video.cpp
    switchres/log.cpp 
    switchres/switchres_wrapper.cpp
    switchres/edid.cpp
)

if(WIN32)
  target_sources(switchres PRIVATE
    switchres/display_windows.cpp 
    switchres/custom_video_ati_family.cpp 
    switchres/custom_video_ati.cpp 
    switchres/custom_video_adl.cpp 
    switchres/custom_video_pstrip.cpp 
    switchres/resync_windows.cpp
)
endif()

if(UNIX)
  target_sources(switchres PRIVATE
    switchres/display_linux.cpp 
    switchres/custom_video_xrandr.cpp     
)
endif()

add_library(mister STATIC
    mister/groovymister.cpp
    mister/groovymister_wrapper.cpp
    mister/gfx_mister.c
)

target_link_libraries(mister lz4)
target_link_libraries(86Box switchres)
target_link_libraries(86Box mister)







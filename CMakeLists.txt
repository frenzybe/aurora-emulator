cmake_minimum_required(VERSION 3.10)

if(WIN32)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

project(AuroraPlayer)

set(CMAKE_CXX_STANDARD 17)

find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)
find_package(SDL2_image REQUIRED)

if(WIN32)
    add_definitions(-DSDL_MAIN_HANDLED)
    link_directories("C:/vcpkg/installed/x64-windows-static/lib")
endif()

include_directories(${SDL2_INCLUDE_DIRS} ${SDL2_TTF_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIRS} /opt/homebrew/include /usr/local/include)
include_directories(/opt/homebrew/include/SDL2 /usr/local/include/SDL2)
include_directories(third_party/httplib)
include_directories(third_party/rcheevos/include)
include_directories(third_party/genplusgx/libretro/libretro-common/include)

add_definitions(-DRC_CLIENT_SUPPORTS_HASH=1)

link_directories(/opt/homebrew/lib /usr/local/lib)

# Include rcheevos source files directly since it lacks CMakeLists.txt
file(GLOB_RECURSE RCHEEVOS_SRC "third_party/rcheevos/src/*.c")

set(SOURCES
    src/main.cpp
    src/emulator_core.cpp
    src/ra_integration.cpp
    ${RCHEEVOS_SRC}
)

add_executable(MegaDriveEmu ${SOURCES})

find_package(OpenSSL REQUIRED)

# Helper to link SDL2 components robustly
macro(link_sdl2_component target component)
    if(TARGET SDL2_${component}::SDL2_${component})
        target_link_libraries(${target} PRIVATE SDL2_${component}::SDL2_${component})
    elseif(TARGET SDL2::SDL2_${component})
        target_link_libraries(${target} PRIVATE SDL2::SDL2_${component})
    else()
        target_link_libraries(${target} PRIVATE SDL2_${component})
    endif()
endmacro()

if(WIN32)
    target_link_libraries(MegaDriveEmu 
        PRIVATE
        SDL2-static.lib
        SDL2_ttf.lib
        SDL2_image.lib
        OpenSSL::SSL 
        OpenSSL::Crypto
        imm32 setupapi version winmm shell32 user32 gdi32 advapi32 ole32 oleaut32 uuid
    )
else()
    target_link_libraries(MegaDriveEmu 
        PRIVATE
        ${SDL2_LIBRARIES} 
        dl 
        OpenSSL::SSL 
        OpenSSL::Crypto 
        "-framework CoreFoundation" 
        "-framework Security"
    )
    link_sdl2_component(MegaDriveEmu ttf)
    link_sdl2_component(MegaDriveEmu image)
endif()

add_subdirectory(tests)

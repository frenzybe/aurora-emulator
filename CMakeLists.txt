cmake_minimum_required(VERSION 3.10)

if(WIN32)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

project(AuroraPlayer)

set(CMAKE_CXX_STANDARD 17)

find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(OpenSSL REQUIRED)

if(WIN32)
    add_definitions(-DSDL_MAIN_HANDLED)
    # Search for static libs in vcpkg
    find_library(SDL2_STATIC_LIB NAMES SDL2-static SDL2 SDL2d PATHS "C:/vcpkg/installed/x64-windows-static/lib")
    find_library(SDL2_TTF_STATIC_LIB NAMES SDL2_ttf SDL2_ttfd PATHS "C:/vcpkg/installed/x64-windows-static/lib")
    find_library(SDL2_IMAGE_STATIC_LIB NAMES SDL2_image SDL2_imaged PATHS "C:/vcpkg/installed/x64-windows-static/lib")
    
    # Force absolute paths if finding failed (for GitHub)
    if(NOT SDL2_STATIC_LIB)
        set(SDL2_STATIC_LIB "C:/vcpkg/installed/x64-windows-static/lib/SDL2-static.lib")
    endif()
    if(NOT SDL2_TTF_STATIC_LIB)
        set(SDL2_TTF_STATIC_LIB "C:/vcpkg/installed/x64-windows-static/lib/SDL2_ttf.lib")
    endif()
    if(NOT SDL2_IMAGE_STATIC_LIB)
        set(SDL2_IMAGE_STATIC_LIB "C:/vcpkg/installed/x64-windows-static/lib/SDL2_image.lib")
    endif()
else()
    # Search for shared libs on Mac (Homebrew)
    find_library(SDL2_LIB NAMES SDL2 PATHS "/opt/homebrew/lib" "/usr/local/lib")
    find_library(SDL2_TTF_LIB NAMES SDL2_ttf PATHS "/opt/homebrew/lib" "/usr/local/lib")
    find_library(SDL2_IMAGE_LIB NAMES SDL2_image PATHS "/opt/homebrew/lib" "/usr/local/lib")
endif()

include_directories(
    ${SDL2_INCLUDE_DIRS} ${SDL2_TTF_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIRS}
    third_party/httplib
    third_party/rcheevos/include
    third_party/genplusgx/libretro/libretro-common/include
    /opt/homebrew/include /usr/local/include
    /opt/homebrew/include/SDL2 /usr/local/include/SDL2
)

add_definitions(-DRC_CLIENT_SUPPORTS_HASH=1)
link_directories(/opt/homebrew/lib /usr/local/lib)

# Include rcheevos source files directly
file(GLOB_RECURSE RCHEEVOS_SRC "third_party/rcheevos/src/*.c")

add_executable(MegaDriveEmu 
    src/main.cpp 
    src/emulator_core.cpp 
    src/ra_integration.cpp 
    ${RCHEEVOS_SRC}
)

if(WIN32)
    target_link_libraries(MegaDriveEmu 
        PRIVATE
        ${SDL2_STATIC_LIB}
        ${SDL2_TTF_STATIC_LIB}
        ${SDL2_IMAGE_STATIC_LIB}
        OpenSSL::SSL OpenSSL::Crypto
        imm32 setupapi version winmm shell32 user32 gdi32 advapi32 ole32 oleaut32 uuid
    )
else()
    target_link_libraries(MegaDriveEmu 
        PRIVATE
        ${SDL2_LIB}
        ${SDL2_TTF_LIB}
        ${SDL2_IMAGE_LIB}
        OpenSSL::SSL OpenSSL::Crypto
        dl "-framework CoreFoundation" "-framework Security"
    )
endif()

# add_subdirectory(tests)

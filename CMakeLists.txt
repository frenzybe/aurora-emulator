cmake_minimum_required(VERSION 3.10)
project(AuroraPlayer)

set(CMAKE_CXX_STANDARD 17)

find_package(SDL2 REQUIRED)

include_directories(${SDL2_INCLUDE_DIRS} /opt/homebrew/include /usr/local/include)
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

if(WIN32)
    target_link_libraries(MegaDriveEmu 
        ${SDL2_LIBRARIES} 
        SDL2_ttf 
        SDL2_image 
        OpenSSL::SSL 
        OpenSSL::Crypto
    )
else()
    target_link_libraries(MegaDriveEmu 
        ${SDL2_LIBRARIES} 
        dl 
        SDL2_ttf 
        SDL2_image 
        OpenSSL::SSL 
        OpenSSL::Crypto 
        "-framework CoreFoundation" 
        "-framework Security"
    )
endif()

add_subdirectory(tests)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)
find_package(SDL2_image REQUIRED)

add_executable(core_tests core_tests.cpp ../src/emulator_core.cpp)
if(WIN32)
    target_link_libraries(core_tests 
        PRIVATE
        gtest_main 
        SDL2-static.lib
        SDL2_ttf.lib
        SDL2_image.lib
        OpenSSL::SSL 
        OpenSSL::Crypto
        imm32 setupapi version winmm shell32 user32 gdi32 advapi32 ole32 oleaut32 uuid
    )
else()
    target_link_libraries(core_tests 
        PRIVATE
        gtest_main 
        ${SDL2_LIBRARIES} 
        dl 
        OpenSSL::SSL 
        OpenSSL::Crypto 
        "-framework CoreFoundation" 
        "-framework Security"
    )
    link_sdl2_component(core_tests ttf)
    link_sdl2_component(core_tests image)
endif()

include(GoogleTest)
gtest_discover_tests(core_tests)

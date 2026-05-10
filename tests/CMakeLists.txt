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
find_package(OpenSSL REQUIRED)

add_executable(core_tests core_tests.cpp ../src/emulator_core.cpp)

if(WIN32)
    target_link_libraries(core_tests 
        PRIVATE
        gtest_main 
        ${SDL2_STATIC_LIB}
        ${SDL2_TTF_STATIC_LIB}
        ${SDL2_IMAGE_STATIC_LIB}
        OpenSSL::SSL OpenSSL::Crypto
        imm32 setupapi version winmm shell32 user32 gdi32 advapi32 ole32 oleaut32 uuid
    )
else()
    target_link_libraries(core_tests 
        PRIVATE
        gtest_main 
        ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES} ${SDL2_IMAGE_LIBRARIES}
        OpenSSL::SSL OpenSSL::Crypto 
        dl "-framework CoreFoundation" "-framework Security"
    )
endif()

include(GoogleTest)
gtest_discover_tests(core_tests)

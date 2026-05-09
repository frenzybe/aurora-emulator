include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(core_tests core_tests.cpp ../src/emulator_core.cpp)
if(WIN32)
    target_link_libraries(core_tests 
        gtest_main 
        ${SDL2_LIBRARIES} 
        SDL2_ttf 
        SDL2_image 
        OpenSSL::SSL 
        OpenSSL::Crypto
    )
else()
    target_link_libraries(core_tests 
        gtest_main 
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

include(GoogleTest)
gtest_discover_tests(core_tests)

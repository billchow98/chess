include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

function(add_tuna_test name)
    add_executable(${name} ${ARGN})
    target_include_directories(${name} PRIVATE "${CMAKE_SOURCE_DIR}/src")
    target_compile_definitions(${name} PRIVATE "${TUNA_DEFINES}")
    target_compile_options(${name} PRIVATE -flto -march=x86-64-v3)
    target_link_options(${name} PRIVATE -flto -static)
    find_package(Threads REQUIRED)
    target_link_libraries(${name} PRIVATE tuna_lib Threads::Threads GTest::gtest_main)
    gtest_discover_tests(${name})
endfunction()

enable_testing()
include(GoogleTest)

add_tuna_test(movegen_test movegen_test.cpp)

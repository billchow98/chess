cmake_minimum_required(VERSION 3.22)
project(tuna)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

execute_process(
	COMMAND git describe --tags --exact-match
	RESULT_VARIABLE TAG_RESULT
	OUTPUT_VARIABLE GIT_TAG
	OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
	COMMAND git rev-parse --short HEAD
	OUTPUT_VARIABLE GIT_COMMIT
	OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
	COMMAND git rev-parse --abbrev-ref HEAD
	OUTPUT_VARIABLE GIT_BRANCH
	OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
  COMMAND git status --untracked-files=no --porcelain
  OUTPUT_VARIABLE GIT_STATUS
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(TAG_RESULT EQUAL 0)
	set(PROJECT_VERSION "${GIT_TAG}")
elseif(GIT_STATUS STREQUAL "")
	set(PROJECT_VERSION "${GIT_BRANCH}-${GIT_COMMIT}")
else()
  	set(PROJECT_VERSION "${GIT_BRANCH}-${GIT_COMMIT}-dirty")
endif()

add_library(tuna_lib STATIC
	src/bb.cpp
	src/board.cpp
	src/common.cpp
	src/eval.cpp
	src/hash.cpp
	src/io.cpp
	src/logging.cpp
	src/lookup.cpp
	src/movegen.cpp
	src/movepick.cpp
	src/perf.cpp
	src/search.cpp
	src/tt.cpp
	src/uci.cpp
)

set(TUNA_DEFINES TUNA_VERSION="${PROJECT_VERSION}")
target_compile_definitions(tuna_lib PRIVATE "${TUNA_DEFINES}")
target_compile_options(tuna_lib PRIVATE -flto -march=x86-64-v3)
target_link_options(tuna_lib PRIVATE -flto -static)

add_executable(tuna src/main.cpp)
target_compile_definitions(tuna PRIVATE "${TUNA_DEFINES}")
target_compile_options(tuna PRIVATE -flto -march=x86-64-v3)
target_link_options(tuna PRIVATE -flto -static)

find_package(Threads REQUIRED)
target_link_libraries(tuna PRIVATE tuna_lib Threads::Threads)

add_subdirectory(test)

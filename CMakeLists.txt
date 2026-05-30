cmake_minimum_required(VERSION 3.11)
project(FakeJoystick C)

include(FetchContent)

FetchContent_Declare(OptionalAcornC GIT_REPOSITORY https://github.com/chrisbazley/OptionalAcornC.git GIT_TAG main)

FetchContent_MakeAvailable(OptionalAcornC)

set(CMAKE_C_STANDARD 99)

if(MSVC)
    # MSVC equivalent of -Wall -Wextra is /W4
    add_compile_options(/W4 /wd4200 /wd4996 /wd4130)
else()
    # For GCC and Clang
    add_compile_options(-Wall -Wextra -Wsign-compare -pedantic)
endif()

string(TOUPPER "${CMAKE_SYSTEM_NAME}" SYSTEM_NAME_UPPER)

if(SYSTEM_NAME_UPPER STREQUAL "RISCOS")
  # RISC OS: The only platform where it links and runs
  add_executable(FakeJoystick FakeJoystick.c)
else()
  # Other platforms: Compile only (no linking) to catch errors
  add_library(FakeJoystick FakeJoystick.c)
endif()

target_link_libraries(FakeJoystick PRIVATE
    OptionalAcornC
)

target_compile_definitions(FakeJoystick PUBLIC
    $<$<CONFIG:Debug>:DEBUG_OUTPUT>
)

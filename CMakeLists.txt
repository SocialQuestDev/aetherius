cmake_minimum_required(VERSION 3.14)
project(Aetherius)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)
FetchContent_Declare(
  magic_enum
  GIT_REPOSITORY https://github.com/Neargye/magic_enum.git
  GIT_TAG v0.9.5
)
FetchContent_MakeAvailable(magic_enum)

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

find_package(Boost 1.71 REQUIRED CONFIG)

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    include_directories(${CMAKE_SOURCE_DIR}/include)

    file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS
            "src/*.cpp"
            "src/*.c"
            "include/*.h"
    )

    add_executable(Aetherius ${CORE_SOURCES})

    target_link_libraries(Aetherius PRIVATE ${Boost_LIBRARIES})
endif()

find_package(tomlplusplus REQUIRED)
target_link_libraries(Aetherius PRIVATE tomlplusplus::tomlplusplus)

find_package(nlohmann_json REQUIRED)
target_link_libraries(Aetherius PRIVATE nlohmann_json::nlohmann_json)

find_package(OpenSSL REQUIRED)
target_link_libraries(Aetherius PRIVATE OpenSSL::SSL)
target_link_libraries(Aetherius PRIVATE OpenSSL::Crypto)

find_package(ZLIB REQUIRED)
target_link_libraries(Aetherius PRIVATE ZLIB::ZLIB)

target_link_libraries(Aetherius PRIVATE magic_enum::magic_enum)

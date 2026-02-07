cmake_minimum_required(VERSION 4.1)
project(Aetherius)

set(CMAKE_CXX_STANDARD 20)

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

find_package(Boost 1.80 REQUIRED)

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})

    file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS
            "src/*.cpp"
            "src/*.c"
            "include/*.h"
    )

    add_executable(Aetherius ${CORE_SOURCES}
            src/utility/ConfigValidator.cpp
            include/utility/ConfigValidator.h)

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
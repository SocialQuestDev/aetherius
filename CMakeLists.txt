cmake_minimum_required(VERSION 4.1)
project(Aetherius)

set(CMAKE_CXX_STANDARD 20)

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

find_package(Boost 1.80 REQUIRED)

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})

    add_executable(Aetherius src/main.cpp
            include/Logger.h
            src/Logger.cpp
            src/Connection.cpp
            include/Connection.h
            src/Server.cpp
            include/Server.h
            include/Server.h
            include/PacketBuffer.h
            include/NbtBuilder.h
            include/World.h
            include/Chunk.h)

    target_link_libraries(Aetherius PRIVATE ${Boost_LIBRARIES})
endif()

find_package(tomlplusplus REQUIRED)
target_link_libraries(Aetherius PRIVATE tomlplusplus::tomlplusplus)
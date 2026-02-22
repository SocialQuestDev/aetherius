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

FetchContent_Declare(
  sol2
  GIT_REPOSITORY https://github.com/ThePhD/sol2.git
  GIT_TAG v3.5.0
)
FetchContent_MakeAvailable(sol2)

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

find_package(Boost 1.71 REQUIRED CONFIG)
add_subdirectory(external/lua)

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

if (TARGET sol2::sol2)
    target_link_libraries(Aetherius PRIVATE sol2::sol2)
elseif(TARGET sol2)
    target_link_libraries(Aetherius PRIVATE sol2)
endif()

set(DOTNET_ROOT "/usr/share/dotnet")

include_directories(
        ${DOTNET_ROOT}/packs/Microsoft.NETCore.App.Host.linux-x64/10.0.3/runtimes/linux-x64/native
)

link_directories(
        ${DOTNET_ROOT}/shared/Microsoft.NETCore.App/10.0.3
)

target_link_libraries(Aetherius PRIVATE
        dl
        pthread
        hostfxr
        /usr/share/dotnet/packs/Microsoft.NETCore.App.Host.linux-x64/10.0.3/runtimes/linux-x64/native/libnethost.so
)

target_link_libraries(Aetherius PRIVATE lua::lib)

file(GLOB MANAGED_FILES "${CMAKE_SOURCE_DIR}/managed/*")

add_custom_target(UpdateManaged
        COMMAND ${CMAKE_COMMAND} -E echo "Copying managed files to build directory..."
        COMMAND ${CMAKE_COMMAND} -E copy ${MANAGED_FILES} "${CMAKE_CURRENT_BINARY_DIR}/"
        COMMENT "Updating managed assemblies in build folder"
        VERBATIM
)

add_custom_target(UpdatePlugins
        COMMAND ${CMAKE_COMMAND} -E rm -rf "${CMAKE_CURRENT_BINARY_DIR}/plugins_lua/"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/plugins_lua/"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/plugins_lua/"
        "${CMAKE_CURRENT_BINARY_DIR}/plugins_lua/"

        COMMAND ${CMAKE_COMMAND} -E rm -rf "${CMAKE_CURRENT_BINARY_DIR}/plugins_sharp/"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/plugins_sharp/"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/plugins_sharp/"
        "${CMAKE_CURRENT_BINARY_DIR}/plugins_sharp/"

        COMMENT "Forcing plugin sync: deleting old and copying fresh files..."
        VERBATIM
)

add_dependencies(Aetherius UpdatePlugins)
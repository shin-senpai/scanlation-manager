cmake_minimum_required (VERSION 3.28)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(scanlation-manager LANGUAGES C CXX)
file(GLOB_RECURSE coresrc CONFIGURE_DEPENDS "src/*.cpp")
add_executable(${PROJECT_NAME} ${coresrc})

set_property(TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

find_package(Threads REQUIRED)
find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)

include(FetchContent)

set(DPP_USE_EXTERNAL_JSON ON)
FetchContent_Declare(
    dpp
    URL https://github.com/brainboxdotcc/DPP/archive/refs/tags/v10.1.4.tar.gz
    URL_HASH SHA256=f11b6d3fc5cc8febcf672b573ca74293ead6c6ea48a66ac0316ab9a5cbd09441
)
FetchContent_MakeAvailable(dpp)
add_library(dpp::dpp ALIAS dpp)

FetchContent_Declare(
    libpqxx
    GIT_REPOSITORY https://github.com/jtv/libpqxx.git
    GIT_TAG        fd7758fcdbf1bc7278d85d525de061acfb412d94
    GIT_SHALLOW    TRUE
)
set(SKIP_BUILD_TEST ON CACHE BOOL "" FORCE)
set(BUILD_DOC OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(libpqxx)

target_include_directories(${PROJECT_NAME}
    PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Threads::Threads
        CURL::libcurl
        nlohmann_json::nlohmann_json
        dpp::dpp
        libpqxx::pqxx
)

set(CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/config.json")
set(CONFIG_DEST "${CMAKE_CURRENT_BINARY_DIR}/config.json")

if(EXISTS ${CONFIG_FILE})
    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CONFIG_FILE}" "${CONFIG_DEST}"
        COMMENT "Copying config.json to build directory..."
    )
else()
    message(STATUS "config.json not found in source directory, skipping copy.")
endif()
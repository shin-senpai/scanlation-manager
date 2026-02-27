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
find_package(DPP)

target_include_directories(${PROJECT_NAME}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Threads::Threads
        OpenSSL::SSL
        OpenSSL::Crypto
        CURL::libcurl
        nlohmann_json::nlohmann_json
)

if (DPP_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE dpp)
else()
    find_package(Git REQUIRED)

    message(WARNING "Could not find DPP install. Building from source instead.")

    if (EXISTS "${CMAKE_SOURCE_DIR}/.git")
        execute_process(
            COMMAND ${GIT_EXECUTABLE} submodule update --init
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            RESULT_VARIABLE RESULT
        )

        if (NOT RESULT EQUAL 0)
            message(FATAL_ERROR "Failed to update the D++ submodule.")
        endif()

        add_subdirectory(DPP)
        target_link_libraries(${PROJECT_NAME} PRIVATE dpp)
    else()
        message(FATAL_ERROR "Failed to find the git executable. Can't update the D++ submodule.")
    endif()
endif()

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
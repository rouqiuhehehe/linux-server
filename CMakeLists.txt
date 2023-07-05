cmake_minimum_required(VERSION 3.5)
project(linux-server)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#message(${CMAKE_VERSION})
add_definitions(-Wno-unknown-pragmas)

function (executable)
    file(GLOB SOURCE_SRC "*.c" "*.cpp")

    foreach (FILE ${SOURCE_SRC})
        string(REGEX REPLACE ".*/(.*)\\.(.*)" "\\1" TARGET_NAME ${FILE})
        string(REGEX REPLACE ".*/(.*)" "\\1" FILE_NAME ${FILE})

        add_executable(${TARGET_NAME} ${FILE_NAME})
    endforeach ()
endfunction ()

include_directories(lib/include)

add_subdirectory(lib)
add_subdirectory(src)
#add_custom_target(ASSETS_COPY ALL VERBATIM COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/assets" "${CMAKE_BINARY_DIR}/assets")

# 拷贝文件夹到指定的目录
add_custom_target(copy_files ALL
        VERBATIM
        # COMMAND_EXPAND_LISTS
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/assets"
        COMMAND ${CMAKE_COMMAND} -E
        copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "${CMAKE_BINARY_DIR}/assets")
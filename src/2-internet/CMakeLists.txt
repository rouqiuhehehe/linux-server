file(GLOB SOURCE_SRC "*.c" "*.cpp")

message("${SOURCE_SRC} ------- ")
foreach (FILE ${SOURCE_SRC})
    string(REGEX REPLACE ".*/(.*).([^.]+)" "\\1" TARGET_NAME ${FILE})
    string(REGEX REPLACE ".*/(.*)" "\\1" FILE_NAME ${FILE})

    if (${TARGET_NAME} STREQUAL "http-reactor-epoll")
        add_executable(${TARGET_NAME} ${FILE_NAME} "websocket/websocket.c")
        target_link_libraries(${TARGET_NAME} crypto)
    else ()
        add_executable(${TARGET_NAME} ${FILE_NAME})
    endif ()
endforeach ()
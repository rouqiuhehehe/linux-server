file(GLOB SOURCE_SRC "*.c" "*.cpp")

message("${SOURCE_SRC} ------- ")
foreach (FILE ${SOURCE_SRC})
    string(REGEX REPLACE ".*/(.*).([^.]+)" "\\1" TARGET_NAME ${FILE})
    string(REGEX REPLACE ".*/(.*)" "\\1" FILE_NAME ${FILE})

    add_executable(${TARGET_NAME} ${FILE_NAME})
    if (${TARGET_NAME} STREQUAL "dns")
        target_link_libraries(${TARGET_NAME} async)
    endif ()

    if (${TARGET_NAME} STREQUAL "icmp-arp-udp-protocol")
        target_include_directories(${TARGET_NAME} PUBLIC /usr/local/include)
#        target_link_libraries(${TARGET_NAME} netmap)
    endif ()
endforeach ()
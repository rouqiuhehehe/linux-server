function(executable)
    file(GLOB SOURCE_SRC "*.c" "*.cpp")

    foreach (FILE ${SOURCE_SRC})
        string(REGEX REPLACE ".*/(.*)\\.(.*)" "\\1" TARGET_NAME ${FILE})
        string(REGEX REPLACE ".*/(.*)" "\\1" FILE_NAME ${FILE})

        add_executable(${TARGET_NAME} ${FILE_NAME})
    endforeach ()
endfunction()

function(auxLibrary targetName link include)
    aux_source_directory(. SOURCE_SRC)
    add_library(${targetName} SHARED ${SOURCE_SRC})
    add_library(${targetName}_static STATIC $<TARGET_OBJECTS:${targetName}>)

    set_target_properties(${targetName}_static PROPERTIES OUTPUT_NAME ${targetName})

    if (link)
        target_link_directories(${targetName} PUBLIC ${link})
        target_link_directories(${targetName}_static PUBLIC ${link})
    endif ()

    if (include)
        target_include_directories(${targetName} PUBLIC ${include})
        target_include_directories(${targetName}_static PUBLIC ${include})
    endif ()
endfunction()
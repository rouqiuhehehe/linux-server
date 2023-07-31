include_directories(../include)

aux_source_directory(. SOURCE_SRC)
add_library(coroutine SHARED ${SOURCE_SRC})

target_link_libraries(coroutine dl pthread)
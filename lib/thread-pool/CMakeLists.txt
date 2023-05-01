aux_source_directory(. SOURCE_SRC)

add_library(threadpool ${SOURCE_SRC})
target_link_libraries(threadpool pthread)
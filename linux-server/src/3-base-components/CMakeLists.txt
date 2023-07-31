executable()

target_link_libraries(memory-leak dl)

target_link_options(back-trance PUBLIC -rdynamic)

target_link_libraries(try-catch-finally pthread)
target_link_options(try-catch-finally PUBLIC -rdynamic)

target_link_libraries(test-multi-thread-i++ PRIVATE pthread)
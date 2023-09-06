project(kv-store)

executable()

target_link_libraries(kv-store pthread dl)
target_link_options(kv-store PRIVATE -rdynamic)
set(MODE_PATH_PREFIX /home/yoshiki/share)

set(NGX_ROOT_PATH /home/yoshiki/share/nginx-1.24.0)
set(PCRE_ROOT_PATH /home/yoshiki/share/pcre-8.45)

set(NGX_INCLUDE_PATH
    ${NGX_ROOT_PATH}/src/core
    ${NGX_ROOT_PATH}/src/os/unix
    ${NGX_ROOT_PATH}/objs)

set(NGX_OBJECTS_PATH
    ${NGX_ROOT_PATH}/objs/src/core/ngx_string.o
    ${NGX_ROOT_PATH}/objs/src/core/ngx_palloc.o
    ${NGX_ROOT_PATH}/objs/src/os/unix/ngx_alloc.o
    ${NGX_ROOT_PATH}/objs/src/core/ngx_palloc.o)

set(NGX_MODULE_INCLUDE_PATH ${PCRE_ROOT_PATH} ${NGX_INCLUDE_PATH})

add_executable(nginx-str nginx-str.c)
target_link_libraries(nginx-str PRIVATE ${NGX_OBJECTS_PATH})
target_include_directories(nginx-str PRIVATE ${NGX_MODULE_INCLUDE_PATH})
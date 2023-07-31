//
// Created by 115282 on 2023/7/20.
//
#include <ngx_core.h>

int main ()
{
    ngx_str_t str = ngx_string("我是你爹");

    printf("str : %s", str.data);
    return 0;
}
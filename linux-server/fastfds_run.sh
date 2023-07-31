#!/bin/bash

tck_res=$(netstat -anop | grep 22122)
if [ -n "$tck_res" ]; then
	echo "trackerd is running..."
else
	. /etc/init.d/fdfs_trackerd start
fi

stor_res=$(netstat -anop | grep 23000)
if [ -n "$stor_res" ]; then
        echo "storaged is running..."
else
        . /etc/init.d/fdfs_storaged start
fi

nginx_res=$(netstat -anop)
if [ -n "$nginx_res" ]; then
        echo "nginx is running..."
else
        . /usr/local/nginx/sbin/nginx -c /usr/local/nginx/conf/nginx.conf
fi

#echo $tck_res
#/etc/init.d/fdfs_trackerd start
#/etc/init.d/fdfs_storaged start

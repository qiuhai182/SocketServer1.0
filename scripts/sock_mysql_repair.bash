#!/bin/bash

# 修复mysql.sock、mysqld.sock链接失败问题

if [ -L "/tmp/mysql.sock" ]; then
    if [ ! -L "/var/run/mysqld/mysqld.sock" ]; then
        if [ ! -d "/var/run/mysqld" ]; then
            sudo mkdir /var/run/mysqld/
        fi
        sudo ln -s /tmp/mysql.sock /var/run/mysqld/mysqld.sock
    fi
elif [ -L "/var/run/mysqld/mysqld.sock" ]; then
    sudo ln -s /var/run/mysqld/mysqld.sock /tmp/mysql.sock
else
    echo "mysql_socket修复失败"
fi

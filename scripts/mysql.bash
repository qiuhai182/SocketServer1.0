#!/bin/bash

# 创建数据库及访问账户

echo -e "\033[32m请在下方输入数据库root账户密码：\033[0m"

mysql -u root -p << EOF
CREATE USER 'iReader'@'localhost' IDENTIFIED  BY '123456@iReader';
create database iReaderDataBase charset utf8;
GRANT ALL on iReaderDataBase.* to 'iReader'@'localhost';
exit
EOF
echo -e "\033[32m 若未报错，则已创建数据库：iReaderDataBase，已创建具备完全访问权限的账号：iReader \033[0m"


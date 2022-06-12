#!/bin/bash

# 如果下面有错误，退出脚本
set -e

#  如果build目录不存在,创建该目录
if [ ! -d `pwd`/build ]; then
    mkdir `pwd`/build
fi

# 将之前产生过的编译文件都删除 
rm -rf `pwd`/build/*

#  进入build下,编译
cd `pwd`/build && 
    cmake .. &&
    make

#  回到项目根目录
cd ..

# 把头文件拷贝到 /usr/include/mymuduo 下，so库拷贝到 /usr/lib 下， 直接在系统的环境变量下，不用再指定库的路径 
# 创建目录
if [ ! -d /usr/include/mymuduo ]; then
    mkdir /usr/include/mymuduo
fi

# 将所有头文件拷贝到 usr/include/mymuduo 下
for header in `ls *.h`
do 
    cp $header /usr/include/mymuduo
done

# 将so库拷贝到 /usr/lib 下 
cp `pwd`/lib/libmymuduo.so /usr/lib

# 刷新一下动态库缓存
ldconfig 



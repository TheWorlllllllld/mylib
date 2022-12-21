cmake_minimum_required(VERSION 2.50)
project(mymuduo)

#生成debug版本
# set(CMAKE_BUILD_TYPE "Debug")

#设置编译出来的动态库的路径,PROJECT_BINARY_DIR表示当前目录
set(LIBRARY_OUTPUT_PATH ../lib)

#设置调试信息 比如将版本设为c++11
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11 -fPIC")

#定义编译的文件夹 . 表示当前目录全部文件，放在SRC_LIS这个变量下
aux_source_directory(. SRC_LIST)

#编译成动态库:add_library     shared表示动态库 muduo表示动态库的名字
add_library(mymuduo SHARED ${SRC_LIST})
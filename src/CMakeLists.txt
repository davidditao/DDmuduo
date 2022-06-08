cmake_minimum_required(VERSION 2.5)
project(mymuduo)

# mymuduo最终编译成so动态库，设置动态库的路径， 放在根目录的lib文件夹下面
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

# 设置调试信息
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11 -fPIC")

# 定义参与编译的源代码文件，自动识别文件
# 在新添加文件的时候，有可能会出现新文件没有被编译的问题
# 这是因为，我们必须要先经过 cmake 命令生成 makefile 文件，然后再使用 make 命令
# 而如果新添加的文件没有加入到 makefile 文件中，那么新文件就不会参与编译
# 解决方法是，在 Build 前先 Clean Reconfigure All Projects
aux_source_directory(. SRC_LIST)

# 编译生成动态库mymuduo
add_library(mymuduo SHARED ${SRC_LIST})


# CMakeList.txt: 2.1裁判系统 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)
cmake_minimum_required(VERSION 3.10)
project(RobotSystem)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


project ("2.1裁判系统")

# 将源代码添加到此项目的可执行文件。
add_executable (CMakeTarget "main.cpp"  )


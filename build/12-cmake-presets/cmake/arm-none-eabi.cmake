# 示例工具链文件：需要安装 arm-none-eabi-gcc 后才可以真正使用。
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_C_COMPILER arm-none-eabi-gcc)

# 交叉编译产物不能在宿主机直接运行。
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

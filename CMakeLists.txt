# simple-chatroom 构建脚本（跨平台）
cmake_minimum_required(VERSION 3.10)
project(simple-chatroom CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

# 源码使用 UTF-8（含中文注释/字符串），MSVC 需要显式指定
if(MSVC)
    add_compile_options(/utf-8)
endif()

add_executable(chat-server server.cpp)
add_executable(chat-client client.cpp)

target_link_libraries(chat-server Threads::Threads)
target_link_libraries(chat-client Threads::Threads)

# Windows 需要链接 Winsock；其余平台（Linux/macOS）自动使用 POSIX socket
if(WIN32)
    target_link_libraries(chat-server ws2_32)
    target_link_libraries(chat-client ws2_32)
endif()

# 静态链接 CRT（仅 MSVC）：生成的 exe 不依赖 VC++ 运行库
# （VCRUNTIME140.dll / MSVCP140.dll 等），可直接拷给其他 Windows 电脑运行（免安装）。
# 注意：这台环境的 MS 定制版 CMake 会忽略 MSVC_RUNTIME_LIBRARY 属性，
# 因此直接用目标编译选项注入 /MT（cl 对多个 /M 参数取最后一个生效）。
if(MSVC)
    foreach(tgt chat-server chat-client)
        target_compile_options(${tgt} PRIVATE
            $<$<CONFIG:Release>:/MT>
            $<$<CONFIG:Debug>:/MTd>)
    endforeach()
endif()
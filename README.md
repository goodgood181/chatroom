# 简易在线聊天室（simple-chatroom）

一个用 **C++17** 编写的简易在线聊天室，**同一个源码同时兼容 Linux 和 Windows**，
通过 `_WIN32` 宏在编译期选择 Winsock（Windows）或 POSIX socket（Linux/macOS）实现，
网络逻辑本身完全跨平台。

> 📖 **零基础使用教程见 [TUTORIAL.md](TUTORIAL.md)**（编译、启动、局域网联机、排错全流程）。
> 🕸️ **网络拓扑结构图见 [TOPOLOGY.md](TOPOLOGY.md)**（含 PNG 图片与 Mermaid 版）。

## 功能

- 服务器监听指定端口，支持任意多个客户端同时在线
- 每个客户端一个独立接收线程（`std::thread`）
- 消息实时广播给聊天室内所有人（发送者也会看到自己的消息）
- 成员加入 / 离开的系统提示，昵称重复自动加后缀去重
- 客户端输入 `/quit` 退出；服务器按 `Ctrl+C` 退出
- 兼容 CRLF / LF 换行协议

## 文件结构

```
chatroom/
├── server.cpp      聊天服务器
├── client.cpp      命令行聊天客户端
├── CMakeLists.txt  跨平台 CMake 构建脚本（推荐）
├── Makefile        Linux 下 g++ 直接构建
└── README.md       本文件
```

## 编译

### 方式一：CMake（Windows / Linux 通用，推荐）

```bash
cmake -S . -B build
cmake --build build
```

Windows 上若用 Visual Studio 生成器，产物在 `build\Release\` 下；
Linux 上产物在 `build\` 下。也可以指定生成器：`cmake -S . -B build -G "MinGW Makefiles"`（配合 MinGW-w64）。

### 方式二：直接命令行

**Linux（g++ / clang++）：**

```bash
g++ -std=c++17 -pthread server.cpp -o chat-server
g++ -std=c++17 -pthread client.cpp -o chat-client
# 或直接：make
```

**Windows（MSVC 开发者命令行，源码为 UTF-8，需加 `/utf-8`）：**

```bat
cl /std:c++17 /EHsc /utf-8 server.cpp ws2_32.lib /Fe:chat-server.exe
cl /std:c++17 /EHsc /utf-8 client.cpp ws2_32.lib /Fe:chat-client.exe
```

**Windows（MinGW-w64）：**

```bash
g++ -std=c++17 -pthread server.cpp -o chat-server.exe -lws2_32
g++ -std=c++17 -pthread client.cpp -o chat-client.exe -lws2_32
```

## 使用

> ⚠️ **Windows【命令提示符 cmd】不认 `./` 前缀**（那是 Linux/bash 写法），直接写程序名即可。

### 最简单：双击 / 一行命令（Windows）

项目根目录已备好两个快捷脚本（编译产物在 `build\Release\` 下，它们会自动调用）：

```bat
start-server.bat                  :: 启动服务器（默认端口 8888；自定义：start-server.bat 9000）
join-chat.bat                     :: 连接本机 127.0.0.1:8888
join-chat.bat 192.168.1.10 8888   :: 连接局域网内的其他机器
```

### 发给朋友（Windows 单文件免安装）

只要发 **一个文件** `chat-client-standalone.exe`（静态链接免安装版，不依赖 VC++ 运行库，
任何 Win10/11 x64 电脑都能直接跑）。对方解压后在该目录打开 cmd 执行：

```bat
chat-client-standalone.exe 192.168.1.10 8888   :: 192.168.1.10 = 你（服务器主机）的 IP
```

> 注意：该 exe 仅适用于 **Windows**；若对方的电脑是 Linux，需要对方用源码编译
> （`make`，见 CMakeLists/Makefile）或另发 Linux 版可执行文件。

### Windows（cmd，直接使用编译产物）

```bat
cd /d E:\deepseek\chatroom\build\Release
chat-server.exe                   :: 服务器，监听 8888
:: （另开窗口）聊天客户端：
chat-client.exe 192.168.1.10 8888
```

### Linux / macOS

```bash
cd ~/chatroom/build
./chat-server                     # 服务器，监听 8888
./chat-client 192.168.1.10 8888   # 客户端
```

客户端启动后输入昵称即可开始聊天，输入 `/quit` 退出。

> 💡 如果提示 `g++ 不是内部或外部命令`，说明没有安装 g++——不影响使用，
> Windows 直接用上文**已编译好的 `build\Release` 下的 exe** 或 CMake 构建即可。

## 自动化冒烟测试（Windows 开发用）

`smoke-test.ps1` 会用 .NET Process 驱动多个 `chat-client` 实例模拟用户输入，
用于验证消息广播（见文件头注释）。日常使用不需要它。

## 实现要点（跨平台细节）

| 平台差异         | Windows                                    | Linux/macOS            |
| ---------------- | ------------------------------------------ | ---------------------- |
| 网络库初始化     | `WSAStartup` / `WSACleanup`                | 无（忽略 `SIGPIPE`）   |
| 控制台编码       | 自动切换为 UTF-8(65001)，退出时恢复        | 默认 UTF-8，无需处理   |
| socket 类型      | `SOCKET`（句柄，`INVALID_SOCKET`）         | `int`（`-1` 表示无效） |
| 关闭连接         | `closesocket`                              | `close`                |
| 错误码           | `WSAGetLastError()`                        | `errno`                |
| 链接库           | `ws2_32`                                   | 无                     |

线程统一使用 C++11 标准库 `std::thread` + `std::mutex`，因此双平台零依赖。

## 许可证

MIT —— 自由使用、修改和分发。
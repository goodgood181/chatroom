# 简易在线聊天室 —— 使用教程

本教程面向没有任何 C++ 环境经验的同学，从零开始带你把聊天室跑起来。
整套教程在 **Windows** 和 **Linux** 上都适用（命令不同处会分开写）。

---

## 目录

1. [准备工作](#1-准备工作)
2. [编译（生成可执行文件）](#2-编译生成可执行文件)
3. [启动服务器](#3-启动服务器)
4. [连接客户端（本机多人聊天）](#4-连接客户端本机多人聊天)
5. [连接客户端（局域网多设备聊天）](#5-连接客户端局域网多设备聊天)
6. [聊天中的操作](#6-聊天中的操作)
7. [常见问题排查](#7-常见问题排查)
8. [安全说明](#8-安全说明)

---

## 1. 准备工作

聊天室只有两个程序：

| 程序 | 角色 | 说明 |
| --- | --- | --- |
| `chat-server` | 服务器 | 所有消息都要经过它转发，**必须先启动** |
| `chat-client` | 客户端 | 一个人开一个，用来聊天 |

如果只是**本机自己测试**：服务器和客户端在同一台电脑上。
如果想**和别的设备聊天**：在一台电脑上跑服务器，局域网内其他电脑连过来。

准备工作只需一步：**把源码编译成可执行文件**（见下一节）。
如果你用的是已经编译好的版本（例如 Windows 的 `build\Release\` 目录里有 `.exe`），可以跳过编译，直接看[第 3 节](#3-启动服务器)。

---

## 2. 编译（生成可执行文件）

### 2.1 Windows 用户

Windows 有三种方式，任选其一（推荐第一种）。

**方式 A：CMake（推荐，最简单）**

安装 [CMake](https://cmake.org/download/) 和 Visual Studio（含 C++ 桌面开发组件）
或 Visual Studio Build Tools 后，打开"开始菜单 → Visual Studio 的 *开发人员命令提示符*"
（或者任何终端，只要 CMake 与环境变量可用），进入项目目录：

```bat
cd /d E:\deepseek\chatroom
cmake -S . -B build
cmake --build build --config Release
```

编译完成后，可执行文件在这里：

```
E:\deepseek\chatroom\build\Release\chat-server.exe
E:\deepseek\chatroom\build\Release\chat-client.exe
```

**方式 B：直接使用 MSVC 编译器（cl）**

在"开发人员命令提示符"里执行：

```bat
cd /d E:\deepseek\chatroom
cl /std:c++17 /EHsc /utf-8 server.cpp ws2_32.lib /Fe:chat-server.exe
cl /std:c++17 /EHsc /utf-8 client.cpp ws2_32.lib /Fe:chat-client.exe
```

**方式 C：MinGW-w64（如果你装了 g++）**

```bash
cd /d E:\deepseek\chatroom
g++ -std=c++17 -pthread server.cpp -o chat-server.exe -lws2_32
g++ -std=c++17 -pthread client.cpp -o chat-client.exe -lws2_32
```

### 2.2 Linux 用户

**方式 A：CMake（推荐）**

```bash
cd ~/chatroom          # 换成你解压/克隆到的目录
cmake -S . -B build
cmake --build build
```

生成的可执行文件：

```
build/chat-server
build/chat-client
```

**方式 B：g++ 直接编译（或 make）**

```bash
cd ~/chatroom
make
# 等价于：
# g++ -std=c++17 -pthread server.cpp -o chat-server
# g++ -std=c++17 -pthread client.cpp -o chat-client
```

> 💡 没有 g++ 的话先装一下：Ubuntu/Debian `sudo apt install g++ make`，CentOS/RHEL `sudo yum install gcc-c++ make`。

---

## 3. 启动服务器

打开一个终端窗口，进入可执行文件所在目录，运行服务器：

**Windows：**

```bat
cd /d E:\deepseek\chatroom\build\Release
chat-server.exe
```

**Linux：**

```bash
cd ~/chatroom/build
./chat-server
```

看到这一行就说明启动成功：

```
[server] 聊天服务器已启动，监听端口 8888（Ctrl+C 退出）
```

- 默认端口是 **8888**，想换端口就在后面加参数：`chat-server 9000`
- 服务器启动后**不要关掉这个窗口**，关掉 = 聊天室下线
- 要停止服务器：在窗口里按 `Ctrl+C`

---

## 4. 连接客户端（本机多人聊天）

再打开 **几个新的终端窗口**（一个窗口代表一个"用户"），每个窗口里运行客户端：

**Windows：**

```bat
cd /d E:\deepseek\chatroom\build\Release
chat-client.exe
```

**Linux：**

```bash
cd ~/chatroom/build
./chat-client
```

启动后按提示操作：

```
[client] 正在连接 127.0.0.1:8888 ...
请输入昵称: 小明        ← 在这里输入你的昵称（可中文）
-----------------------------------------
  已进入聊天室，直接输入文字发送
  输入 /quit 退出
-----------------------------------------
```

打开三个窗口，分别输入昵称"小明""小红""小刚"，然后在任意一个窗口打字回车，
其他窗口会立刻收到：

```
[小红] 大家好，我是小红
[小刚] 欢迎欢迎！
```

> 多人聊天其实就是：**每个聊天的人开一个终端窗口，各跑一个客户端**。
> 服务器窗口里也会实时打印所有消息（绿色提醒：别把它当成输密码的地方 😄）。

---

## 5. 连接客户端（局域网多设备聊天）

想让另一台电脑（手机、平板装个终端 APP 也行）连进来，步骤：

### 5.1 查看服务器电脑的 IP 地址

在**运行服务器的那台电脑**上执行：

- **Windows**：打开命令行输入 `ipconfig`，找"无线局域网适配器"或"以太网适配器"下的
  **IPv4 地址**，形如 `192.168.1.100`
- **Linux**：终端输入 `hostname -I`（或 `ip addr`），得到形如 `192.168.1.100` 的地址

### 5.2 放行防火墙端口（第一次需要）

服务器和客户端必须**在同一局域网**（连着同一个路由器/交换机）。

**Windows 防火墙**（以管理员身份打开命令行执行，把 8888 换成你的端口）：

```bat
netsh advfirewall firewall add rule name="SimpleChat 8888" dir=in action=allow protocol=TCP localport=8888
```

**Linux（ufw 为例）**：

```bash
sudo ufw allow 8888/tcp
```

### 5.3 在另一台设备上连接

另一台电脑同样先编译好客户端（见第 2 节），然后运行：

**Windows：**

```bat
chat-client.exe 192.168.1.100 8888
```

**Linux：**

```bash
./chat-client 192.168.1.100 8888
```

看到 `[client] 正在连接 192.168.1.100:8888 ...` 后，按提示输入昵称即可。
这时所有设备（服务器电脑上的 + 局域网里连进来的）都在同一个聊天室里聊天。

> 命令行格式：`chat-client <服务器IP> [端口]`，IP 后面的端口可省略（默认 8888）。

### 5.4 不在同一个局域网怎么办（异地聊天）

**为什么连不上**：家庭宽带 / 手机热点一般没有公网 IP（运营商做了 NAT），对方无法直接访问
你家的机器。聊天室本身没有"互联网地址"，需要一座桥。按你的情况选一种：

**方案 A（推荐 · 免费 · 不需要服务器）：虚拟组网 Tailscale / ZeroTier**

1. 两台电脑都安装 Tailscale（Windows 直接装 exe；Linux：`curl -fsSL https://tailscale.com/install.sh | sh`）
2. 登录同一个账号，或用它的"分享设备加入链接"把对方拉进你的网络
3. 打开 Tailscale 面板，看到对方分配的虚拟 IP（形如 `100.x.y.z`）
4. 之后完全照旧：任意一台跑 `chat-server`，对方连 `chat-client 100.x.y.z 8888`
   ——相当于把两台电脑拉进同一张"虚拟局域网"，防火墙、公网 IP 全都不用操心

**方案 B（最正统 · 需要一台公网云服务器）：把服务器放到云端**

1. 买一台便宜的 Linux 云服务器（腾讯云 / 阿里云"轻量应用服务器"即可）
2. SSH 登录后装编译环境：`sudo apt install g++ make`
3. 把 `server.cpp` 传上去（scp / Git），编译：`g++ -std=c++17 -pthread server.cpp -o chat-server`
4. 在云控制台"防火墙 / 安全组"放行 **TCP 8888**，然后运行：`./chat-server 8888`
5. 你和朋友都连云服务器的公网 IP：`chat-client <云服务器公网IP> 8888`
   （服务器程序监听 `0.0.0.0`，部署到公网**不需要改任何代码**）

**方案 C（家里有公网 IP / 或愿意用内网穿透）：把端口映射出去**

- 路由器拿到的是公网 IP：在路由器上做"端口映射 / 虚拟服务器"，把 TCP 8888 转发到跑服务器的电脑内网 IP，朋友连你的公网 IP:8888
- 没有公网 IP：用内网穿透工具（frp、cpolar、花生壳、ngrok）把本机 8888 隧道到公网，朋友连隧道地址（免费隧道一般会变动、限速，适合临时用）

> 无论哪种方案，聊天室始终**只有一个服务器**；客户端命令里填的地址，就是服务器实际可被访问到的那个地址。

---

## 6. 聊天中的操作

| 操作 | 方法 |
| --- | --- |
| 发送消息 | 直接打字，按回车 |
| 退出聊天室（只退出自己） | 输入 `/quit` 回车 |
| 停止服务器（所有人下线） | 在服务器窗口按 `Ctrl+C` |
| 昵称重复 | 不用管，服务器会自动加后缀（如 `小明_2`） |
| 支持中文 | 支持，消息和昵称都可以用中文 |

---

## 7. 常见问题排查

| 现象 | 原因 | 解决办法 |
| --- | --- | --- |
| `[client] 连接失败，err=10061`（Windows） | 服务器没启动，或端口不对 | 先启动服务器；确认端口一致（默认 8888） |
| `[client] 连接失败，err=10061`，但服务器启动了 | 服务器窗口可能已关闭/崩溃 | 看服务器窗口有没有打印错误信息；重启服务器 |
| 连接超时 / 连不上（跨设备时） | 不在同一局域网 / 防火墙拦截 | 确认两边在同一网络；按 [5.2](#52-放行防火墙端口第一次需要) 放行防火墙端口；用 `ping 服务器IP` 测试网络。**如果根本不在一个局域网（异地），请看 [5.4](#54-不在同一个局域网怎么办异地聊天)** |
| IP 地址输错 | — | 在服务器电脑上重新执行 `ipconfig` / `hostname -I` 核对 |
| 端口被占用 `bind(8888) failed` | 已有服务器在跑，或残留了多余的 chat-server 进程（Windows 下同一端口可能被两个服务器静默监听） | 在任务管理器结束多余的 `chat-server.exe`，或换个端口启动：`chat-server 9000`，客户端连 `chat-client 127.0.0.1 9000` |
| 重新编译时提示 `无法打开 chat-server.exe` | 服务器正在运行，Windows 锁定了 exe 文件 | 先关掉服务器窗口再编译；日常使用无需重编译，直接跑 `build\Release` 下的 exe 即可 |
| 中文显示乱码 | 终端编码与程序不一致 | **新版程序已在启动时自动把控制台切到 UTF-8**，正常情况下不会再乱码。若在 PowerShell/特定终端里仍乱码，先执行 `chcp 65001`，或使用 Windows Terminal / VS Code 终端；Linux 一般无此问题 |
| 一条消息没显示 | 对方还没连上 | 广播只发给**当前在线**的人；让所有人都先连接再发消息 |

---

## 8. 安全说明

这是一个**学习用途**的简易聊天室：

- 消息**不加密**（明文传输），不要在公网上随意使用
- 没有账号/密码体系，知道地址和端口就能进
- 建议只在**可信的局域网**（家里、教室）里使用

---

*遇到教程没覆盖的问题，把报错信息贴出来，一般一看错误码就能定位。*
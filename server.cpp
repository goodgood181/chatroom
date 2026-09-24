//======================================================================
//  simple-chatroom / server.cpp
//  简易在线聊天室 —— 服务器端（跨平台：Linux / Windows）
//
//  功能：
//    * 监听指定端口，接受多个客户端连接
//    * 每个客户端一个接收线程
//    * 收到消息后广播给所有其他客户端
//    * 打印成员加入 / 离开日志
//
//  编译：
//    Linux   : g++ -std=c++17 -pthread server.cpp -o chat-server
//    Windows : cl /std:c++17 /EHsc server.cpp ws2_32.lib /Fe:chat-server.exe
//    (也可用 CMake，见 CMakeLists.txt)
//======================================================================

#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <vector>
#include <algorithm>
#include <atomic>
#include <cstring>
#include <cstdlib>

//-------------------------- 平台抽象层 --------------------------
#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
    #pragma comment(lib, "ws2_32.lib")

    using Socket   = SOCKET;
    using SockLen  = int;
    constexpr Socket kInvalidSocket = INVALID_SOCKET;
    constexpr int   kSocketError    = SOCKET_ERROR;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <csignal>
    #include <cerrno>

    using Socket   = int;
    using SockLen  = socklen_t;
    constexpr Socket kInvalidSocket = -1;
    constexpr int   kSocketError    = -1;
#endif

namespace platform {

#ifdef _WIN32
static UINT g_origOutCp = 0;   // 退出时恢复控制台原始输出代码页
static UINT g_origInCp  = 0;   // 退出时恢复控制台原始输入代码页
#endif

// 初始化网络库（Winsock），POSIX 下为空操作
bool init() {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cerr << "[server] WSAStartup failed" << std::endl;
        return false;
    }
    // 源码与聊天消息均为 UTF-8；把控制台输入/输出代码页切到 UTF-8(65001)，
    // 避免在中文 Windows（默认 GBK 代码页）上显示乱码
    g_origOutCp = ::GetConsoleOutputCP();
    g_origInCp  = ::GetConsoleCP();
    ::SetConsoleOutputCP(CP_UTF8);
    ::SetConsoleCP(CP_UTF8);
#else
    // 避免向已关闭的连接发送时进程被 SIGPIPE 杀死
    std::signal(SIGPIPE, SIG_IGN);
#endif
    return true;
}

void cleanup() {
#ifdef _WIN32
    if (g_origOutCp != 0) ::SetConsoleOutputCP(g_origOutCp);
    if (g_origInCp  != 0) ::SetConsoleCP(g_origInCp);
    WSACleanup();
#endif
}

void close_socket(Socket s) {
#ifdef _WIN32
    closesocket(s);
#else
    ::close(s);
#endif
}

int last_error() {
#ifdef _WIN32
    return WSAGetLastError();
#else
    return errno;
#endif
}

// 阻塞读取一行（以 '\n' 结尾，不含结尾换行符）。
// 返回 true 表示成功读到一行；false 表示连接关闭或出错。
bool recv_line(Socket s, std::string &line) {
    line.clear();
    char ch;
    for (;;) {
        int n = ::recv(s, &ch, 1, 0);
        if (n == 0) return false;                 // 对端关闭
        if (n == kSocketError) return false;      // 出错
        if (ch == '\n') break;
        if (ch != '\r') line.push_back(ch);       // 兼容 CRLF
        if (line.size() > 8192) return false;     // 防御：超长消息
    }
    return true;
}

// 发送整条消息
bool send_all(Socket s, const std::string &msg) {
    size_t sent = 0;
    while (sent < msg.size()) {
        int n = ::send(s, msg.data() + sent,
                       static_cast<int>(msg.size() - sent), 0);
        if (n == kSocketError || n == 0) return false;
        sent += static_cast<size_t>(n);
    }
    return true;
}

} // namespace platform

//-------------------------- 服务器逻辑 --------------------------

struct Client {
    Socket   sock;
    std::string name;
};

static std::mutex            g_mutex;      // 保护 g_clients
static std::vector<Client>   g_clients;
static std::atomic<bool>     g_running{true};

static void broadcast(const std::string &msg) {
    std::lock_guard<std::mutex> lock(g_mutex);
    for (auto it = g_clients.begin(); it != g_clients.end();) {
        if (!platform::send_all(it->sock, msg)) {
            // 发送失败：移除该客户端
            platform::close_socket(it->sock);
            std::cout << "[server] client '" << it->name
                      << "' disconnected" << std::endl;
            it = g_clients.erase(it);
        } else {
            ++it;
        }
    }
}

static void remove_client(const Socket s) {
    std::lock_guard<std::mutex> lock(g_mutex);
    auto it = std::find_if(g_clients.begin(), g_clients.end(),
                           [s](const Client &c) { return c.sock == s; });
    if (it != g_clients.end()) {
        std::cout << "[server] client '" << it->name
                  << "' disconnected" << std::endl;
        platform::close_socket(it->sock);
        g_clients.erase(it);
    }
}

static void handle_client(Socket s) {
    std::string name;
    if (!platform::recv_line(s, name) || name.empty()) {
        platform::close_socket(s);
        return;
    }
    // 名字去重：重名则在后面加数字
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        std::string base = name;
        int suffix = 2;
        auto taken = [&](const std::string &n) {
            return std::any_of(g_clients.begin(), g_clients.end(),
                               [&](const Client &c) { return c.name == n; });
        };
        while (taken(name)) name = base + "_" + std::to_string(suffix++);
        g_clients.push_back(Client{s, name});
    }

    std::cout << "[server] '" << name << "' joined ("
              << g_clients.size() << " online)" << std::endl;

    broadcast("[system] " + name + " 加入了聊天室\n");
    // 告知本人连接成功
    platform::send_all(s, "[system] 连接成功，当前在线 " +
                          std::to_string(g_clients.size()) + " 人\n");

    std::string line;
    while (g_running.load() && platform::recv_line(s, line)) {
        if (line.empty()) continue;
        std::cout << "[" << name << "] " << line << std::endl;
        broadcast("[" + name + "] " + line + "\n");
    }

    remove_client(s);
    broadcast("[system] " + name + " 离开了聊天室\n");
}

int main(int argc, char *argv[]) {
    if (!platform::init()) return 1;

    int port = 8888;
    if (argc >= 2) {
        port = std::atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            std::cerr << "用法: chat-server [端口]（默认 8888）" << std::endl;
            return 1;
        }
    }

    Socket listen_sock = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock == kInvalidSocket) {
        std::cerr << "[server] socket() failed, err="
                  << platform::last_error() << std::endl;
        platform::cleanup();
        return 1;
    }

#ifdef _WIN32
    // Windows 下【不】设置 SO_REUSEADDR：
    // 该选项在 Windows 上允许第二个服务器静默绑定同一端口（双监听，易造成消息路由混乱）。
    // 不设置后，若端口被占用 bind() 会直接报错退出，提示更明确。
#else
    int reuse = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
#endif

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);   // 监听所有网卡
    addr.sin_port        = htons(static_cast<u_short>(port));

    if (::bind(listen_sock, reinterpret_cast<sockaddr *>(&addr),
               sizeof(addr)) == kSocketError) {
        std::cerr << "[server] bind(" << port << ") failed, err="
                  << platform::last_error() << std::endl;
        platform::close_socket(listen_sock);
        platform::cleanup();
        return 1;
    }

    if (::listen(listen_sock, 16) == kSocketError) {
        std::cerr << "[server] listen() failed, err="
                  << platform::last_error() << std::endl;
        platform::close_socket(listen_sock);
        platform::cleanup();
        return 1;
    }

    std::cout << "[server] 聊天服务器已启动，监听端口 " << port
              << "（Ctrl+C 退出）" << std::endl;

    std::vector<std::thread> threads;
    while (g_running.load()) {
        sockaddr_in peer{};
        SockLen peer_len = sizeof(peer);
        Socket client = ::accept(listen_sock,
                                 reinterpret_cast<sockaddr *>(&peer),
                                 &peer_len);
        if (client == kInvalidSocket) {
            if (!g_running.load()) break;
            std::cerr << "[server] accept() failed, err="
                      << platform::last_error() << std::endl;
            continue;
        }

        char ip[INET_ADDRSTRLEN] = {0};
#ifdef _WIN32
        inet_ntop(AF_INET, &peer.sin_addr, ip, sizeof(ip));
#else
        inet_ntop(AF_INET, &peer.sin_addr, ip, sizeof(ip));
#endif
        std::cout << "[server] 新连接来自 " << ip << ":"
                  << ntohs(peer.sin_port) << std::endl;

        threads.emplace_back(handle_client, client);
    }

    platform::close_socket(listen_sock);
    for (auto &t : threads) {
        if (t.joinable()) t.join();
    }
    platform::cleanup();
    std::cout << "[server] 已退出" << std::endl;
    return 0;
}
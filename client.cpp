//======================================================================
//  simple-chatroom / client.cpp
//  简易在线聊天室 —— 客户端（跨平台：Linux / Windows）
//
//  功能：
//    * 输入昵称并连接到服务器
//    * 接收线程实时显示其他成员的消息
//    * 主线程读取键盘输入并发送；输入 /quit 退出
//
//  编译：
//    Linux   : g++ -std=c++17 -pthread client.cpp -o chat-client
//    Windows : cl /std:c++17 /EHsc client.cpp ws2_32.lib /Fe:chat-client.exe
//    (也可用 CMake，见 CMakeLists.txt)
//======================================================================

#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <cstdlib>
#include <cstring>

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
    #pragma comment(lib, "ws2_32.lib")

    using Socket   = SOCKET;
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
    constexpr Socket kInvalidSocket = -1;
    constexpr int   kSocketError    = -1;
#endif

namespace platform {

#ifdef _WIN32
static UINT g_origOutCp = 0;   // 退出时恢复控制台原始输出代码页
static UINT g_origInCp  = 0;   // 退出时恢复控制台原始输入代码页
#endif

bool init() {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cerr << "[client] WSAStartup failed" << std::endl;
        return false;
    }
    // 源码与聊天消息均为 UTF-8；把控制台输入/输出代码页切到 UTF-8(65001)，
    // 避免在中文 Windows（默认 GBK 代码页）上显示乱码
    g_origOutCp = ::GetConsoleOutputCP();
    g_origInCp  = ::GetConsoleCP();
    ::SetConsoleOutputCP(CP_UTF8);
    ::SetConsoleCP(CP_UTF8);
#else
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

// 先 shutdown 再 close：确保阻塞在 recv 的接收线程能被唤醒（Linux 必需）
void shutdown_socket(Socket s) {
#ifdef _WIN32
    ::shutdown(s, SD_BOTH);
#else
    ::shutdown(s, SHUT_RDWR);
#endif
}

int last_error() {
#ifdef _WIN32
    return WSAGetLastError();
#else
    return errno;
#endif
}

// 读取一行（按 '\n' 分割），会立刻做出输出
bool recv_line(Socket s, std::string &line) {
    line.clear();
    char ch;
    for (;;) {
        int n = ::recv(s, &ch, 1, 0);
        if (n == 0) return false;                 // 连接被关闭
        if (n == kSocketError) return false;
        if (ch == '\n') break;
        if (ch != '\r') line.push_back(ch);
        if (line.size() > 8192) return false;
    }
    return true;
}

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

//-------------------------- 客户端逻辑 --------------------------

static void receiver(Socket s, std::atomic<bool> &connected) {
    std::string line;
    while (platform::recv_line(s, line)) {
        std::cout << line << std::endl;
    }
    connected.store(false);   // 服务器断开
    // 唤醒主线程：主线程阻塞在 getline 上，这里无法打断，
    // 因此打印提示，主线程下一次输入时会检测到 connected == false
    std::cout << "[client] 与服务器断开连接，按回车退出" << std::endl;
}

int main(int argc, char *argv[]) {
    if (!platform::init()) return 1;

    const char *host = "127.0.0.1";
    int port = 8888;

    if (argc >= 2) host = argv[1];
    if (argc >= 3) {
        port = std::atoi(argv[2]);
        if (port <= 0 || port > 65535) {
            std::cerr << "用法: chat-client [服务器地址] [端口]" << std::endl;
            std::cerr << "  默认连接 127.0.0.1:8888" << std::endl;
            return 1;
        }
    }

    Socket s = ::socket(AF_INET, SOCK_STREAM, 0);
    if (s == kInvalidSocket) {
        std::cerr << "[client] socket() failed, err="
                  << platform::last_error() << std::endl;
        platform::cleanup();
        return 1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(static_cast<u_short>(port));
    if (::inet_pton(AF_INET, host, &addr.sin_addr) != 1) {
        std::cerr << "[client] 无效的服务器地址: " << host << std::endl;
        platform::close_socket(s);
        platform::cleanup();
        return 1;
    }

    std::cout << "[client] 正在连接 " << host << ":" << port << " ..." << std::endl;
    if (::connect(s, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) == kSocketError) {
        std::cerr << "[client] 连接失败，err=" << platform::last_error()
                  << "（请确认服务器已启动）" << std::endl;
        platform::close_socket(s);
        platform::cleanup();
        return 1;
    }

    // 输入昵称（最长为发送给服务器的首行）
    std::string name;
    std::cout << "请输入昵称: ";
    std::getline(std::cin, name);
    if (name.empty()) name = "匿名用户";
    if (name.size() > 32) name = name.substr(0, 32);

    if (!platform::send_all(s, name + "\n")) {
        std::cerr << "[client] 发送昵称失败" << std::endl;
        platform::close_socket(s);
        platform::cleanup();
        return 1;
    }

    std::atomic<bool> connected{true};
    std::thread recv_thread(receiver, s, std::ref(connected));

    std::cout << "-----------------------------------------" << std::endl;
    std::cout << "  已进入聊天室，直接输入文字发送" << std::endl;
    std::cout << "  输入 /quit 退出" << std::endl;
    std::cout << "-----------------------------------------" << std::endl;

    std::string line;
    while (connected.load() && std::getline(std::cin, line)) {
        if (!connected.load()) break;               // 等待接收线程给出提示
        if (line == "/quit") break;
        if (line.empty()) continue;
        if (line.size() > 2048) line = line.substr(0, 2048);
        if (!platform::send_all(s, line + "\n")) {
            std::cerr << "[client] 发送失败，连接可能已断开" << std::endl;
            break;
        }
    }

    connected.store(false);
    platform::shutdown_socket(s);   // 先唤醒接收线程
    platform::close_socket(s);      // 再关闭，接收线程的 recv 会返回并退出
    if (recv_thread.joinable()) recv_thread.join();

    platform::cleanup();
    std::cout << "[client] 已退出" << std::endl;
    return 0;
}
#include <wfc/mswinsck/winsock_io.hpp>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>

#include <chrono>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <sstream>
#endif

namespace wfc::net {

#ifdef _WIN32

namespace {

void ensure_started() {
    static std::once_flag once;
    std::call_once(once, [] {
        WSADATA data{};
        static_cast<void>(WSAStartup(MAKEWORD(2, 2), &data));
    });
}

// A virtual LAN for running several WFC programs on one machine: when
// WFC_LAN_ADDRESS is set (say 127.0.0.2), this program is the host with that
// loopback address -- it binds there, reports it as its LocalIP -- and a
// broadcast goes to every address in WFC_LAN_PEERS (comma separated), so
// programs that look for each other with broadcasts find one another.
[[nodiscard]] std::string environment(const char* name) {
    char text[512] = {};
    const DWORD length = GetEnvironmentVariableA(name, text, sizeof(text));
    return length > 0 && length < sizeof(text) ? std::string(text)
                                               : std::string();
}

// WFC_NET_TRACE=1 reports every datagram on standard error, for diagnosing
// programs that talk to each other.
void trace(const char* what, const std::string& peer, int port,
           const std::string& data) {
    static const bool enabled = !environment("WFC_NET_TRACE").empty();
    if (!enabled) {
        return;
    }
    const auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                         std::chrono::system_clock::now().time_since_epoch())
                         .count();
    std::fprintf(stderr, "[net %lld %s] %s %s:%d '%s'\n",
                 static_cast<long long>(now % 1000000),
                 environment("WFC_LAN_ADDRESS").c_str(), what, peer.c_str(),
                 port, data.substr(0, 40).c_str());
}

[[nodiscard]] bool is_broadcast(const std::string& host) {
    return host == "255.255.255.255";
}

[[nodiscard]] SOCKET native(const Handle handle) {
    return static_cast<SOCKET>(handle);
}

[[nodiscard]] bool would_block(const int error) {
    return error == WSAEWOULDBLOCK || error == WSAEINPROGRESS ||
           error == WSAEALREADY;
}

// An IPv4 address for `host` (a name or dotted address).
[[nodiscard]] bool resolve(const std::string& host, const int port,
                           sockaddr_in& address, int& error) {
    ensure_started();
    std::memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(static_cast<u_short>(port));
    if (host.empty()) {
        address.sin_addr.s_addr = htonl(INADDR_ANY);
        return true;
    }
    addrinfo hints{};
    hints.ai_family = AF_INET;
    addrinfo* found = nullptr;
    const int status = getaddrinfo(host.c_str(), nullptr, &hints, &found);
    if (status != 0 || found == nullptr) {
        error = status != 0 ? status : WSAHOST_NOT_FOUND;
        return false;
    }
    address.sin_addr = reinterpret_cast<sockaddr_in*>(found->ai_addr)->sin_addr;
    freeaddrinfo(found);
    return true;
}

[[nodiscard]] std::string dotted(const in_addr& address) {
    char text[INET_ADDRSTRLEN] = {};
    InetNtopA(AF_INET, const_cast<in_addr*>(&address), text, sizeof(text));
    return text;
}

}  // namespace

Handle open(const bool udp, int& error) {
    ensure_started();
    const SOCKET created = socket(AF_INET, udp ? SOCK_DGRAM : SOCK_STREAM,
                                  udp ? IPPROTO_UDP : IPPROTO_TCP);
    if (created == INVALID_SOCKET) {
        error = WSAGetLastError();
        return kInvalid;
    }
    u_long on = 1;
    ioctlsocket(created, FIONBIO, &on);
    if (udp) {
        // Datagram sockets may send to a broadcast address, as Winsock's do.
        BOOL broadcast = TRUE;
        setsockopt(created, SOL_SOCKET, SO_BROADCAST,
                   reinterpret_cast<const char*>(&broadcast),
                   sizeof(broadcast));
    }
    return static_cast<Handle>(created);
}

void close(const Handle handle) {
    if (handle != kInvalid) {
        closesocket(native(handle));
    }
}

bool bind(const Handle handle, const std::string& address, const int port,
          int& error) {
    sockaddr_in local{};
    const std::string lan = environment("WFC_LAN_ADDRESS");
    if (!resolve(address.empty() ? lan : address, port, local, error)) {
        return false;
    }
    if (::bind(native(handle), reinterpret_cast<sockaddr*>(&local),
               sizeof(local)) == SOCKET_ERROR) {
        error = WSAGetLastError();
        return false;
    }
    return true;
}

int local_port(const Handle handle) {
    sockaddr_in local{};
    int size = sizeof(local);
    if (getsockname(native(handle), reinterpret_cast<sockaddr*>(&local),
                    &size) == SOCKET_ERROR) {
        return 0;
    }
    return ntohs(local.sin_port);
}

bool listen(const Handle handle, int& error) {
    if (::listen(native(handle), SOMAXCONN) == SOCKET_ERROR) {
        error = WSAGetLastError();
        return false;
    }
    return true;
}

Handle accept(const Handle handle, int& error) {
    error = 0;
    const SOCKET accepted = ::accept(native(handle), nullptr, nullptr);
    if (accepted == INVALID_SOCKET) {
        const int code = WSAGetLastError();
        error = would_block(code) ? 0 : code;
        return kInvalid;
    }
    u_long on = 1;
    ioctlsocket(accepted, FIONBIO, &on);
    return static_cast<Handle>(accepted);
}

Progress connect(const Handle handle, const std::string& host, const int port,
                 std::string& address, int& error) {
    sockaddr_in remote{};
    if (!resolve(host, port, remote, error)) {
        return Progress::Failed;
    }
    address = dotted(remote.sin_addr);
    if (::connect(native(handle), reinterpret_cast<sockaddr*>(&remote),
                  sizeof(remote)) == SOCKET_ERROR) {
        const int code = WSAGetLastError();
        if (would_block(code)) {
            return Progress::Pending;
        }
        error = code;
        return Progress::Failed;
    }
    return Progress::Done;
}

Progress connect_status(const Handle handle, int& error) {
    fd_set writable;
    fd_set failed;
    FD_ZERO(&writable);
    FD_ZERO(&failed);
    FD_SET(native(handle), &writable);
    FD_SET(native(handle), &failed);
    timeval none{0, 0};
    const int ready = select(0, nullptr, &writable, &failed, &none);
    if (ready <= 0) {
        return Progress::Pending;
    }
    if (FD_ISSET(native(handle), &failed)) {
        int code = 0;
        int size = sizeof(code);
        getsockopt(native(handle), SOL_SOCKET, SO_ERROR,
                   reinterpret_cast<char*>(&code), &size);
        error = code != 0 ? code : WSAECONNREFUSED;
        return Progress::Failed;
    }
    return Progress::Done;
}

long receive(const Handle handle, std::string& data, int& error) {
    error = 0;
    char buffer[8192];
    long total = 0;
    for (;;) {
        const int count = recv(native(handle), buffer, sizeof(buffer), 0);
        if (count > 0) {
            data.append(buffer, static_cast<std::size_t>(count));
            total += count;
            continue;
        }
        if (count == 0) {
            return total > 0 ? total : 0;  // closed after the data, if any
        }
        const int code = WSAGetLastError();
        if (would_block(code)) {
            return total > 0 ? total : -1;
        }
        error = code;
        return total > 0 ? total : -1;
    }
}

bool send(const Handle handle, const std::string& data, int& error) {
    std::size_t sent = 0;
    while (sent < data.size()) {
        const int count = ::send(native(handle), data.data() + sent,
                                 static_cast<int>(data.size() - sent), 0);
        if (count == SOCKET_ERROR) {
            const int code = WSAGetLastError();
            if (would_block(code)) {
                fd_set writable;
                FD_ZERO(&writable);
                FD_SET(native(handle), &writable);
                timeval wait{1, 0};
                select(0, nullptr, &writable, nullptr, &wait);
                continue;
            }
            error = code;
            return false;
        }
        sent += static_cast<std::size_t>(count);
    }
    return true;
}

long receive_from(const Handle handle, std::string& data, std::string& address,
                  int& port, int& error) {
    error = 0;
    char buffer[65536];
    sockaddr_in from{};
    int size = sizeof(from);
    const int count = recvfrom(native(handle), buffer, sizeof(buffer), 0,
                               reinterpret_cast<sockaddr*>(&from), &size);
    if (count == SOCKET_ERROR) {
        const int code = WSAGetLastError();
        // A reset from an earlier datagram is not an error of this one.
        error = would_block(code) || code == WSAECONNRESET ? 0 : code;
        return -1;
    }
    data.append(buffer, static_cast<std::size_t>(count));
    address = dotted(from.sin_addr);
    port = ntohs(from.sin_port);
    trace("recv", address, port, std::string(buffer, count));
    return count;
}

bool send_to(const Handle handle, const std::string& data,
             const std::string& host, const int port, int& error) {
    trace("send", host, port, data);
    const std::string peers = environment("WFC_LAN_PEERS");
    if (is_broadcast(host) && !peers.empty() &&
        !environment("WFC_LAN_ADDRESS").empty()) {
        // The virtual LAN: a broadcast reaches each peer (and this host).
        bool sent = false;
        std::stringstream list(peers);
        std::string peer_address;
        while (std::getline(list, peer_address, ',')) {
            sockaddr_in target{};
            if (!resolve(peer_address, port, target, error)) {
                continue;
            }
            sent = sendto(native(handle), data.data(),
                          static_cast<int>(data.size()), 0,
                          reinterpret_cast<sockaddr*>(&target),
                          sizeof(target)) != SOCKET_ERROR ||
                   sent;
        }
        if (!sent) {
            error = WSAGetLastError();
        }
        return sent;
    }
    sockaddr_in remote{};
    // An empty host is this host (on the virtual LAN: its own address).
    const std::string lan = environment("WFC_LAN_ADDRESS");
    if (!lan.empty() && !host.empty() && host.rfind("127.", 0) != 0) {
        return true;  // outside the virtual LAN: the datagram is lost
    }
    if (!resolve(host.empty() ? lan : host, port, remote, error)) {
        return false;
    }
    if (sendto(native(handle), data.data(), static_cast<int>(data.size()), 0,
               reinterpret_cast<sockaddr*>(&remote),
               sizeof(remote)) == SOCKET_ERROR) {
        error = WSAGetLastError();
        return false;
    }
    return true;
}

bool peer(const Handle handle, std::string& address, int& port) {
    sockaddr_in remote{};
    int size = sizeof(remote);
    if (getpeername(native(handle), reinterpret_cast<sockaddr*>(&remote),
                    &size) == SOCKET_ERROR) {
        return false;
    }
    address = dotted(remote.sin_addr);
    port = ntohs(remote.sin_port);
    return true;
}

std::string host_name() {
    ensure_started();
    char name[256] = {};
    return gethostname(name, sizeof(name)) == 0 ? name : "localhost";
}

std::string local_address() {
    if (const std::string lan = environment("WFC_LAN_ADDRESS"); !lan.empty()) {
        return lan;
    }
    sockaddr_in address{};
    int error = 0;
    if (resolve(host_name(), 0, address, error) &&
        address.sin_addr.s_addr != htonl(INADDR_ANY)) {
        return dotted(address.sin_addr);
    }
    return "127.0.0.1";
}

bool virtual_lan() {
    return !environment("WFC_LAN_ADDRESS").empty();
}

std::string error_text(const int error) {
    switch (error) {
        case WSAEADDRINUSE:
            return "Address in use";
        case WSAECONNREFUSED:
            return "Connection is forcefully rejected";
        case WSAECONNRESET:
            return "Connection is aborted due to timeout or other failure";
        case WSAETIMEDOUT:
            return "Connection timed out";
        case WSAHOST_NOT_FOUND:
        case WSANO_DATA:
            return "Authoritative answer: Host not found";
        case WSAENETUNREACH:
            return "Network is unreachable";
        default:
            break;
    }
    char text[256] = {};
    const DWORD length = FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr,
        static_cast<DWORD>(error), 0, text, sizeof(text), nullptr);
    std::string result(text, length);
    while (!result.empty() && (result.back() == '\r' || result.back() == '\n' ||
                               result.back() == ' ' || result.back() == '.')) {
        result.pop_back();
    }
    return result.empty() ? "Unexpected error " + std::to_string(error)
                          : result;
}

#else

Handle open(bool, int& error) {
    error = 10047;
    return kInvalid;
}
void close(Handle) {}
bool bind(Handle, const std::string&, int, int& error) {
    error = 10047;
    return false;
}
int local_port(Handle) {
    return 0;
}
bool listen(Handle, int& error) {
    error = 10047;
    return false;
}
Handle accept(Handle, int& error) {
    error = 10047;
    return kInvalid;
}
Progress connect(Handle, const std::string&, int, std::string&, int& error) {
    error = 10047;
    return Progress::Failed;
}
Progress connect_status(Handle, int& error) {
    error = 10047;
    return Progress::Failed;
}
long receive(Handle, std::string&, int& error) {
    error = 10047;
    return -1;
}
bool send(Handle, const std::string&, int& error) {
    error = 10047;
    return false;
}
long receive_from(Handle, std::string&, std::string&, int&, int& error) {
    error = 10047;
    return -1;
}
bool send_to(Handle, const std::string&, const std::string&, int, int& error) {
    error = 10047;
    return false;
}
bool peer(Handle, std::string&, int&) {
    return false;
}
std::string host_name() {
    return "localhost";
}
std::string local_address() {
    return "127.0.0.1";
}
bool virtual_lan() {
    return false;
}
std::string error_text(int) {
    return "Sockets are not available";
}

#endif

}  // namespace wfc::net

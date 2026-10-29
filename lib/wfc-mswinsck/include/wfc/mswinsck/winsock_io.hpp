#pragma once

// Non-blocking sockets for the Winsock control (REQ-0305). Kept apart from
// forms.cpp because winsock2.h must not meet windows.h's winsock.h. Internal
// to the WFC runtime.

#include <cstdint>
#include <string>

namespace wfc::net {

using Handle = std::intptr_t;
constexpr Handle kInvalid = -1;

// Opens a TCP or UDP socket in non-blocking mode; kInvalid on failure.
[[nodiscard]] Handle open(bool udp, int& error);
void close(Handle handle);

// Binds to `address` (empty: any) and `port` (0: any).
[[nodiscard]] bool bind(Handle handle, const std::string& address, int port,
                        int& error);
[[nodiscard]] int local_port(Handle handle);
[[nodiscard]] bool listen(Handle handle, int& error);

// The next pending connection, or kInvalid (error is 0 when none is waiting).
[[nodiscard]] Handle accept(Handle handle, int& error);

enum class Progress { Pending, Done, Failed };
// Starts a connection to `host`:`port`. `address` receives the resolved
// IPv4 address.
[[nodiscard]] Progress connect(Handle handle, const std::string& host, int port,
                               std::string& address, int& error);
// Whether a pending connection has finished.
[[nodiscard]] Progress connect_status(Handle handle, int& error);

// Appends what is available to `data`. Returns the bytes added; 0 when the
// peer closed the connection; -1 when nothing is available (error 0) or the
// call failed (error set).
[[nodiscard]] long receive(Handle handle, std::string& data, int& error);
[[nodiscard]] bool send(Handle handle, const std::string& data, int& error);
[[nodiscard]] long receive_from(Handle handle, std::string& data,
                                std::string& address, int& port, int& error);
[[nodiscard]] bool send_to(Handle handle, const std::string& data,
                           const std::string& host, int port, int& error);

// The peer of a connected socket.
[[nodiscard]] bool peer(Handle handle, std::string& address, int& port);

[[nodiscard]] std::string local_address();
// Whether this program is a host of a virtual LAN (WFC_LAN_ADDRESS is set).
[[nodiscard]] bool virtual_lan();
[[nodiscard]] std::string host_name();
[[nodiscard]] std::string error_text(int error);

}  // namespace wfc::net

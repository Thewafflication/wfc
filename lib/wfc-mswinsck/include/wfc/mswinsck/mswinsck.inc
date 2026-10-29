// Native replacement for the Winsock control (`mswinsck.ocx`, library
// MSWinsockLib; MP-0008, REQ-0305). Included once by forms.cpp after ocx_support.inc,
// inside namespace wfc::detail. Internal to the WFC evaluator; not part of the
// public API.

// ----- Winsock -------------------------------------------------------------------

namespace {

enum SocketState : Integer {
    kClosed = 0,
    kOpen = 1,
    kListening = 2,
    kConnectionPending = 3,
    kConnecting = 6,
    kConnected = 7,
    kClosing = 8,
    kSocketError = 9
};

struct WinsockState final : OcxState {
    net::Handle socket{net::kInvalid};
    std::string received;  // data waiting for GetData
    std::map<long, net::Handle> pending;  // connections offered, by request
    bool peer_closed{false};
    bool bound{false};
    ~WinsockState() override {
        net::close(socket);
        for (const auto& [id, handle] : pending) {
            net::close(handle);
        }
    }
};

std::vector<std::weak_ptr<InstanceData>>& live_sockets() {
    static std::vector<std::weak_ptr<InstanceData>> sockets;
    return sockets;
}

[[nodiscard]] WinsockState& socket_state(ControlObject& control) {
    if (control.comctl->ocx == nullptr) {
        control.comctl->ocx = std::make_shared<WinsockState>();
        live_sockets().push_back(control.self);
    }
    return static_cast<WinsockState&>(*control.comctl->ocx);
}

void set_state(Interpreter& in, ControlObject& control, const Integer state) {
    control.values["state"] = Value{state};
    // A real host sleeps in its message loop: a wake-up timer polls sockets
    // for as long as one is open.
    auto& runtime = FormSupport::runtime(in);
    if (runtime.headless == nullptr) {
        const bool awake = state != kClosed && state != kSocketError;
        if (awake && !runtime.sockets_awake) {
            runtime.host->wake_timer(0, 25);
            runtime.sockets_awake = true;
        }
    }
}

void reset_socket(Interpreter& in, ControlObject& control, const Integer state) {
    auto& socket = socket_state(control);
    net::close(socket.socket);
    socket.socket = net::kInvalid;
    for (const auto& [id, handle] : socket.pending) {
        net::close(handle);
    }
    socket.pending.clear();
    socket.received.clear();
    socket.peer_closed = false;
    control.values["bytesreceived"] = Value{Integer{0}};
    set_state(in, control, state);
}

// Reports a socket failure as the Error event.
[[nodiscard]] bool socket_error(Interpreter& in, ControlObject& control,
                                const int code) {
    reset_socket(in, control, kSocketError);
    std::vector<Value> arguments{Value{static_cast<Int16>(code)},
                                 Value{net::error_text(code)},
                                 Value{static_cast<Integer>(code)},
                                 Value{std::string{"Winsock"}},
                                 Value{std::string{}},
                                 Value{Integer{0}},
                                 Value{false}};
    const auto form = control.form.lock();
    if (form == nullptr) {
        return true;
    }
    return FormSupport::raise(in, *form, control.name, control.index, "error",
                              arguments, FormSupport::offset_of(in));
}

[[nodiscard]] bool is_udp(ControlObject& control) {
    return property_whole(control, "protocol") == 1;
}

[[nodiscard]] long socket_status(ControlObject& control) {
    return property_whole(control, "state");
}

// The bytes of a value as Winsock sends them.
[[nodiscard]] std::string wire_bytes(const Value& value) {
    const auto little = [](auto number) {
        std::string bytes(sizeof(number), '\0');
        std::memcpy(bytes.data(), &number, sizeof(number));
        return bytes;
    };
    if (const auto* text = std::get_if<std::string>(&value)) {
        return *text;
    }
    if (const auto* number = std::get_if<Integer>(&value)) {
        return little(*number);
    }
    if (const auto* number = std::get_if<Int16>(&value)) {
        return little(*number);
    }
    if (const auto* number = std::get_if<Byte>(&value)) {
        return little(*number);
    }
    if (const auto* number = std::get_if<double>(&value)) {
        return little(*number);
    }
    if (const auto* number = std::get_if<float>(&value)) {
        return little(*number);
    }
    if (const auto* flag = std::get_if<bool>(&value)) {
        return little(static_cast<Int16>(*flag ? -1 : 0));
    }
    return FormSupport::text_of(value);
}

// Reads GetData/PeekData: the value of `type` (a VarType; String by default).
[[nodiscard]] Value wire_value(const std::string& bytes, const long type) {
    const auto read = [&](auto zero) {
        decltype(zero) number{};
        std::memcpy(&number, bytes.data(), std::min(bytes.size(), sizeof(number)));
        return number;
    };
    switch (type) {
        case 2:
            return Value{read(Int16{})};
        case 3:
            return Value{read(Integer{})};
        case 4:
            return Value{read(0.0F)};
        case 5:
            return Value{read(0.0)};
        case 17:
            return Value{static_cast<Byte>(
                bytes.empty() ? 0 : static_cast<unsigned char>(bytes[0]))};
        default:
            return Value{bytes};
    }
}

[[nodiscard]] long wire_width(const long type) {
    switch (type) {
        case 2:
            return 2;
        case 3:
        case 4:
            return 4;
        case 5:
            return 8;
        case 17:
            return 1;
        default:
            return 0;  // a string takes whatever is there
    }
}

[[nodiscard]] bool raise_socket_event(Interpreter& in, ControlObject& control,
                                      const std::string& event,
                                      std::vector<Value> arguments) {
    const auto form = control.form.lock();
    if (form == nullptr) {
        return true;
    }
    return FormSupport::raise(in, *form, control.name, control.index, event,
                              arguments, FormSupport::offset_of(in));
}

// One pass over a socket control; false when a handler failed.
[[nodiscard]] bool poll_socket(Interpreter& in, ControlObject& control) {
    auto& socket = socket_state(control);
    const long state = socket_status(control);
    int error = 0;
    if (state == kListening) {
        for (;;) {
            const net::Handle accepted = net::accept(socket.socket, error);
            if (accepted == net::kInvalid) {
                return error == 0 || socket_error(in, control, error);
            }
            const long request = static_cast<long>(accepted);
            socket.pending[request] = accepted;
            if (!raise_socket_event(in, control, "connectionrequest",
                                    {Value{static_cast<Integer>(request)}})) {
                return false;
            }
            // A request nobody accepted is refused.
            if (const auto left = socket.pending.find(request);
                left != socket.pending.end()) {
                net::close(left->second);
                socket.pending.erase(left);
            }
        }
    }
    if (state == kConnecting) {
        const auto progress = net::connect_status(socket.socket, error);
        if (progress == net::Progress::Pending) {
            return true;
        }
        if (progress == net::Progress::Failed) {
            return socket_error(in, control, error);
        }
        std::string address;
        int port = 0;
        if (net::peer(socket.socket, address, port)) {
            control.values["remotehostip"] = Value{address};
        }
        set_state(in, control, kConnected);
        return raise_socket_event(in, control, "connect", {});
    }
    if (state == kConnected && !is_udp(control)) {
        const long count = net::receive(socket.socket, socket.received, error);
        if (count > 0) {
            control.values["bytesreceived"] =
                Value{static_cast<Integer>(socket.received.size())};
            if (!raise_socket_event(
                    in, control, "dataarrival",
                    {Value{static_cast<Integer>(socket.received.size())}})) {
                return false;
            }
        } else if (count == 0) {
            set_state(in, control, kClosing);
            return raise_socket_event(in, control, "close", {});
        } else if (error != 0) {
            return socket_error(in, control, error);
        }
        return true;
    }
    if (state == kOpen && is_udp(control)) {
        for (;;) {
            std::string datagram;
            std::string address;
            int port = 0;
            const long count =
                net::receive_from(socket.socket, datagram, address, port, error);
            if (count < 0) {
                return error == 0 || socket_error(in, control, error);
            }
            socket.received += datagram;
            control.values["remotehostip"] = Value{address};
            control.values["remoteport"] = Value{static_cast<Integer>(port)};
            control.values["bytesreceived"] =
                Value{static_cast<Integer>(socket.received.size())};
            if (!raise_socket_event(
                    in, control, "dataarrival",
                    {Value{static_cast<Integer>(socket.received.size())}})) {
                return false;
            }
        }
    }
    return true;
}

// Opens the control's socket when it has none; false on failure.
[[nodiscard]] bool ensure_socket(Interpreter& in, ControlObject& control,
                                 NativeFailure& failure) {
    auto& socket = socket_state(control);
    if (socket.socket != net::kInvalid) {
        return true;
    }
    int error = 0;
    socket.socket = net::open(is_udp(control), error);
    if (socket.socket == net::kInvalid) {
        failure = NativeFailure{static_cast<Integer>(error),
                                net::error_text(error)};
        return false;
    }
    control.values["sockethandle"] =
        Value{static_cast<Integer>(socket.socket)};
    static_cast<void>(in);
    return true;
}

[[nodiscard]] bool bind_socket(Interpreter& in, ControlObject& self,
                               const std::string& address, const long port,
                               NativeFailure& failure) {
    if (!ensure_socket(in, self, failure)) {
        return false;
    }
    auto& socket = socket_state(self);
    int error = 0;
    if (!net::bind(socket.socket, address, static_cast<int>(port), error)) {
        failure = NativeFailure{static_cast<Integer>(error),
                                net::error_text(error)};
        return false;
    }
    self.values["localport"] =
        Value{static_cast<Integer>(net::local_port(socket.socket))};
    socket.bound = true;
    return true;
}

bool winsock_get(Interpreter& in, ControlObject& self, const std::string& member,
                 std::vector<CallArgument>& arguments, Value& result,
                 NativeFailure& failure) {
    auto& socket = socket_state(self);
    const auto optional = [&](const std::size_t at) -> const Value* {
        return at < arguments.size() && !arguments[at].omitted
                   ? &arguments[at].value
                   : nullptr;
    };
    result = Value{Empty{}};
    if (member == "localip") {
        result = Value{net::local_address()};
        return true;
    }
    if (member == "localhostname") {
        result = Value{net::host_name()};
        return true;
    }
    if (member == "close") {
        const bool had_socket = socket.socket != net::kInvalid;
        reset_socket(in, self, kClosed);
        static_cast<void>(had_socket);
        return true;
    }
    if (member == "listen") {
        if (socket_status(self) != kClosed && socket_status(self) != kOpen) {
            failure = NativeFailure{kInvalidOperation,
                                    "Invalid operation at current state"};
            return true;
        }
        if (is_udp(self)) {
            failure = NativeFailure{kInvalidState,
                                    "Wrong protocol or connection state for "
                                    "the requested transaction or request"};
            return true;
        }
        if (socket.socket == net::kInvalid &&
            !bind_socket(in, self, "", property_whole(self, "localport"),
                         failure)) {
            return true;
        }
        int error = 0;
        if (!net::listen(socket.socket, error)) {
            failure = NativeFailure{static_cast<Integer>(error),
                                    net::error_text(error)};
            return true;
        }
        set_state(in, self, kListening);
        return true;
    }
    if (member == "bind") {
        if (socket_status(self) != kClosed) {
            failure = NativeFailure{kInvalidOperation,
                                    "Invalid operation at current state"};
            return true;
        }
        const long port = optional(0) != nullptr
                              ? static_cast<long>(as_double(*optional(0)))
                              : property_whole(self, "localport");
        const std::string address =
            optional(1) != nullptr ? FormSupport::text_of(*optional(1))
                                   : std::string{};
        if (bind_socket(in, self, address, port, failure)) {
            set_state(in, self, kOpen);
        }
        return true;
    }
    if (member == "connect") {
        if (socket_status(self) != kClosed) {
            failure = NativeFailure{kInvalidOperation,
                                    "Invalid operation at current state"};
            return true;
        }
        if (optional(0) != nullptr) {
            self.values["remotehost"] = Value{FormSupport::text_of(*optional(0))};
        }
        if (optional(1) != nullptr) {
            self.values["remoteport"] =
                Value{static_cast<Integer>(as_double(*optional(1)))};
        }
        if (is_udp(self)) {
            // A datagram socket "connects" by remembering the peer.
            if (ensure_socket(in, self, failure)) {
                set_state(in, self, kOpen);
            }
            return true;
        }
        if (!ensure_socket(in, self, failure)) {
            return true;
        }
        std::string address;
        int error = 0;
        const auto progress = net::connect(
            socket.socket, property_text(self, "remotehost"),
            static_cast<int>(property_whole(self, "remoteport")), address, error);
        if (progress == net::Progress::Failed) {
            if (!socket_error(in, self, error)) {
                failure = FormSupport::reported();
            }
            return true;
        }
        self.values["remotehostip"] = Value{address};
        set_state(in, self, kConnecting);
        return true;
    }
    if (member == "accept") {
        if (socket_status(self) != kClosed) {
            failure = NativeFailure{kInvalidOperation,
                                    "Invalid operation at current state"};
            return true;
        }
        const long request = optional(0) != nullptr
                                 ? static_cast<long>(as_double(*optional(0)))
                                 : 0;
        // The request belongs to the listening control that raised it.
        const auto form = self.form.lock();
        net::Handle taken = net::kInvalid;
        if (form != nullptr) {
            FormSupport::for_each_control(*form, [&](ControlObject& other) {
                if (other.comctl != nullptr &&
                    other.comctl->ocx != nullptr && &other != &self) {
                    auto& state = static_cast<WinsockState&>(*other.comctl->ocx);
                    const auto found = state.pending.find(request);
                    if (found != state.pending.end() && taken == net::kInvalid) {
                        taken = found->second;
                        state.pending.erase(found);
                    }
                }
            });
        }
        if (taken == net::kInvalid) {
            failure = NativeFailure{kInvalidOperation,
                                    "Invalid operation at current state"};
            return true;
        }
        socket.socket = taken;
        std::string address;
        int port = 0;
        if (net::peer(taken, address, port)) {
            self.values["remotehostip"] = Value{address};
            self.values["remoteport"] = Value{static_cast<Integer>(port)};
        }
        self.values["sockethandle"] = Value{static_cast<Integer>(taken)};
        set_state(in, self, kConnected);
        return true;
    }
    if (member == "senddata") {
        if (arguments.size() != 1U) {
            failure = NativeFailure{450, "Wrong number of arguments"};
            return true;
        }
        const std::string bytes = wire_bytes(arguments[0].value);
        int error = 0;
        if (is_udp(self)) {
            if (!ensure_socket(in, self, failure)) {
                return true;
            }
            if (!socket.bound && property_whole(self, "localport") > 0 &&
                !bind_socket(in, self, "", property_whole(self, "localport"),
                             failure)) {
                return true;  // a datagram uses LocalPort as its own port
            }
            if (socket_status(self) == kClosed) {
                set_state(in, self, kOpen);
            }
            if (!net::send_to(socket.socket, bytes,
                              property_text(self, "remotehost"),
                              static_cast<int>(property_whole(self, "remoteport")),
                              error)) {
                failure = NativeFailure{static_cast<Integer>(error),
                                        net::error_text(error)};
                return true;
            }
        } else {
            if (socket_status(self) != kConnected) {
                failure = NativeFailure{kInvalidState,
                                        "Wrong protocol or connection state "
                                        "for the requested transaction or "
                                        "request"};
                return true;
            }
            if (!net::send(socket.socket, bytes, error)) {
                if (!socket_error(in, self, error)) {
                    failure = FormSupport::reported();
                }
                return true;
            }
        }
        if (!raise_socket_event(in, self, "sendcomplete", {})) {
            failure = FormSupport::reported();
        }
        return true;
    }
    if (member == "getdata" || member == "peekdata") {
        if (arguments.empty()) {
            failure = NativeFailure{450, "Wrong number of arguments"};
            return true;
        }
        const long type = optional(1) != nullptr
                              ? static_cast<long>(as_double(*optional(1)))
                              : 8;
        long limit = optional(2) != nullptr
                         ? static_cast<long>(as_double(*optional(2)))
                         : 0;
        if (limit <= 0 || limit > static_cast<long>(socket.received.size())) {
            limit = static_cast<long>(socket.received.size());
        }
        if (const long width = wire_width(type); width > 0) {
            limit = std::min(limit, width);
        }
        const std::string taken = socket.received.substr(
            0, static_cast<std::size_t>(limit));
        if (arguments[0].byref_target != nullptr) {
            *arguments[0].byref_target = wire_value(taken, type);
        }
        if (member == "getdata") {
            socket.received.erase(0, static_cast<std::size_t>(limit));
            self.values["bytesreceived"] =
                Value{static_cast<Integer>(socket.received.size())};
        }
        return true;
    }
    return false;
}

bool winsock_put(Interpreter& in, ControlObject& self, const std::string& member,
                 const Value& value, NativeFailure& failure) {
    if (member == "protocol" && socket_status(self) != kClosed) {
        failure = NativeFailure{kInvalidOperation,
                                "Invalid operation at current state"};
        return true;
    }
    static_cast<void>(in);
    static_cast<void>(value);
    return false;
}

}  // namespace

// Polls every Winsock control for connections, data and closes; false when an
// event handler failed. Called from the event loop and DoEvents.
bool ocx_poll_sockets(Interpreter& in) {
    auto& sockets = live_sockets();
    // Handlers may create or release controls: work on a snapshot.
    std::vector<std::shared_ptr<InstanceData>> snapshot;
    for (auto it = sockets.begin(); it != sockets.end();) {
        if (auto locked = it->lock()) {
            snapshot.push_back(std::move(locked));
            ++it;
        } else {
            it = sockets.erase(it);
        }
    }
    for (const auto& instance : snapshot) {
        ControlObject* control = control_of(*instance);
        if (control == nullptr || control->comctl == nullptr ||
            control->comctl->ocx == nullptr) {
            continue;
        }
        if (!poll_socket(in, *control)) {
            return false;
        }
    }
    return true;
}

bool ocx_has_sockets() {
    for (const auto& weak : live_sockets()) {
        if (const auto locked = weak.lock()) {
            const ControlObject* control = control_of(*locked);
            if (control != nullptr && control->comctl != nullptr &&
                control->comctl->ocx != nullptr &&
                FormSupport::number_of(*control, "state") != 0.0) {
                return true;
            }
        }
    }
    return false;
}

#include "pds_screen.hpp"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <thread>
#include <utility>
#include <vector>

#include "vb_text.hpp"
#include "wfc/brun71/cp437.hpp"
#include "wfc/brun71/input.hpp"
#include "wfc/brun71/keyboard.hpp"

#ifdef WFC_BRUN71_HAS_CONSOLE
#include "wfc/brun71/console.hpp"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace wfc::detail::pds {
namespace {

using Clock = std::chrono::steady_clock;

// INKEY$ calls that find no key, after the standard input has ended, before
// the program is stopped with error 62 instead of waiting for ever.
constexpr long kEmptyPollsAfterEnd = 1000000;

struct Session {
    brun71::TextScreen screen;
    brun71::KeyBuffer keys;
#ifdef WFC_BRUN71_HAS_CONSOLE
    std::unique_ptr<brun71::Console> host;
#endif
    bool console_input = false;  // keys come from the console
    bool input_ended = false;    // the standard input is at its end
    bool after_cr = false;       // the last byte read was a CR
    long empty_polls = 0;
    std::string pending;  // the second byte of an extended key for INPUT$
    bool dirty = false;
    Clock::time_point shown{};
};

std::unique_ptr<Session> g_session;
bool g_finished = false;

#ifdef WFC_BRUN71_HAS_CONSOLE
bool is_console(DWORD which) {
    HANDLE handle = GetStdHandle(which);
    DWORD mode = 0;
    return handle != INVALID_HANDLE_VALUE && handle != nullptr &&
           GetConsoleMode(handle, &mode) != FALSE;
}
#endif

void finish_at_exit() {
    const std::string text = finish();
    if (!text.empty()) {
        std::fwrite(text.data(), 1U, text.size(), stdout);
        std::fputc('\n', stdout);
        std::fflush(stdout);
    }
}

Session& session() {
    if (!g_session) {
        g_session = std::make_unique<Session>();
        g_finished = false;
#ifdef WFC_BRUN71_HAS_CONSOLE
        // WFC_PDS_CONSOLE=0 keeps the screen off the console.
        char setting[4] = {};
        const DWORD length =
            GetEnvironmentVariableA("WFC_PDS_CONSOLE", setting, 4);
        const bool allowed = !(length == 1 && setting[0] == '0');
        const bool output = allowed && is_console(STD_OUTPUT_HANDLE);
        const bool input = is_console(STD_INPUT_HANDLE);
        if (output || input) {
            // Shown only when the output is the console; otherwise the
            // buffer is a way to read the console's keys.
            g_session->host = brun71::Console::open(output);
        }
        g_session->console_input = input && g_session->host != nullptr;
        g_session->screen.on_bell = [] {
            MessageBeep(MB_OK);
        };
#endif
        std::atexit(finish_at_exit);
    }
    return *g_session;
}

// The screen bytes of wfc text: the Windows-1252 byte of each character, or
// its code page 437 byte when the character is one of the screen's own
// glyphs (box drawing written as Unicode).
std::string screen_bytes(const std::string_view text) {
    const std::string owned(text);
    if (is_ascii_text(owned)) {
        return owned;
    }
    std::string bytes;
    for (const char16_t unit : to_utf16_units(owned)) {
        if (is_byte_half(unit)) {
            bytes.push_back(static_cast<char>(unit & 0xFFU));
            continue;
        }
        const unsigned ansi = unicode_to_ansi(unit);
        if (ansi_to_unicode(ansi) != unit) {
            if (const auto code = brun71::unicode_to_cp437(unit)) {
                bytes.push_back(static_cast<char>(*code));
                continue;
            }
        }
        bytes.push_back(static_cast<char>(ansi));
    }
    return bytes;
}

// The text of the screen as it is shown, for the plain output.
std::string screen_text(const brun71::TextScreen& screen) {
    int last = 0;
    std::vector<std::string> rows;
    for (int row = 1; row <= screen.rows(); ++row) {
        std::string line;
        int length = 0;
        for (int column = 1; column <= screen.columns(); ++column) {
            if (screen.cell(row, column).character > 32) {
                length = column;
            }
        }
        for (int column = 1; column <= length; ++column) {
            append_utf8_unit(line, brun71::cp437_to_unicode(
                                       screen.cell(row, column).character));
        }
        if (length > 0) {
            last = row;
        }
        rows.push_back(std::move(line));
    }
    std::string text;
    for (int row = 0; row < last; ++row) {
        if (row > 0) {
            text.push_back('\n');
        }
        text += rows[static_cast<std::size_t>(row)];
    }
    return text;
}

// Reads one byte of the standard input; nullopt at its end.
std::optional<unsigned char> read_input_byte() {
#ifdef _WIN32
    HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
    unsigned char byte = 0;
    DWORD read = 0;
    if (input == INVALID_HANDLE_VALUE || input == nullptr ||
        !ReadFile(input, &byte, 1, &read, nullptr) || read == 0) {
        return std::nullopt;
    }
    return byte;
#else
    const int byte = std::getchar();
    if (byte == EOF) {
        return std::nullopt;
    }
    return static_cast<unsigned char>(byte);
#endif
}

// The next key of the standard input (a file or a pipe).
std::optional<brun71::Key> read_input_key(Session& state) {
    if (state.input_ended) {
        return std::nullopt;
    }
    const auto byte = read_input_byte();
    if (!byte) {
        state.input_ended = true;
        return std::nullopt;
    }
    if (*byte == 0) {
        const auto code = read_input_byte();
        if (!code) {
            state.input_ended = true;
            return std::nullopt;
        }
        return brun71::Key{0, *code};
    }
    // CR LF, CR and LF are each one Enter.
    if (*byte == '\n' && std::exchange(state.after_cr, false)) {
        return read_input_key(state);
    }
    state.after_cr = *byte == '\r';
    if (*byte == '\r' || *byte == '\n') {
        return brun71::Key{13, 0};
    }
    return brun71::Key{*byte, 0};
}

}  // namespace

bool active() noexcept {
    return g_session != nullptr;
}

brun71::TextScreen& screen() {
    return session().screen;
}

void changed(const bool force) {
    Session& state = session();
    state.dirty = true;
#ifdef WFC_BRUN71_HAS_CONSOLE
    if (!state.host) {
        return;
    }
    const auto now = Clock::now();
    if (force || now - state.shown >= std::chrono::milliseconds(15)) {
        state.host->present(state.screen);
        state.shown = now;
        state.dirty = false;
    }
#else
    static_cast<void>(force);
#endif
}

void print_item(const std::string_view text) {
    screen().print_item(screen_bytes(text));
    changed();
}

void comma() {
    screen().comma();
    changed();
}

void tab(const int column) {
    screen().tab(column);
    changed();
}

void spc(const int count) {
    screen().spc(count);
    changed();
}

void new_line() {
    screen().new_line();
    changed();
}

std::string inkey() {
    Session& state = session();
    changed(true);
    if (state.keys.empty()) {
#ifdef WFC_BRUN71_HAS_CONSOLE
        if (state.console_input) {
            state.host->poll(state.keys);
        } else
#endif
        {
            if (const auto key = read_input_key(state)) {
                state.keys.push(*key);
            }
        }
    }
    const auto key = state.keys.pop();
    if (!key) {
        if (state.input_ended && ++state.empty_polls >= kEmptyPollsAfterEnd) {
            state.empty_polls = 0;
            throw brun71::BasicError(62);
        }
        std::this_thread::yield();
        return {};
    }
    state.empty_polls = 0;
    return ansi_bytes_to_text(brun71::inkey_string(*key));
}

namespace {

// Waits for the next key; error 62 when the standard input has ended.
brun71::Key wait_key() {
    Session& state = session();
    changed(true);
    while (true) {
        if (const auto key = state.keys.pop()) {
            return *key;
        }
#ifdef WFC_BRUN71_HAS_CONSOLE
        if (state.console_input) {
            static_cast<void>(state.host->wait(100));
            state.host->poll(state.keys);
            continue;
        }
#endif
        if (const auto key = read_input_key(state)) {
            return *key;
        }
        throw brun71::BasicError(brun71::kInputPastEnd);
    }
}

}  // namespace

void print_text(const std::string_view text) {
    screen().print_text(screen_bytes(text));
    changed();
}

std::string read_line() {
    brun71::LineEditor editor(screen());
    while (!editor.key(wait_key())) {
        changed();
    }
    changed(true);
    return ansi_bytes_to_text(editor.text());
}

std::string read_chars(const int count) {
    Session& state = session();
    std::string bytes;
    while (static_cast<int>(bytes.size()) < count) {
        if (!state.pending.empty()) {
            bytes += state.pending;
            state.pending.clear();
            continue;
        }
        const std::string key = brun71::inkey_string(wait_key());
        const auto room = static_cast<std::size_t>(count) - bytes.size();
        bytes += key.substr(0, room);
        if (key.size() > room) {
            state.pending = key.substr(room);
        }
    }
    return ansi_bytes_to_text(bytes);
}

std::string finish() {
    if (!g_session || g_finished) {
        return {};
    }
    g_finished = true;
    std::string text = screen_text(g_session->screen);
#ifdef WFC_BRUN71_HAS_CONSOLE
    g_session->host.reset();
#endif
    return text;
}

}  // namespace wfc::detail::pds

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
#include "wfc/brun71/keyline.hpp"
#include "wfc/brun71/keytrap.hpp"
#include "wfc/brun71/sound.hpp"

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
    bool live = false;           // the screen is shown on the console
    brun71::SoftKeys soft_keys;
    brun71::MusicPlayer music;
    brun71::KeyTraps traps;
    Clock::time_point polled{};  // the console was last read for events
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
        g_session->live = output && g_session->host != nullptr;
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
#if defined(_WIN32) && defined(WFC_BRUN71_HAS_CONSOLE)
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

// Takes the keys the traps catch out of the buffer (ON KEY); returns how
// many were taken.
int take_trapped(Session& state) {
    if (!state.traps.armed()) {
        return 0;
    }
    int taken = 0;
    const std::size_t count = state.keys.size();
    for (std::size_t i = 0; i < count; ++i) {
        const auto key = state.keys.pop();
        if (!key) {
            break;
        }
        if (state.traps.take(*key)) {
            ++taken;
        } else {
            static_cast<void>(state.keys.push(*key));
        }
    }
    return taken;
}

// Moves the keys typed since the last call into the buffer (from the
// console, or one key of the standard input when the buffer is empty), then
// lets the traps take theirs. Returns how many the traps took.
int fetch(Session& state) {
#ifdef WFC_BRUN71_HAS_CONSOLE
    if (state.console_input) {
        state.host->poll(state.keys);
        return take_trapped(state);
    }
#endif
    if (state.keys.empty()) {
        if (const auto key = read_input_key(state)) {
            state.keys.push(*key);
        }
    }
    return take_trapped(state);
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
        static_cast<void>(fetch(state));
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
            static_cast<void>(fetch(state));
            continue;
        }
#endif
        if (state.input_ended) {
            throw brun71::BasicError(brun71::kInputPastEnd);
        }
        static_cast<void>(fetch(state));
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

namespace {

// Plays tones on the console's speaker (Beep) when the screen is shown
// there; otherwise there is nothing to hear and no time is spent.
void play_tones(const std::vector<brun71::Tone>& tones) {
#ifdef WFC_BRUN71_HAS_CONSOLE
    if (!session().live) {
        return;
    }
    changed(true);
    for (const auto& tone : tones) {
        if (tone.milliseconds <= 0) {
            continue;
        }
        if (tone.frequency == 0) {
            Sleep(static_cast<DWORD>(tone.milliseconds));
        } else {
            Beep(static_cast<DWORD>(tone.frequency),
                 static_cast<DWORD>(tone.milliseconds));
        }
    }
#else
    static_cast<void>(tones);
#endif
}

}  // namespace

void beep() {
    if (active()) {
        play_tones({brun71::Tone{800, 250}});
    }
}

void sound(const double frequency, const double duration) {
    play_tones({brun71::sound_tone(frequency, duration)});
}

void play(const std::string_view music) {
    play_tones(session().music.play(screen_bytes(music)));
}

void sleep(const double seconds) {
    Session& state = session();
    changed(true);
    if (!state.keys.empty()) {
        return;
    }
    const bool forever = seconds <= 0.0;
#ifdef WFC_BRUN71_HAS_CONSOLE
    if (state.console_input) {
        const auto end = Clock::now() + std::chrono::duration<double>(seconds);
        while (state.keys.empty()) {
            int wait = 100;
            if (!forever) {
                const auto left =
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        end - Clock::now())
                        .count();
                if (left <= 0) {
                    return;
                }
                wait = static_cast<int>(left < 100 ? left : 100);
            }
            static_cast<void>(state.host->wait(wait));
            if (fetch(state) > 0) {
                return;  // a trapped key ends it too
            }
        }
        return;
    }
#endif
    // The standard input: a key waiting there ends the sleep at once (and
    // stays for INKEY$, unless a trap takes it); when it has ended, the time
    // passes.
    if (!state.input_ended) {
        static_cast<void>(fetch(state));
        if (!state.input_ended) {
            return;
        }
    }
    if (!forever) {
        std::this_thread::sleep_for(std::chrono::duration<double>(seconds));
    }
}

void key_line(const bool show) {
    Session& state = session();
    if (show) {
        state.soft_keys.show(state.screen);
    } else {
        state.soft_keys.hide(state.screen);
    }
    changed();
}

void assign_key(const int key, const std::string_view text) {
    Session& state = session();
    if (key >= 15 && key <= 25) {
        state.traps.define(key, screen_bytes(text));
        return;
    }
    state.soft_keys.assign(key, screen_bytes(text), state.screen);
    changed();
}

void list_keys() {
    Session& state = session();
    for (const auto& line : state.soft_keys.list()) {
        state.screen.print_text(line);
        state.screen.new_line();
    }
    changed();
}

void redraw_keys() {
    if (active()) {
        session().soft_keys.redraw(session().screen);
    }
}

const char* report_error(const int number) {
    const char* text = brun71::basic_error_text(number);
    if (g_session == nullptr || g_finished || text == nullptr) {
        return text;
    }
    brun71::TextScreen& shown = g_session->screen;
    if (shown.pos() != 1) {
        shown.new_line();
    }
    shown.print_text(text);
    shown.new_line();
    changed(true);
    return text;
}

bool events_armed() noexcept {
    return g_session != nullptr && g_session->traps.armed();
}

std::optional<int> next_key_event() {
    Session& state = *g_session;
    const auto now = Clock::now();
    if (!state.console_input ||
        now - state.polled >= std::chrono::milliseconds(10)) {
        state.polled = now;
        static_cast<void>(fetch(state));
    }
    return state.traps.next();
}

void key_trap(const int key, const brun71::TrapState trap_state) {
    session().traps.set(key, trap_state);
}

void key_handler(const int key, const bool present) {
    session().traps.handler(key, present);
}

void begin_event(const int key) {
    session().traps.begin(key);
}

void end_event(const int key) {
    session().traps.end(key);
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

// The text screen of a BASIC PDS 7.1 (QuickBASIC) program (ADR-0011): one
// per process, shared by the interpreter and native code. It starts with the
// first statement of the screen (CLS, LOCATE, COLOR, VIEW PRINT, WIDTH,
// INPUT, LINE INPUT, or the functions INKEY$, INPUT$(n), CSRLIN and POS);
// from then on PRINT to the console writes on it instead of the plain output.
//
// When the standard output is a console, the screen is shown live in a
// console screen buffer of its own and keys come from the console. Otherwise
// it is kept in memory and keys come from the standard input (each byte a
// key; a line break is Enter; a 0 byte is followed by the scan code of an
// extended key). When the program ends, the screen goes away and its text
// (rows 1 to the last one that is not blank, trailing blanks removed) is
// what the program printed.
//
// Strings cross as wfc text (UTF-8 of the Unicode characters); the screen
// holds the Windows-1252 bytes of that text, read as code page 437, which is
// what CHR$(n) and the bytes of a DOS source file mean to a PDS program.
#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "wfc/brun71/error.hpp"
#include "wfc/brun71/keytrap.hpp"
#include "wfc/brun71/mouse.hpp"
#include "wfc/brun71/screen.hpp"

namespace wfc::detail::pds {

// True once the program has used the screen.
[[nodiscard]] bool active() noexcept;

// The screen, starting the session when needed. Statements that change it
// call changed() afterwards.
[[nodiscard]] brun71::TextScreen& screen();

// Shows the changes on the console (at most every 15 ms unless forced).
void changed(bool force = false);

// PRINT: an item (wfc text), the separators and the end of the statement.
void print_item(std::string_view text);
void comma();
void tab(int column);
void spc(int count);
void new_line();

// INKEY$: the next key as wfc text, or "".
[[nodiscard]] std::string inkey();

// Text at the cursor as it is (an INPUT prompt, PRINT USING): wrapping and
// scrolling, but no item rules.
void print_text(std::string_view text);

// INPUT and LINE INPUT: waits for the line the user types at the cursor
// (brun71::LineEditor) and returns it as wfc text; the cursor stays after it.
// Throws brun71::BasicError 62 when the standard input ends first.
[[nodiscard]] std::string read_line();

// INPUT$(n): the next n characters typed, not shown (an extended key is two,
// CHR$(0) and its code). Throws brun71::BasicError 62 like read_line.
[[nodiscard]] std::string read_chars(int count);

// BEEP: a short tone, when the screen has started (a VB6 Beep is silent).
void beep();

// SOUND and PLAY (brun71 sound.hpp): heard when the screen is shown on the
// console; otherwise skipped at once. Throw brun71::BasicError 5 for an
// argument out of range.
void sound(double frequency, double duration);
void play(std::string_view music);

// SLEEP [seconds]: waits for the time, or with 0 or none for ever, until a
// key is pressed; the key stays for INKEY$. With keys from the standard
// input a key there ends it at once; once that input has ended the time
// passes (SLEEP with no time returns).
void sleep(double seconds);

// KEY ON, KEY OFF, KEY n, text$, KEY LIST (brun71 keyline.hpp); after CLS
// and WIDTH the line is drawn again.
void key_line(bool show);
void assign_key(int key, std::string_view text);
void list_keys();
void redraw_keys();

// ON KEY (brun71 keytrap.hpp). KEY n, CHR$(flags) + CHR$(scan) for n 15-25
// defines a trap key (assign_key). events_armed() is true while a trap is on
// or stopped with a handler; then the interpreter asks next_key_event()
// before each statement (reading the keys typed meanwhile) and runs the
// handler between begin_event() and end_event() (its RETURN).
[[nodiscard]] bool events_armed() noexcept;
[[nodiscard]] std::optional<int> next_key_event();
void key_trap(int key, brun71::TrapState state);
void key_handler(int key, bool present);
void begin_event(int key);
void end_event(int key);

// An error no handler took ends a program that has started the screen:
// BRUN71's message for it (brun71::basic_error_text) goes on a line of its
// own on the screen. Returns the message, nullptr when BASIC has none for
// the number (nothing is shown).
const char* report_error(int number);

// The keys of the text host of forms (ADR-0012), bypassing INKEY$: the next
// key, or nullopt when none comes within `wait_ms` (on the console). With
// keys from the standard input a key is read at once (the read waits for
// it); keys_ended() is then true once that input has ended and every key
// was taken.
[[nodiscard]] std::optional<brun71::Key> next_key(int wait_ms);
[[nodiscard]] bool keys_ended();
// True when the keys come from the console, false from the standard input.
[[nodiscard]] bool keys_from_console();
// The mouse of the text host: from then on the console reports it, and
// the mouse events come from next_mouse(). With keys from the standard
// input a mouse record (the bytes 0, 255 and four more, see
// brun71::mouse_record) is read as the key 0, 255 and its event waits
// for next_mouse().
void enable_mouse();
[[nodiscard]] std::optional<brun71::MouseEvent> next_mouse();
// Shows the screen on the console now.
void present();
// wfc text as the bytes the screen holds (see the top of this file).
[[nodiscard]] std::string to_screen(std::string_view text);

// Ends the session: closes the console screen buffer and returns the text
// of the screen (empty when there was no session, or it already ended).
[[nodiscard]] std::string finish();

// Runs a statement of the screen; a BASIC error of the runtime comes back as
// its number (5, Illegal function call), 0 when the statement succeeded.
template <typename Statement>
[[nodiscard]] int run(Statement&& statement) {
    try {
        statement(screen());
    } catch (const brun71::BasicError& error) {
        return error.number();
    }
    changed();
    return 0;
}

}  // namespace wfc::detail::pds

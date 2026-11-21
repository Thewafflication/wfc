// The text screen of a BASIC PDS 7.1 (QuickBASIC) program (ADR-0011): one
// per process, shared by the interpreter and native code. It starts with the
// first statement of the screen (CLS, LOCATE, COLOR, VIEW PRINT, WIDTH, or
// the functions INKEY$, CSRLIN and POS); from then on PRINT to the console
// writes on it instead of the plain output.
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

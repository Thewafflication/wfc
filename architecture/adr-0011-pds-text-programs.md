# ADR-0011: PDS 7.1 Text-Mode Programs on wfc-brun71

**Content type:** Architecture decision record

**Status:** Accepted by the owner as milestone M2 of `lib/wfc-brun71`
(2026-11-21); the screen statements, `INKEY$`, `CSRLIN`, `POS`, `INPUT`,
`LINE INPUT`, `INPUT$(n)`, `PRINT USING`, `SOUND`, `PLAY`, `SLEEP`,
`BEEP`, `KEY` and `ON KEY` are implemented

**Date:** 2026-11-21

## Context

The owner asked for a clone of the BASIC PDS 7.1 runtime (`BRUN71`) with a
text user interface, as a WFC library: `lib/wfc-brun71` has the text screen of
`SCREEN 0`, the keyboard of `INKEY$`, code page 437 and a Windows console host
(M1). M2 makes WFC run PDS and QuickBASIC programs on it.

WFC already runs a lone `.bas` file: its module-level statements run in order
and `PRINT` writes to the standard output, with the VB6 number format (a sign
place and a trailing space) that PDS also has. What a PDS program adds is a
screen: `CLS`, `LOCATE`, `COLOR`, `WIDTH`, `VIEW PRINT`, `INKEY$`, `CSRLIN`,
`POS`. Three things constrain the design:

1. Several of those words mean something else in Visual Basic: `Cls`, `Width`
   and `Print` are members of forms and picture boxes, `Width #n` is a file
   statement, and programs use `color`, `pos` and `width` as variable names.
2. Programs run in the interpreter and, by default, as native code. Native
   code already hands statements it does not compile to the interpreter
   (*engine statements*, ADR-0009) and calls the interpreter's library
   functions by name.
3. The interpreter collects its output and prints it when the program ends;
   native code writes as it goes. A screen program has to be seen while it
   runs, and the corpus tests compare the standard output.

## Decision

1. **One screen per process.** `src/interpreter/pds_screen.cpp` owns a
   `brun71::TextScreen` and a `brun71::KeyBuffer`, shared by the interpreter
   and native code (native programs run the interpreter as their engine). It
   starts with the first screen statement or function; a program that never
   uses one is not affected.
2. **No mode switch.** A statement is a PDS one by its place and its form: in
   a standard module (not in form, control or class code), the words `CLS`,
   `LOCATE`, `COLOR`, `WIDTH` and `VIEW PRINT` start a statement unless they
   are followed by `=`, `.`, `#` or `(`, or the program has a procedure of
   that name (the interpreter checks this). `INKEY$`, `CSRLIN` and `POS` are
   library functions, which the program's own variables and procedures
   shadow as they shadow every other one.
3. **Both engines, one implementation.** The interpreter runs the statements
   (`pds_statements.cpp`); the native parser turns them into engine
   statements (the arguments are compiled, the statement runs in the
   interpreter). The functions reach the interpreter through the existing
   library-function call.
4. **PRINT follows the screen.** Once the screen has started, console `PRINT`
   in either engine places each item on the screen (the library's layout
   rules: wrapping, scrolling, 14-column zones, `TAB`, `SPC`). What was printed
   before stays in the plain output.
5. **Where the screen is shown.** When the standard output is a console, the
   screen is shown live in a console screen buffer of its own (redrawn at
   most every 15 ms, and before every `INKEY$`), and keys come from the
   console. Otherwise the screen is kept in memory and keys come from the
   standard input: each byte a key, CR, LF or CR LF one Enter, a 0 byte
   followed by an extended key's scan code. `WFC_PDS_CONSOLE=0` keeps the
   screen off the console.
6. **The end of the program.** The screen buffer goes away and the text of the
   final screen (rows 1 to the last that is not blank, trailing blanks
   removed, code page 437 as Unicode) is printed after the plain output. So a
   program leaves its result in the terminal's history, and the corpus tests
   compare a redirected run's final screen. An exit that bypasses both
   engines' ends prints it from an `atexit` handler.
7. **Text.** wfc strings are Unicode; the screen holds the Windows-1252 bytes
   of a string (what `CHR$(n)` and the bytes of a DOS source file are), read
   as code page 437. A character that is itself a code page 437 glyph
   (box drawing written as Unicode) is placed as that glyph.
8. **No endless wait.** With the standard input at its end, 1,000,000 empty
   `INKEY$` calls in a row raise error 62 (Input past end of file), so a test
   whose keys run out fails instead of hanging; `INPUT`, `LINE INPUT` and
   `INPUT$(n)`, which wait for keys, raise it at once.
9. **Keyboard input.** `INPUT` and `LINE INPUT` without `#` (in the same
   places as the statements of 2) and `INPUT$(n)` with one argument read the
   keyboard; they start the screen like the other statements. The line
   editor and the field rules are the library's (`input.hpp`); the
   interpreter stores the fields, asking again ("Redo from start") before any
   variable changes. Native code runs them as engine statements, the
   variables passed by reference. A variable with no type is a Variant in
   WFC, so `INPUT n` stores a number when the field is one, else the text.
10. **PRINT USING.** `PRINT [#n,] USING` formats with the library
    (`using.hpp`) on the Windows-1252 bytes of the format and the strings,
    then prints the text where that `PRINT` would print: the screen, a file,
    or the plain output. Native code runs it as an engine statement.
11. **Sound, SLEEP and the soft keys.** `SOUND`, `PLAY`, `SLEEP` and `KEY`
    (`ON`, `OFF`, `LIST`, `n, text$`) are statements in the places of 2;
    `SLEEP`, `SOUND` and `PLAY` also yield to a procedure of the program
    with that name, so a `Declare Sub Sleep Lib "kernel32"` keeps working.
    Native code makes them engine statements when it lowers a call to a name
    the program does not define. The tones (library `sound.hpp`) are played
    with the Windows `Beep` only when the screen is shown on a console, so a
    redirected run is silent and spends no time. `SLEEP` ends on a key,
    which stays for `INKEY$`. `BEEP` sounds once the screen has started; a
    VB6 `Beep` stays silent as before.
12. **ON KEY.** `ON KEY(n) GOSUB label` and `KEY(n) ON/OFF/STOP` (library
    `keytrap.hpp`) keep the handlers' offsets in the interpreter. The keys a
    trap catches are taken out of the key buffer whenever keys are read
    (`INKEY$`, `INPUT`, `SLEEP`, the check below), so the program never sees
    them. While a trap is armed, `parse_statement` checks for events before
    each statement of the module level: it reads the keys typed meanwhile
    (the console at most every 10 ms; the standard input one key when none
    is waiting) and runs each waiting handler with `run_gosub`, the key
    stopped until its `RETURN`. Events that come inside a `SUB` or
    `FUNCTION` wait for the module level, a simplification of BRUN71's
    checks everywhere. Native code does not compile `GOSUB`, so such a
    program falls back to the interpreter; `KEY(n)` is not taken for an
    array or procedure named `key`.

## Consequences

- PDS programs run in WFC unchanged where they use these statements; the
  corpus programs `lib/wfc-brun71/tests/corpus/30x-pds-*` run in both engines
  (`TC-MP0002-corpus-30x-*` and `...-native`), with a `.keys` file as the
  standard input (`tests/corpus/run_corpus_case.cmake` takes `-DKEYS`).
- VB6 programs keep their meaning: the full corpus passes unchanged, and
  `303-pds-names` checks the names `color`, `pos` and `width` and `Width #n`.
- Known differences, recorded in the library's `COMPATIBILITY.md`:
  `SCREEN(row, col)` is not available (the name is VB6's `Screen` object);
  `LOCATE (5), 1` is read as an array element; native code does not see a
  program's own `Sub Locate`; `ERROR$(5)` gives VB6's text.
- `305-pds-input` and `306-pds-using` check the keyboard input and
  `PRINT USING` in both engines; `307-pds-sound-keys` the sound statements,
  `SLEEP` and the soft keys; `308-pds-sleep-api` a declared `Sleep`;
  `309-pds-on-key` key trapping.
- Next in M2: `WRITE` to the screen, `SCREEN(row, col)` and the PDS
  `ERROR$` texts; then M3, the User Interface Toolbox.

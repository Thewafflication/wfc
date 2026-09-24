cmake_minimum_required(VERSION 3.28)

project(
    WFC
    VERSION 0.1.0
    DESCRIPTION "Waughtal Foundation Classes"
    LANGUAGES CXX
)

option(WFC_BUILD_TESTS "Build WFC automated tests" ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(WfcWarnings)

add_library(wfc_frontend STATIC)
target_sources(
    wfc_frontend
    PRIVATE
        src/evaluator.cpp
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS include
        FILES
            include/wfc/evaluator.hpp
            include/wfc/version.hpp
)
target_compile_features(wfc_frontend PUBLIC cxx_std_20)
target_include_directories(
    wfc_frontend
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
)
wfc_enable_strict_warnings(wfc_frontend)

add_executable(wfc src/main.cpp)
target_compile_features(wfc PRIVATE cxx_std_20)
target_link_libraries(wfc PRIVATE wfc_frontend)
wfc_enable_strict_warnings(wfc)

include(CTest)
if(BUILD_TESTING AND WFC_BUILD_TESTS)
    add_executable(wfc_frontend_tests tests/evaluator_tests.cpp)
    target_compile_features(wfc_frontend_tests PRIVATE cxx_std_20)
    target_link_libraries(wfc_frontend_tests PRIVATE wfc_frontend)
    wfc_enable_strict_warnings(wfc_frontend_tests)

    add_test(NAME TC-MP0001-evaluator COMMAND wfc_frontend_tests)
    set_tests_properties(
        TC-MP0001-evaluator
        PROPERTIES
            LABELS "unit;mp-0001;mp-0002;vertical-slice;expressions;statements;variables;boolean"
    )

    add_test(NAME TC-MP0001-cli COMMAND wfc --eval "Print \"Hello, WFC!\"")
    set_tests_properties(
        TC-MP0001-cli
        PROPERTIES
            LABELS "integration;mp-0001;vertical-slice"
            PASS_REGULAR_EXPRESSION "Hello, WFC!"
    )

    add_test(
        NAME TC-MP0002-expression-cli
        COMMAND wfc --eval "Print (2 + 3) * 4 & \" items\""
    )
    set_tests_properties(
        TC-MP0002-expression-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions"
            PASS_REGULAR_EXPRESSION "20 items"
    )

    add_test(
        NAME TC-MP0002-program-cli
        COMMAND wfc --eval "Dim total As Long: total = (6 + 4) * 3: Print total"
    )
    set_tests_properties(
        TC-MP0002-program-cli
        PROPERTIES
            LABELS "integration;mp-0002;statements;variables"
            PASS_REGULAR_EXPRESSION "30"
    )

    add_test(
        NAME TC-MP0002-boolean-cli
        COMMAND wfc --eval "Dim ready As Boolean: ready = 2 + 3 = 5: Print ready And Not False"
    )
    set_tests_properties(
        TC-MP0002-boolean-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;boolean"
            PASS_REGULAR_EXPRESSION "True"
    )

    add_test(
        NAME TC-MP0002-conditional-cli
        COMMAND wfc --eval "Dim total As Long: If True Then total = 6 * 7 Else total = 0: If total = 42 Then Print \"selected\" Else Print \"wrong\""
    )
    set_tests_properties(
        TC-MP0002-conditional-cli
        PROPERTIES
            LABELS "integration;mp-0002;statements;conditionals"
            PASS_REGULAR_EXPRESSION "selected"
    )

    add_test(
        NAME TC-MP0002-block-conditional-cli
        COMMAND wfc --eval [=[Dim total As Long
If True Then
total = 42
End If
Print total]=]
    )
    set_tests_properties(
        TC-MP0002-block-conditional-cli
        PROPERTIES
            LABELS "integration;mp-0002;statements;conditionals"
            PASS_REGULAR_EXPRESSION "42"
    )

    add_test(
        NAME TC-MP0002-elseif-cli
        COMMAND wfc --eval [=[If False Then
Print "wrong"
ElseIf 2 + 2 = 4 Then
Print "selected"
Else
Print "wrong"
End If]=]
    )
    set_tests_properties(
        TC-MP0002-elseif-cli
        PROPERTIES
            LABELS "integration;mp-0002;statements;conditionals"
            PASS_REGULAR_EXPRESSION "selected"
    )

    add_test(
        NAME TC-MP0002-while-cli
        COMMAND wfc --eval [=[Dim index As Long
Dim total As Long
While index < 4
total = total + index
index = index + 1
Wend
Print total]=]
    )
    set_tests_properties(
        TC-MP0002-while-cli
        PROPERTIES
            LABELS "integration;mp-0002;statements;loops"
            PASS_REGULAR_EXPRESSION "6"
    )

    add_test(
        NAME TC-MP0002-do-cli
        COMMAND wfc --eval [=[Dim index As Long
Do Until index = 3
Print index
index = index + 1
Loop]=]
    )
    set_tests_properties(
        TC-MP0002-do-cli
        PROPERTIES
            LABELS "integration;mp-0002;statements;loops"
            PASS_REGULAR_EXPRESSION "2"
    )

    add_test(
        NAME TC-MP0002-posttest-do-cli
        COMMAND wfc --eval [=[Dim index As Long
Do
Print index
index = index + 1
Loop While index < 3]=]
    )
    set_tests_properties(
        TC-MP0002-posttest-do-cli
        PROPERTIES
            LABELS "integration;mp-0002;statements;loops"
            PASS_REGULAR_EXPRESSION "2"
    )

    add_test(
        NAME TC-MP0002-rem-cli
        COMMAND wfc --eval [=[Rem ignored
Print 42]=]
    )
    set_tests_properties(
        TC-MP0002-rem-cli
        PROPERTIES
            LABELS "integration;mp-0002;statements;comments"
            PASS_REGULAR_EXPRESSION "42"
    )

    add_test(
        NAME TC-MP0002-exit-do-cli
        COMMAND wfc --eval [=[Dim index As Long
Do While True
index = index + 1
If index = 3 Then
Exit Do
End If
Loop
Print index]=]
    )
    set_tests_properties(
        TC-MP0002-exit-do-cli
        PROPERTIES
            LABELS "integration;mp-0002;statements;loops"
            PASS_REGULAR_EXPRESSION "3"
    )

    add_test(
        NAME TC-MP0002-for-cli
        COMMAND wfc --eval [=[Dim index As Long
Dim total As Long
For index = 1 To 4
total = total + index
Next index
Print total]=]
    )
    set_tests_properties(
        TC-MP0002-for-cli
        PROPERTIES
            LABELS "integration;mp-0002;statements;loops"
            PASS_REGULAR_EXPRESSION "10"
    )

    add_test(
        NAME TC-MP0002-exit-for-cli
        COMMAND wfc --eval [=[Dim index As Long
For index = 1 To 10
If index = 3 Then
Exit For
End If
Next index
Print index]=]
    )
    set_tests_properties(
        TC-MP0002-exit-for-cli
        PROPERTIES
            LABELS "integration;mp-0002;statements;loops"
            PASS_REGULAR_EXPRESSION "3"
    )

    add_test(
        NAME TC-MP0002-select-case-cli
        COMMAND wfc --eval [=[Select Case 2
Case 1
Print "wrong"
Case 2
Print "selected"
Case Else
Print "wrong"
End Select]=]
    )
    set_tests_properties(
        TC-MP0002-select-case-cli
        PROPERTIES
            LABELS "integration;mp-0002;statements;conditionals"
            PASS_REGULAR_EXPRESSION "selected"
    )

    add_test(
        NAME TC-MP0002-select-list-cli
        COMMAND wfc --eval [=[Select Case 2
Case 1, 2, 3
Print "selected"
End Select]=]
    )
    set_tests_properties(
        TC-MP0002-select-list-cli
        PROPERTIES
            LABELS "integration;mp-0002;statements;conditionals"
            PASS_REGULAR_EXPRESSION "selected"
    )

    add_test(
        NAME TC-MP0002-select-range-cli
        COMMAND wfc --eval [=[Select Case 7
Case 1 To 3, 7 To 9
Print "selected"
Case Else
Print "wrong"
End Select]=]
    )
    set_tests_properties(
        TC-MP0002-select-range-cli
        PROPERTIES
            LABELS "integration;mp-0002;statements;conditionals"
            PASS_REGULAR_EXPRESSION "selected"
    )

    add_test(
        NAME TC-MP0002-select-is-cli
        COMMAND wfc --eval [=[Select Case 7
Case Is < 0
Print "wrong"
Case Is >= 5
Print "selected"
End Select]=]
    )
    set_tests_properties(
        TC-MP0002-select-is-cli
        PROPERTIES
            LABELS "integration;mp-0002;statements;conditionals"
            PASS_REGULAR_EXPRESSION "selected"
    )

    add_test(
        NAME TC-MP0002-const-cli
        COMMAND wfc --eval [=[Const base As Long = 6
Const answer As Long = base * 7
Print answer]=]
    )
    set_tests_properties(
        TC-MP0002-const-cli
        PROPERTIES
            LABELS "integration;mp-0002;statements;constants"
            PASS_REGULAR_EXPRESSION "42"
    )

    add_test(
        NAME TC-MP0002-option-explicit-cli
        COMMAND wfc --eval [=[Option Explicit
Dim value As Long
value = 42
Print value]=]
    )
    set_tests_properties(
        TC-MP0002-option-explicit-cli
        PROPERTIES
            LABELS "integration;mp-0002;statements;modules"
            PASS_REGULAR_EXPRESSION "42"
    )

    add_test(
        NAME TC-MP0002-option-compare-cli
        COMMAND wfc --eval [=[Option Compare Text
Print "Alpha" = "alpha"]=]
    )
    set_tests_properties(
        TC-MP0002-option-compare-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;modules;strings"
            PASS_REGULAR_EXPRESSION "True"
    )

    add_test(
        NAME TC-MP0002-len-cli
        COMMAND wfc --eval [=[Print Len("Hello, WFC!")]=]
    )
    set_tests_properties(
        TC-MP0002-len-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;vba;strings"
            PASS_REGULAR_EXPRESSION "11"
    )

    add_test(
        NAME TC-MP0002-string-case-cli
        COMMAND wfc --eval [=[Print LCase$("Hello, WFC!")
Print UCase("Hello, WFC!")]=]
    )
    set_tests_properties(
        TC-MP0002-string-case-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;vba;strings"
            PASS_REGULAR_EXPRESSION "hello, wfc!.*HELLO, WFC!"
    )

    add_test(
        NAME TC-MP0002-string-trim-cli
        COMMAND wfc --eval [=[Print "[" & Trim$("  Hello, WFC!  ") & "]"]=]
    )
    set_tests_properties(
        TC-MP0002-string-trim-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;vba;strings"
            PASS_REGULAR_EXPRESSION "\[Hello, WFC!\]"
    )

    add_test(
        NAME TC-MP0002-string-slice-cli
        COMMAND wfc --eval [=[Print Left$("Hello", 2) & Right$("WFC!", 2)]=]
    )
    set_tests_properties(
        TC-MP0002-string-slice-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;vba;strings"
            PASS_REGULAR_EXPRESSION "HeC!"
    )

    add_test(
        NAME TC-MP0002-mid-cli
        COMMAND wfc --eval [=[Print Mid$("Hello, WFC!", 8, 3)]=]
    )
    set_tests_properties(
        TC-MP0002-mid-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;vba;strings"
            PASS_REGULAR_EXPRESSION "WFC"
    )

    add_test(
        NAME TC-MP0002-character-code-cli
        COMMAND wfc --eval [=[Print Chr$(Asc("W")) & Chr$(70) & Chr$(67)]=]
    )
    set_tests_properties(
        TC-MP0002-character-code-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;vba;strings"
            PASS_REGULAR_EXPRESSION "WFC"
    )

    add_test(
        NAME TC-MP0002-string-reverse-cli
        COMMAND wfc --eval [=[Print StrReverse("!CFW ,olleH")]=]
    )
    set_tests_properties(
        TC-MP0002-string-reverse-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;vba;strings"
            PASS_REGULAR_EXPRESSION "Hello, WFC!"
    )

    add_test(
        NAME TC-MP0002-string-fill-cli
        COMMAND wfc --eval [=[Print "[" & Space$(3) & "]" & String$(3, "*")]=]
    )
    set_tests_properties(
        TC-MP0002-string-fill-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;vba;strings"
            PASS_REGULAR_EXPRESSION "\\[   \\]\\*\\*\\*"
    )

    add_test(
        NAME TC-MP0002-instr-cli
        COMMAND wfc --eval [=[Print InStr("Hello, WFC!", "WFC")]=]
    )
    set_tests_properties(
        TC-MP0002-instr-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;vba;strings"
            PASS_REGULAR_EXPRESSION "8"
    )

    add_test(
        NAME TC-MP0002-strcomp-cli
        COMMAND wfc --eval [=[Print StrComp("abc", "abd")]=]
    )
    set_tests_properties(
        TC-MP0002-strcomp-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;vba;strings"
            PASS_REGULAR_EXPRESSION "-1"
    )

    add_test(
        NAME TC-MP0002-replace-cli
        COMMAND wfc --eval [=[Print Replace("abcabc", "b", "X")]=]
    )
    set_tests_properties(
        TC-MP0002-replace-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;vba;strings"
            PASS_REGULAR_EXPRESSION "aXcaXc"
    )

    add_test(
        NAME TC-MP0002-radix-cli
        COMMAND wfc --eval [=[Print Hex("15.5") & " " & Hex("+17.5") & " " & Oct("7.5")]=]
    )
    set_tests_properties(
        TC-MP0002-radix-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;vba;conversion"
            PASS_REGULAR_EXPRESSION "10 12 10"
    )

    add_test(
        NAME TC-MP0002-str-cli
        COMMAND wfc --eval [=[Print "[" & Str(2.5) & "] [" & Str$(-2.5) & "]"]=]
    )
    set_tests_properties(
        TC-MP0002-str-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;vba;conversion"
            PASS_REGULAR_EXPRESSION "\\[ 2.5\\] \\[-2.5\\]"
    )

    add_test(
        NAME TC-MP0002-compare-method-cli
        COMMAND wfc --eval [=[Option Compare Binary
Print InStr(1, "aXbXc", "x", vbTextCompare)
Print StrComp("abc", "ABC", vbTextCompare)]=]
    )
    set_tests_properties(
        TC-MP0002-compare-method-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;vba;strings;constants"
            PASS_REGULAR_EXPRESSION "2.*0"
    )

    add_test(
        NAME TC-MP0002-replace-options-cli
        COMMAND wfc --eval [=[Print Replace("prefix aXbXc", "x", "-", 8, 1, vbTextCompare)]=]
    )
    set_tests_properties(
        TC-MP0002-replace-options-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;vba;strings"
            PASS_REGULAR_EXPRESSION "a-bXc"
    )

    add_test(
        NAME TC-MP0002-instrrev-cli
        COMMAND wfc --eval [=[Print InStrRev("one two one", "one")]=]
    )
    set_tests_properties(
        TC-MP0002-instrrev-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;vba;strings"
            PASS_REGULAR_EXPRESSION "9"
    )

    add_test(
        NAME TC-MP0002-val-cli
        COMMAND wfc --eval [=[Print Val(" 2 45 7th Street") & " " & Val("&HFFFF") & " " & Val("&O10")]=]
    )
    set_tests_properties(
        TC-MP0002-val-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;vba;conversion"
            PASS_REGULAR_EXPRESSION "2457 -1 8"
    )

    add_test(
        NAME TC-MP0002-integer-math-cli
        COMMAND wfc --eval [=[Print Abs(-2.5) & " " & Sgn(-0.25)]=]
    )
    set_tests_properties(
        TC-MP0002-integer-math-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;vba;math"
            PASS_REGULAR_EXPRESSION "2.5 -1"
    )

    add_test(
        NAME TC-MP0002-cstr-cli
        COMMAND wfc --eval [=[Print CStr(42) & " " & CStr(True)]=]
    )
    set_tests_properties(
        TC-MP0002-cstr-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;vba;conversion"
            PASS_REGULAR_EXPRESSION "42 True"
    )

    add_test(
        NAME TC-MP0002-clng-cli
        COMMAND wfc --eval [=[Print CLng("2.5") & " " & CLng("3.5")]=]
    )
    set_tests_properties(
        TC-MP0002-clng-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;vba;conversion"
            PASS_REGULAR_EXPRESSION "2 4"
    )

    add_test(
        NAME TC-MP0002-cbool-cli
        COMMAND wfc --eval [=[Print CStr(CBool("1.5e2")) & " " & CStr(CBool("0.0"))]=]
    )
    set_tests_properties(
        TC-MP0002-cbool-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;vba;conversion;boolean"
            PASS_REGULAR_EXPRESSION "True False"
    )

    add_test(
        NAME TC-MP0002-cbyte-cli
        COMMAND wfc --eval [=[Print CByte(True) & " " & CByte(False) & " " & CByte("2.546e2")]=]
    )
    set_tests_properties(
        TC-MP0002-cbyte-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;vba;conversion"
            PASS_REGULAR_EXPRESSION "255 0 255"
    )

    add_test(
        NAME TC-MP0002-cvar-cli
        COMMAND wfc --eval [=[Print TypeName(CVar(42)) & " " & TypeName(CVar(2.5)) & " " & CVar("value")]=]
    )
    set_tests_properties(
        TC-MP0002-cvar-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;vba;conversion;variant"
            PASS_REGULAR_EXPRESSION "Long Double value"
    )

    add_test(
        NAME TC-MP0002-macid-cli
        COMMAND wfc --eval [=[Print Hex(MacID("TEXT")) & " " & Hex(MacID("XLS8"))]=]
    )
    set_tests_properties(
        TC-MP0002-macid-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;vba;conversion;macintosh"
            PASS_REGULAR_EXPRESSION "54455854 584C5338"
    )

    add_test(
        NAME TC-MP0002-error-function-cli
        COMMAND wfc --eval [=[Print Error(5) & " | " & Error$(11) & " | " & Error(1000)]=]
    )
    set_tests_properties(
        TC-MP0002-error-function-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;vba;conversion;errors"
            PASS_REGULAR_EXPRESSION "Invalid procedure call or argument.*Division by zero.*Application-defined or object-defined error"
    )

    add_test(
        NAME TC-MP0002-cint-cli
        COMMAND wfc --eval [=[Print CInt(-32768) & " " & CInt("32767")]=]
    )
    set_tests_properties(
        TC-MP0002-cint-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;vba;conversion"
            PASS_REGULAR_EXPRESSION "-32768 32767"
    )

    add_test(
        NAME TC-MP0002-isnumeric-cli
        COMMAND wfc --eval [=[Print CStr(IsNumeric(" -3.14e2 ")) & " " & CStr(IsNumeric("1e"))]=]
    )
    set_tests_properties(
        TC-MP0002-isnumeric-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;vba;information"
            PASS_REGULAR_EXPRESSION "True False"
    )

    add_test(
        NAME TC-MP0002-typename-cli
        COMMAND wfc --eval [=[Print TypeName(42) & " " & TypeName(True) & " " & TypeName("x")]=]
    )
    set_tests_properties(
        TC-MP0002-typename-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;vba;information"
            PASS_REGULAR_EXPRESSION "Long Boolean String"
    )

    add_test(
        NAME TC-MP0002-vartype-cli
        COMMAND wfc --eval [=[Print VarType(42) & " " & VarType(True) & " " & VarType("x")]=]
    )
    set_tests_properties(
        TC-MP0002-vartype-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;vba;information"
            PASS_REGULAR_EXPRESSION "3 11 8"
    )

    add_test(
        NAME TC-MP0002-iif-cli
        COMMAND wfc --eval [=[Print IIf(1 < 2, "low", "high")]=]
    )
    set_tests_properties(
        TC-MP0002-iif-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;vba;information"
            PASS_REGULAR_EXPRESSION "low"
    )

    add_test(
        NAME TC-MP0002-choose-cli
        COMMAND wfc --eval [=[Print Choose(2, "red", "green", "blue")]=]
    )
    set_tests_properties(
        TC-MP0002-choose-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;vba;information"
            PASS_REGULAR_EXPRESSION "green"
    )

    add_test(
        NAME TC-MP0002-switch-cli
        COMMAND wfc --eval [=[Print Switch(1 > 2, "a", 2 > 1, "b")]=]
    )
    set_tests_properties(
        TC-MP0002-switch-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;vba;information"
            PASS_REGULAR_EXPRESSION "b"
    )

    add_test(
        NAME TC-MP0002-int-fix-cli
        COMMAND wfc --eval [=[Print Int(-2.5) & " " & Fix(-2.5)]=]
    )
    set_tests_properties(
        TC-MP0002-int-fix-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;vba;math"
            PASS_REGULAR_EXPRESSION "-3 -2"
    )

    add_test(
        NAME TC-MP0002-ascw-chrw-cli
        COMMAND wfc --eval [=[Print AscW("A") & " " & ChrW(66)]=]
    )
    set_tests_properties(
        TC-MP0002-ascw-chrw-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;vba;strings"
            PASS_REGULAR_EXPRESSION "65 B"
    )

    add_test(
        NAME TC-MP0002-info-predicates-cli
        COMMAND wfc --eval [=[Print CStr(IsArray(1)) & " " & CStr(IsObject(1)) & " " & CStr(IsNull(1))]=]
    )
    set_tests_properties(
        TC-MP0002-info-predicates-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;vba;information"
            PASS_REGULAR_EXPRESSION "False False False"
    )

    add_test(
        NAME TC-MP0002-rgb-cli
        COMMAND wfc --eval [=[Print RGB(255, 0, 0) & " " & RGB(0, 0, 255)]=]
    )
    set_tests_properties(
        TC-MP0002-rgb-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;vba;information;color"
            PASS_REGULAR_EXPRESSION "255 16711680"
    )

    add_test(
        NAME TC-MP0002-qbcolor-cli
        COMMAND wfc --eval [=[Print QBColor(1) & " " & QBColor(12) & " " & QBColor(15)]=]
    )
    set_tests_properties(
        TC-MP0002-qbcolor-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;vba;information;color"
            PASS_REGULAR_EXPRESSION "8388608 255 16777215"
    )

    add_test(
        NAME TC-MP0002-byte-string-cli
        COMMAND wfc --eval [=[Print LenB("WFC") & " " & AscB("A") & " " & ChrB(66)]=]
    )
    set_tests_properties(
        TC-MP0002-byte-string-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;vba;strings;bytes"
            PASS_REGULAR_EXPRESSION "3 65 B"
    )

    add_test(
        NAME TC-MP0002-byte-slice-cli
        COMMAND wfc --eval [=[Print LeftB("WFC", 2) & " " & RightB("WFC", 2) & " " & MidB("WFC", 2, 1) & " " & InStrB("WFC", "FC")]=]
    )
    set_tests_properties(
        TC-MP0002-byte-slice-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;vba;strings;bytes"
            PASS_REGULAR_EXPRESSION "WF FC F 2"
    )

    add_test(
        NAME TC-MP0002-strconv-cli
        COMMAND wfc --eval [=[Print StrConv("the quICK fox", vbProperCase)]=]
    )
    set_tests_properties(
        TC-MP0002-strconv-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;vba;strings"
            PASS_REGULAR_EXPRESSION "The Quick Fox"
    )

    add_test(
        NAME TC-MP0002-vba-constants-cli
        COMMAND wfc --eval [=[Print vbTrue & " " & vbYes & " " & vbReadOnly & " " & vbSaturday]=]
    )
    set_tests_properties(
        TC-MP0002-vba-constants-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;vba;constants"
            PASS_REGULAR_EXPRESSION "-1 6 1 7"
    )

    add_test(
        NAME TC-MP0002-vba-string-constants-cli
        COMMAND wfc --eval [=[Print AscB(vbTab) & " " & AscB(vbCr) & " " & AscB(vbLf) & " " & Len(vbCrLf) & " " & vbObjectError]=]
    )
    set_tests_properties(
        TC-MP0002-vba-string-constants-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;vba;constants"
            PASS_REGULAR_EXPRESSION "9 13 10 2 -2147221504"
    )

    add_test(
        NAME TC-MP0002-round-cli
        COMMAND wfc --eval [=[Print Round(2.5) & " " & Round(3.5) & " " & Round(1.25, 1)]=]
    )
    set_tests_properties(
        TC-MP0002-round-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;vba;math"
            PASS_REGULAR_EXPRESSION "2 4 1.2"
    )

    add_test(
        NAME TC-MP0002-double-cli
        COMMAND wfc --eval [=[Print 5 / 2 & " " & (1.5 + 2.5) & " " & CLng(2.5)]=]
    )
    set_tests_properties(
        TC-MP0002-double-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;vba;numeric;double"
            PASS_REGULAR_EXPRESSION "2.5 4 2"
    )

    add_test(
        NAME TC-MP0002-cdbl-cli
        COMMAND wfc --eval [=[Print CDbl("2.5") + CDbl(3) & " " & CSng(2.25)]=]
    )
    set_tests_properties(
        TC-MP0002-cdbl-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;vba;numeric;conversion"
            PASS_REGULAR_EXPRESSION "5.5 2.25"
    )

    add_test(
        NAME TC-MP0002-float-math-cli
        COMMAND wfc --eval [=[Print Sqr(9) & " " & Exp(0) & " " & CLng(Atn(1) * 4 * 1000000)]=]
    )
    set_tests_properties(
        TC-MP0002-float-math-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;vba;math;double"
            PASS_REGULAR_EXPRESSION "3 1 3141593"
    )

    add_test(
        NAME TC-MP0002-double-declarations-cli
        COMMAND wfc --eval [=[Dim value As Double
value = 5
Const divisor As Double = 2
Print TypeName(value) & " " & VarType(value) & " " & (value / divisor)]=]
    )
    set_tests_properties(
        TC-MP0002-double-declarations-cli
        PROPERTIES
            LABELS "integration;mp-0002;statements;numeric;double"
            PASS_REGULAR_EXPRESSION "Double 5 2.5"
    )

    add_test(
        NAME TC-MP0002-numeric-literal-suffixes-cli
        COMMAND wfc --eval [=[Print TypeName(1#) & " " & VarType(1#) & " " & TypeName(42&)]=]
    )
    set_tests_properties(
        TC-MP0002-numeric-literal-suffixes-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;numeric;literals"
            PASS_REGULAR_EXPRESSION "Double 5 Long"
    )

    add_test(
        NAME TC-MP0002-identifier-type-characters-cli
        COMMAND wfc --eval [=[Dim value#
value = 2.5
Const label$ = "value="
Print label & value# & " " & TypeName(value)]=]
    )
    set_tests_properties(
        TC-MP0002-identifier-type-characters-cli
        PROPERTIES
            LABELS "integration;mp-0002;statements;numeric;variables;constants"
            PASS_REGULAR_EXPRESSION "value=2.5 Double"
    )

    add_test(
        NAME TC-MP0002-format-cli
        COMMAND wfc --eval [=[Print Format(1234.5, "Standard") & " " & Format(0.5, "Percent") & " " & Format(True, "Yes/No") & " " & Format(-1234.5, "Currency") & " " & Format(7, "0000") & " " & Format(1234567.891, "#,##0.00")]=]
    )
    set_tests_properties(
        TC-MP0002-format-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;vba;conversion"
            PASS_REGULAR_EXPRESSION "1,234.50 50.00% Yes -\\$1,234.50 0007 1,234,567.89"
    )

    add_test(
        NAME TC-MP0002-rnd-randomize-cli
        COMMAND wfc --eval [=[Randomize 42
Print Rnd()
Print Rnd()
Randomize 42
Print Rnd()]=]
    )
    set_tests_properties(
        TC-MP0002-rnd-randomize-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;vba;math;single"
            PASS_REGULAR_EXPRESSION "0.21560597.*0.52674717.*0.21560597"
    )

    add_test(
        NAME TC-MP0002-single-cli
        COMMAND wfc --eval [=[Dim x As Single
x = 2.5
Print x & " " & TypeName(x) & " " & (x + 1) & " " & TypeName(x + 1) & " " & (x + 1.0#) & " " & TypeName(x + 1.0#)]=]
    )
    set_tests_properties(
        TC-MP0002-single-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;numeric;variables;single"
            PASS_REGULAR_EXPRESSION "2.5 Single 3.5 Single 3.5 Double"
    )

    add_test(
        NAME TC-MP0002-currency-cli
        COMMAND wfc --eval [=[Dim x As Currency
x = 10.5
Print x & " " & TypeName(x) & " " & (10@ / 3@) & " " & (100.5@ * 3@)]=]
    )
    set_tests_properties(
        TC-MP0002-currency-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;numeric;variables;currency"
            PASS_REGULAR_EXPRESSION "10.5 Currency 3.3333 301.5"
    )

    add_test(
        NAME TC-MP0002-decimal-cli
        COMMAND wfc --eval [=[Dim x As Variant
x = CDec("10.25")
Print x & " " & TypeName(x) & " " & (x * CDec(2)) & " " & (CDec(1) / CDec(3))]=]
    )
    set_tests_properties(
        TC-MP0002-decimal-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;numeric;variables;decimal;variant"
            PASS_REGULAR_EXPRESSION "10.25 Decimal 20.5 0.3333333333333333333333333333"
    )

    add_test(
        NAME TC-MP0002-variant-cli
        COMMAND wfc --eval [=[Dim x As Variant
x = 5
Print TypeName(x) & " " & CStr(IsNull(x)) & " " & CStr(IsEmpty(x))
x = Null
Print TypeName(x) & " " & CStr(IsNull(x)) & " " & (x + 1 & "|" & x & "|")
x = "hello"
Print TypeName(x) & " " & x]=]
    )
    set_tests_properties(
        TC-MP0002-variant-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;variables;variant"
            PASS_REGULAR_EXPRESSION "Long False False.*Null True \\|\\|.*String hello"
    )

    add_test(
        NAME TC-MP0002-integer-cli
        COMMAND wfc --eval [=[Dim x As Integer
x = 5
Print x & " " & TypeName(x) & " " & (x + 3&) & " " & TypeName(x + 3&) & " " & (x + 1.0#) & " " & TypeName(x + 1.0#)]=]
    )
    set_tests_properties(
        TC-MP0002-integer-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;numeric;variables;integer"
            PASS_REGULAR_EXPRESSION "5 Integer 8 Long 6 Double"
    )

    add_test(
        NAME TC-MP0002-array-cli
        COMMAND wfc --eval [=[Dim arr(3) As Long
arr(0) = 10
arr(3) = 40
Print arr(0) & " " & arr(3) & " " & LBound(arr) & " " & UBound(arr) & " " & CStr(IsArray(arr))]=]
    )
    set_tests_properties(
        TC-MP0002-array-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;variables;array"
            PASS_REGULAR_EXPRESSION "10 40 0 3 True"
    )

    add_test(
        NAME TC-MP0002-object-cli
        COMMAND wfc --eval [=[Dim x As Object
Print TypeName(Nothing) & " " & CStr(IsObject(x))
Set x = Nothing
Print CStr(x Is Nothing)]=]
    )
    set_tests_properties(
        TC-MP0002-object-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;variables;object"
            PASS_REGULAR_EXPRESSION "Nothing True.*True"
    )

    add_test(
        NAME TC-MP0002-procedure-cli
        COMMAND wfc --eval [=[Function Factorial(n As Long) As Long
If n <= 1 Then
Factorial = 1
Else
Factorial = n * Factorial(n - 1)
End If
End Function

Sub Increment(ByRef x As Long)
x = x + 1
End Sub

Print Factorial(5)
Dim n As Long
n = 10
Call Increment(n)
Print n]=]
    )
    set_tests_properties(
        TC-MP0002-procedure-cli
        PROPERTIES
            LABELS "integration;mp-0002;statements;procedures"
            PASS_REGULAR_EXPRESSION "120.*11"
    )

    add_test(
        NAME TC-MP0002-class-cli
        COMMAND wfc
            --class Counter [=[Public value As Long

Sub Increment()
value = value + 1
End Sub

Function GetValue() As Long
GetValue = value
End Function

Property Get Doubled() As Long
Doubled = value * 2
End Property

Property Let Doubled(v As Long)
value = v \ 2
End Property]=]
            --eval [=[Dim c As New Counter
Call c.Increment()
Call c.Increment()
Print c.GetValue()
Print c.Doubled
c.Doubled = 20
Print c.value
Print TypeName(c) & " " & VarType(c) & " " & CStr(IsObject(c))
Dim d As Counter
Print CStr(d Is Nothing)
Set d = c
Print CStr(d Is c)
Print CStr(c Is New Counter)]=]
    )
    set_tests_properties(
        TC-MP0002-class-cli
        PROPERTIES
            LABELS "integration;mp-0002;statements;classes;object"
            PASS_REGULAR_EXPRESSION "2.*4.*10.*Counter 9 True.*True.*True.*False"
    )

    add_test(
        NAME TC-MP0002-class-lifecycle-cli
        COMMAND wfc
            --class Foo [=[Public n As Long

Sub Class_Initialize()
n = 100
Print "init"
End Sub

Sub Class_Terminate()
Print "term " & n
End Sub

Sub Bump()
n = n + 1
Print Me.n
End Sub]=]
            --eval [=[Dim x As New Foo
Call x.Bump()
Call x.Bump()
Set x = Nothing
Print "after"]=]
    )
    set_tests_properties(
        TC-MP0002-class-lifecycle-cli
        PROPERTIES
            LABELS "integration;mp-0002;statements;classes;object"
            PASS_REGULAR_EXPRESSION "init.*101.*102.*term 102.*after"
    )

    add_test(
        NAME TC-MP0002-class-refinements-cli
        COMMAND wfc
            --class Counter [=[Public n As Long]=]
            --class Pair [=[Public a As Long
Public b As Long

Property Get Item(i As Long) As Long
If i = 0 Then
Item = a
Else
Item = b
End If
End Property

Property Let Item(i As Long, v As Long)
If i = 0 Then
a = v
Else
b = v
End If
End Property]=]
            --class Holder [=[Public c As Counter]=]
            --eval [=[Dim p As New Pair
p.Item(0) = 10
p.Item(1) = 20
Print p.Item(0) & " " & p.Item(1)
Dim h As New Holder
Dim c As New Counter
c.n = 5
Set h.c = c
Print h.c.n & " " & CStr(h.c Is c)]=]
    )
    set_tests_properties(
        TC-MP0002-class-refinements-cli
        PROPERTIES
            LABELS "integration;mp-0002;statements;classes;object"
            PASS_REGULAR_EXPRESSION "10 20.*5 True"
    )

    add_test(
        NAME TC-MP0002-procedure-modifiers-cli
        COMMAND wfc
            --class Foo [=[Private secret As Long

Public Sub SetSecret(v As Long)
secret = v
End Sub

Private Function Helper() As Long
Helper = secret * 2
End Function

Public Function DoubleSecret() As Long
DoubleSecret = Helper()
End Function]=]
            --eval [=[Function Greet(name As String, Optional greeting As String = "Hello") As String
Greet = greeting & ", " & name
End Function
Print Greet("World")
Print Greet("Bob", "Hi")

Function Total(ParamArray nums() As Long) As Long
Dim i As Long
Dim s As Long
s = 0
For i = LBound(nums) To UBound(nums)
s = s + nums(i)
Next i
Total = s
End Function
Print Total(1, 2, 3, 4)

Function NextId() As Long
Static counter As Long
counter = counter + 1
NextId = counter
End Function
Print NextId()
Print NextId()

Dim f As New Foo
Call f.SetSecret(10)
Print f.DoubleSecret()]=]
    )
    set_tests_properties(
        TC-MP0002-procedure-modifiers-cli
        PROPERTIES
            LABELS "integration;mp-0002;statements;procedures;classes"
            PASS_REGULAR_EXPRESSION "Hello, World.*Hi, Bob.*10.*1.*2.*20"
    )

    add_test(
        NAME TC-MP0002-redim-cli
        COMMAND wfc --eval [=[Dim arr() As Long
Print CStr(IsArray(arr)) & " " & TypeName(arr)
ReDim arr(3)
arr(0) = 1
arr(3) = 4
Print arr(0) & " " & arr(3) & " " & LBound(arr) & " " & UBound(arr)
ReDim Preserve arr(5)
Print arr(0) & " " & arr(3) & " " & arr(5) & " " & UBound(arr)]=]
    )
    set_tests_properties(
        TC-MP0002-redim-cli
        PROPERTIES
            LABELS "integration;mp-0002;statements;array"
            PASS_REGULAR_EXPRESSION "True Long\\(\\).*1 4 0 3.*1 4 0 5"
    )

    add_test(
        NAME TC-MP0002-erase-cli
        COMMAND wfc --eval [=[Dim arr(3) As Long
arr(0) = 1
arr(3) = 4
Erase arr
Print arr(0) & " " & arr(3) & " " & UBound(arr)
Dim dyn() As Long
ReDim dyn(2)
dyn(0) = 5
Erase dyn
Print CStr(IsArray(dyn))]=]
    )
    set_tests_properties(
        TC-MP0002-erase-cli
        PROPERTIES
            LABELS "integration;mp-0002;statements;array"
            PASS_REGULAR_EXPRESSION "0 0 3.*True"
    )

    add_test(
        NAME TC-MP0002-for-each-cli
        COMMAND wfc --eval [=[Dim arr(3) As Long
Dim x As Long
Dim total As Long
arr(0) = 10
arr(1) = 20
arr(2) = 30
arr(3) = 40
For Each x In arr
total = total + x
Next x
Print total]=]
    )
    set_tests_properties(
        TC-MP0002-for-each-cli
        PROPERTIES
            LABELS "integration;mp-0002;statements;loops;array"
            PASS_REGULAR_EXPRESSION "100"
    )

    add_test(
        NAME TC-MP0002-multidim-array-cli
        COMMAND wfc --eval [=[Dim arr(1 To 2, 1 To 3) As Long
arr(1, 1) = 1
arr(2, 3) = 6
Print arr(1, 1) & " " & arr(2, 3) & " " & LBound(arr, 2) & " " & UBound(arr, 2) & " " & TypeName(arr)]=]
    )
    set_tests_properties(
        TC-MP0002-multidim-array-cli
        PROPERTIES
            LABELS "integration;mp-0002;statements;array"
            PASS_REGULAR_EXPRESSION "1 6 1 3 Long\\(\\)"
    )

    add_test(
        NAME TC-MP0002-array-parameter-cli
        COMMAND wfc --eval [=[Function Total(nums() As Long) As Long
Dim i As Long
Dim s As Long
s = 0
For i = LBound(nums) To UBound(nums)
s = s + nums(i)
nums(i) = nums(i) * 2
Next i
Total = s
End Function
Dim arr(3) As Long
arr(0) = 1
arr(1) = 2
arr(2) = 3
arr(3) = 4
Print Total(arr)
Print arr(0) & " " & arr(1) & " " & arr(2) & " " & arr(3)]=]
    )
    set_tests_properties(
        TC-MP0002-array-parameter-cli
        PROPERTIES
            LABELS "integration;mp-0002;statements;procedures;array"
            PASS_REGULAR_EXPRESSION "10.*2 4 6 8"
    )

    add_test(
        NAME TC-MP0002-variant-object-array-cli
        COMMAND wfc --eval [=[Dim arr(2) As Variant
arr(0) = 5
arr(1) = "hello"
arr(2) = True
Print TypeName(arr(0)) & " " & TypeName(arr(1)) & " " & TypeName(arr(2)) & " " & TypeName(arr)
Dim objs(0) As Object
Print TypeName(objs) & " " & CStr(objs(0) Is Nothing)]=]
    )
    set_tests_properties(
        TC-MP0002-variant-object-array-cli
        PROPERTIES
            LABELS "integration;mp-0002;statements;array;variant;object"
            PASS_REGULAR_EXPRESSION "Long String Boolean Variant\\(\\).*Object\\(\\) True"
    )

    add_test(
        NAME TC-MP0002-parenless-call-cli
        COMMAND wfc --eval [=[Function NextId() As Long
Static counter As Long
counter = counter + 1
NextId = counter
End Function
Print NextId
Call NextId
Print NextId
Print Rnd]=]
    )
    set_tests_properties(
        TC-MP0002-parenless-call-cli
        PROPERTIES
            LABELS "integration;mp-0002;statements;procedures"
            PASS_REGULAR_EXPRESSION "1.*3.*0\\.7055475"
    )

    add_test(
        NAME TC-MP0002-class-typed-array-cli
        COMMAND wfc --class Counter [=[Public value As Long]=]
            --eval [=[Dim arr(1) As Counter
Print TypeName(arr) & " " & CStr(arr(0) Is Nothing)
Dim c As New Counter
Set arr(0) = c
Print CStr(arr(0) Is c)]=]
    )
    set_tests_properties(
        TC-MP0002-class-typed-array-cli
        PROPERTIES
            LABELS "integration;mp-0002;statements;array;classes"
            PASS_REGULAR_EXPRESSION "Counter\\(\\) True.*True"
    )

    add_test(
        NAME TC-MP0002-array-return-cli
        COMMAND wfc --eval [=[Function MakeArray() As Long()
Dim result(2) As Long
result(0) = 10
result(1) = 20
result(2) = 30
MakeArray = result
End Function
Dim x() As Long
x = MakeArray()
Print x(0) & " " & x(1) & " " & x(2) & " " & UBound(x)]=]
    )
    set_tests_properties(
        TC-MP0002-array-return-cli
        PROPERTIES
            LABELS "integration;mp-0002;statements;procedures;array"
            PASS_REGULAR_EXPRESSION "10 20 30 2"
    )

    add_test(
        NAME TC-MP0002-bare-call-cli
        COMMAND wfc --class Counter [=[Private n As Long

Sub Increment()
n = n + 1
End Sub

Property Get Total() As Long
Total = n
End Property]=]
            --eval [=[Function NextId() As Long
Static counter As Long
counter = counter + 1
NextId = counter
End Function
NextId
Print NextId
Dim c As New Counter
c.Increment
Print c.Total]=]
    )
    set_tests_properties(
        TC-MP0002-bare-call-cli
        PROPERTIES
            LABELS "integration;mp-0002;statements;procedures;classes"
            PASS_REGULAR_EXPRESSION "2.*1"
    )

    add_test(
        NAME TC-MP0002-dynamic-multidim-array-cli
        COMMAND wfc --eval [=[Dim arr() As Long
ReDim arr(1, 2)
arr(0, 0) = 1
arr(1, 2) = 6
Print arr(0, 0) & " " & arr(1, 2) & " " & UBound(arr, 2)
ReDim Preserve arr(1, 4)
arr(1, 4) = 9
Print arr(0, 0) & " " & arr(1, 4) & " " & UBound(arr, 2)
Erase arr
ReDim arr(2, 2)
Print UBound(arr, 2)]=]
    )
    set_tests_properties(
        TC-MP0002-dynamic-multidim-array-cli
        PROPERTIES
            LABELS "integration;mp-0002;statements;array"
            PASS_REGULAR_EXPRESSION "1 6 2.*1 9 4.*2"
    )
endif()

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
        COMMAND wfc --eval [=[Print Hex(255) & " " & Oct(8)]=]
    )
    set_tests_properties(
        TC-MP0002-radix-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;vba;conversion"
            PASS_REGULAR_EXPRESSION "FF 10"
    )

    add_test(
        NAME TC-MP0002-str-cli
        COMMAND wfc --eval [=[Print "[" & Str(42) & "]"]=]
    )
    set_tests_properties(
        TC-MP0002-str-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;vba;conversion"
            PASS_REGULAR_EXPRESSION "\\[ 42\\]"
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
        COMMAND wfc --eval [=[Print Val(" 2 45 7th Street")]=]
    )
    set_tests_properties(
        TC-MP0002-val-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;vba;conversion"
            PASS_REGULAR_EXPRESSION "2457"
    )

    add_test(
        NAME TC-MP0002-integer-math-cli
        COMMAND wfc --eval [=[Print Abs(-42) & " " & Sgn(-42)]=]
    )
    set_tests_properties(
        TC-MP0002-integer-math-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;vba;math"
            PASS_REGULAR_EXPRESSION "42 -1"
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
        COMMAND wfc --eval [=[Print CLng(" 42 ") & " " & CLng(True)]=]
    )
    set_tests_properties(
        TC-MP0002-clng-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;vba;conversion"
            PASS_REGULAR_EXPRESSION "42 -1"
    )

    add_test(
        NAME TC-MP0002-cbool-cli
        COMMAND wfc --eval [=[Print CStr(CBool(1)) & " " & CStr(CBool("False"))]=]
    )
    set_tests_properties(
        TC-MP0002-cbool-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;vba;conversion;boolean"
            PASS_REGULAR_EXPRESSION "True False"
    )

    add_test(
        NAME TC-MP0002-cbyte-cli
        COMMAND wfc --eval [=[Print CByte(0) & " " & CByte(255)]=]
    )
    set_tests_properties(
        TC-MP0002-cbyte-cli
        PROPERTIES
            LABELS "integration;mp-0002;expressions;vba;conversion"
            PASS_REGULAR_EXPRESSION "0 255"
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
        COMMAND wfc --eval [=[Print CStr(IsNumeric(" -42 ")) & " " & CStr(IsNumeric("abc"))]=]
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
endif()

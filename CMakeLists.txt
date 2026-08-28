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
endif()

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
            LABELS "unit;mp-0001;mp-0002;vertical-slice;expressions;statements;variables"
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
endif()

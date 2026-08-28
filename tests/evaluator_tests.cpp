#include "wfc/evaluator.hpp"

#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {

int failures = 0;

void expect_success(const std::string_view source, const std::string_view expected) {
    const auto result = wfc::evaluate_print_statement(source);
    if (!result.success || result.output != expected || !result.diagnostic.empty()) {
        std::cerr << "expected success for [" << source << "] but got ["
                  << result.diagnostic << "]\n";
        ++failures;
    }
}

void expect_failure(const std::string_view source, const std::string_view code) {
    const auto result = wfc::evaluate_print_statement(source);
    if (result.success || !result.output.empty() || !result.diagnostic.starts_with(code)) {
        std::cerr << "expected " << code << " for [" << source << "] but got ["
                  << result.diagnostic << "]\n";
        ++failures;
    }
}

void expect_program_success(const std::string_view source, const std::string_view expected) {
    const auto result = wfc::evaluate_program(source);
    if (!result.success || result.output != expected || !result.diagnostic.empty()) {
        std::cerr << "expected program success for [" << source << "] but got ["
                  << result.diagnostic << "]\n";
        ++failures;
    }
}

void expect_program_failure(const std::string_view source, const std::string_view code) {
    const auto result = wfc::evaluate_program(source);
    if (result.success || !result.output.empty() || !result.diagnostic.starts_with(code)) {
        std::cerr << "expected " << code << " for program [" << source << "] but got ["
                  << result.diagnostic << "]\n";
        ++failures;
    }
}

}  // namespace

int main() {
    expect_success("Print \"Hello, WFC!\"", "Hello, WFC!");
    expect_success("  pRiNt  \"case insensitive\"  \r\n", "case insensitive");
    expect_success("Print \"VB says \"\"hello\"\".\"", "VB says \"hello\".");
    expect_success("Print \"\"", "");
    expect_success("Print 42", "42");
    expect_success("Print 2 + 3 * 4", "14");
    expect_success("Print (2 + 3) * 4", "20");
    expect_success("Print -10 + +3", "-7");
    expect_success("Print -2147483648", "-2147483648");
    expect_success("Print 17 \\ 5", "3");
    expect_success("Print 17 Mod 5", "2");
    expect_success("Print 2 + 3 & \" items\"", "5 items");
    expect_success("Print \"page \" & 2 & \" of \" & (1 + 2)", "page 2 of 3");

    expect_failure("", "WFC0001");
    expect_failure("Printer \"no\"", "WFC0001");
    expect_failure("Print", "WFC0002");
    expect_failure("Print value", "WFC0002");
    expect_failure("Print \"unterminated", "WFC0003");
    expect_failure("Print \"ok\" trailing", "WFC0004");
    expect_failure("Print (1 + 2", "WFC0005");
    expect_failure("Print 2147483648", "WFC0006");
    expect_failure("Print -2147483649", "WFC0006");
    expect_failure("Print \"one\" + \"two\"", "WFC0007");
    expect_failure("Print 1 \\ 0", "WFC0008");
    expect_failure("Print 2147483647 + 1", "WFC0009");

    expect_program_success(
        "Dim count As Long\r\n"
        "count = 6\r\n"
        "Let count = count * 7\r\n"
        "Print count",
        "42");
    expect_program_success(
        "dim Greeting as string: GREETING = \"Hello\": Print greeting & \"!\"",
        "Hello!");
    expect_program_success(
        "' defaults and comments\n"
        "Dim number As Long ' initialized to zero\n"
        "Dim text As String\n"
        "Print text & number\n"
        "Print number + 1",
        "0\n1");
    expect_program_success("Print \"\"\nPrint \"second\"", "\nsecond");
    expect_program_success(
        "Print True\n"
        "Print False\n"
        "Print 2 + 3 * 4 = 14\n"
        "Print \"a\" & \"b\" = \"ab\"\n"
        "Print 2 < 10\n"
        "Print \"alpha\" < \"beta\"\n"
        "Print 3 <> 4\n"
        "Print 3 <= 3\n"
        "Print 4 > 3\n"
        "Print 4 >= 5\n"
        "Print True <> False",
        "True\nFalse\nTrue\nTrue\nTrue\nTrue\nTrue\nTrue\nTrue\nFalse\nTrue");
    expect_program_success(
        "Print Not False And False Or True\n"
        "Print True Xor True\n"
        "Print True Eqv True\n"
        "Print True Imp False",
        "True\nFalse\nTrue\nFalse");
    expect_program_success(
        "Dim ready As Boolean\n"
        "ready = 10 >= 5\n"
        "Print ready",
        "True");

    expect_program_failure("Dim 1 As Long", "WFC0011");
    expect_program_failure("Dim value As Integer", "WFC0012");
    expect_program_failure("Dim value As Long: Dim VALUE As Long", "WFC0013");
    expect_program_failure("Dim value As Long: value 1", "WFC0014");
    expect_program_failure("missing = 1", "WFC0015");
    expect_program_failure("Print missing", "WFC0015");
    expect_program_failure("Dim value As Long: value = \"wrong\"", "WFC0016");
    expect_program_failure("Dim Print As Long", "WFC0017");
    expect_program_failure("Print 1 = \"1\"", "WFC0018");
    expect_program_failure("Print True < False", "WFC0018");
    expect_program_failure("Print 1 And 2", "WFC0019");
    expect_program_failure("Print True & \" value\"", "WFC0020");

    if (failures != 0) {
        std::cerr << failures << " evaluator test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "all evaluator tests passed\n";
    return EXIT_SUCCESS;
}

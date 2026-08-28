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

    if (failures != 0) {
        std::cerr << failures << " evaluator test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "all evaluator tests passed\n";
    return EXIT_SUCCESS;
}
